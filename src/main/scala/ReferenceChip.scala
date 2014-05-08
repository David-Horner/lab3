package referencechip

import Chisel._
import uncore._
import rocket._
import rocket.Util._
import ReferenceChipBackend._
import scala.collection.mutable.HashMap
import DRAMModel._
import BOOM.DpathDebugIo

object DesignSpaceConstants {
  val NTILES = 1
  val NBANKS = 1
  val HTIF_WIDTH = 16
  val ENABLE_SHARING = true
  val ENABLE_CLEAN_EXCLUSIVE = true
  val HAS_FPU = false
  val NL2_REL_XACTS = 1
  val NL2_ACQ_XACTS = 7
  val NMSHRS = 2
}

object MemoryConstants {
  val CACHE_DATA_SIZE_IN_BYTES = 1 << 6 //TODO: How configurable is this really?
  val OFFSET_BITS = log2Up(CACHE_DATA_SIZE_IN_BYTES)
  val PADDR_BITS = 32
  val VADDR_BITS = 43
  val PGIDX_BITS = 13
  val ASID_BITS = 7
  val PERM_BITS = 6
  val MEM_TAG_BITS = 5
  val MEM_DATA_BITS = 128
  val MEM_ADDR_BITS = PADDR_BITS - OFFSET_BITS
  val MEM_DATA_BEATS = 4
}

object TileLinkSizeConstants {
  val WRITE_MASK_BITS = 6
  val SUBWORD_ADDR_BITS = 3
  val ATOMIC_OP_BITS = 4
}

import DesignSpaceConstants._
import MemoryConstants._
import TileLinkSizeConstants._

object ReferenceChipBackend {
  val initMap = new HashMap[Module, Bool]()
}

class ReferenceChipBackend extends VerilogBackend
{
  initMap.clear()
  override def emitPortDef(m: MemAccess, idx: Int) = {
    val res = new StringBuilder()
    for (node <- m.mem.inputs) {
      if(node.name.contains("init"))
         res.append("    .init(" + node.name + "),\n")
    }
    (if (idx == 0) res.toString else "") + super.emitPortDef(m, idx)
  }

  def addMemPin(c: Module) = {
    for (mod <- Module.components; node <- mod.nodes) {
      if (node.isInstanceOf[Mem[ _ ]] && node.component != null && node.asInstanceOf[Mem[_]].seqRead) {
        connectMemPin(c, node.component, node)
      }
    }
  }

  def connectMemPin(topC: Module, c: Module, p: Node): Unit = {
    var isNewPin = false
    val compInitPin = 
      if (initMap.contains(c)) {
        initMap(c)
      } else {
        isNewPin = true
        val res = Bool(INPUT)
        res.isIo = true
        res
      }

    p.inputs += compInitPin

    if (isNewPin) {
      compInitPin.setName("init")
      c.io.asInstanceOf[Bundle] += compInitPin
      compInitPin.component = c
      initMap += (c -> compInitPin)
      connectMemPin(topC, c.parent, compInitPin)
    }
  }

  def addTopLevelPin(c: Module) = {
    val init = Bool(INPUT)
    init.isIo = true
    init.setName("init")
    init.component = c
    c.io.asInstanceOf[Bundle] += init
    initMap += (c -> init)
  }

  transforms += ((c: Module) => addTopLevelPin(c))
  transforms += ((c: Module) => addMemPin(c))
  transforms += ((c: Module) => collectNodesIntoComp(initializeDFS))
}

class Fame1ReferenceChipBackend extends ReferenceChipBackend with Fame1Transform

class OuterMemorySystem(htif_width: Int)(implicit conf: UncoreConfiguration) extends Module
{
  implicit val (tl, ln, l2, mif) = (conf.tl, conf.tl.ln, conf.l2, conf.mif)
  val io = new Bundle {
    val tiles = Vec.fill(conf.nTiles){new TileLinkIO}.flip
    val htif = (new TileLinkIO).flip
    val incoherent = Vec.fill(ln.nClients){Bool()}.asInput
    val mem = new MemIO
    val mem_backup = new MemSerializedIO(htif_width)
    val mem_backup_en = Bool(INPUT)
  }

  val refill_cycles = tl.dataBits/mif.dataBits
  val llc_tag_leaf = Mem(Bits(width = 152), 512, seqRead = true)
  val llc_data_leaf = Mem(Bits(width = 64), 4096, seqRead = true)
  val llc = Module(new DRAMSideLLC(sets=512, ways=8, outstanding=16, refill_cycles=refill_cycles, tagLeaf=llc_tag_leaf, dataLeaf=llc_data_leaf))
  //val llc = Module(new DRAMSideLLCNull(NL2_REL_XACTS+NL2_ACQ_XACTS, refill_cycles))
  val mem_serdes = Module(new MemSerdes(htif_width))

  val masterEndpoints = (0 until ln.nMasters).map(i => Module(new L2CoherenceAgent(i)))
  val net = Module(new ReferenceChipCrossbarNetwork)
  net.io.clients zip (io.tiles :+ io.htif) map { case (net, end) => net <> end }
  net.io.masters zip (masterEndpoints.map(_.io.client)) map { case (net, end) => net <> end }
  masterEndpoints.map{ _.io.incoherent zip io.incoherent map { case (m, c) => m := c } }

  val conv = Module(new MemIOUncachedTileLinkIOConverter(2))
  if(ln.nMasters > 1) {
    val arb = Module(new UncachedTileLinkIOArbiterThatAppendsArbiterId(ln.nMasters))
    arb.io.in zip masterEndpoints.map(_.io.master) map { case (arb, cache) => arb <> cache }
    conv.io.uncached <> arb.io.out
  } else {
    conv.io.uncached <> masterEndpoints.head.io.master
  }
  llc.io.cpu.req_cmd <> Queue(conv.io.mem.req_cmd)
  llc.io.cpu.req_data <> Queue(conv.io.mem.req_data, refill_cycles)
  conv.io.mem.resp <> llc.io.cpu.resp

  // mux between main and backup memory ports
  val mem_cmdq = Module(new Queue(new MemReqCmd, 2))
  mem_cmdq.io.enq <> llc.io.mem.req_cmd
  mem_cmdq.io.deq.ready := Mux(io.mem_backup_en, mem_serdes.io.wide.req_cmd.ready, io.mem.req_cmd.ready)
  io.mem.req_cmd.valid := mem_cmdq.io.deq.valid && !io.mem_backup_en
  io.mem.req_cmd.bits := mem_cmdq.io.deq.bits
  mem_serdes.io.wide.req_cmd.valid := mem_cmdq.io.deq.valid && io.mem_backup_en
  mem_serdes.io.wide.req_cmd.bits := mem_cmdq.io.deq.bits

  val mem_dataq = Module(new Queue(new MemData, refill_cycles))
  mem_dataq.io.enq <> llc.io.mem.req_data
  mem_dataq.io.deq.ready := Mux(io.mem_backup_en, mem_serdes.io.wide.req_data.ready, io.mem.req_data.ready)
  io.mem.req_data.valid := mem_dataq.io.deq.valid && !io.mem_backup_en
  io.mem.req_data.bits := mem_dataq.io.deq.bits
  mem_serdes.io.wide.req_data.valid := mem_dataq.io.deq.valid && io.mem_backup_en
  mem_serdes.io.wide.req_data.bits := mem_dataq.io.deq.bits

  llc.io.mem.resp.valid := Mux(io.mem_backup_en, mem_serdes.io.wide.resp.valid, io.mem.resp.valid)
  io.mem.resp.ready := Bool(true)
  llc.io.mem.resp.bits := Mux(io.mem_backup_en, mem_serdes.io.wide.resp.bits, io.mem.resp.bits)

  io.mem_backup <> mem_serdes.io.narrow
}

case class UncoreConfiguration(l2: L2CoherenceAgentConfiguration, tl: TileLinkConfiguration, mif: MemoryIFConfiguration, nTiles: Int, nBanks: Int, bankIdLsb: Int, nSCR: Int, offsetBits: Int)

class Uncore(htif_width: Int)(implicit conf: UncoreConfiguration) extends Module
{
  implicit val (tl, mif) = (conf.tl, conf.mif)
  val io = new Bundle {
    val host = new HostIO(htif_width)
    val mem = new MemIO
    val tiles = Vec.fill(conf.nTiles){new TileLinkIO}.flip
    val htif = Vec.fill(conf.nTiles){new HTIFIO(conf.nTiles)}.flip
    val incoherent = Vec.fill(conf.nTiles){Bool()}.asInput
    val mem_backup = new MemSerializedIO(htif_width)
    val mem_backup_en = Bool(INPUT)
  }
  val htif = Module(new HTIF(htif_width, CSRs.reset, conf.nSCR, conf.offsetBits))
  val outmemsys = Module(new OuterMemorySystem(htif_width))
  val incoherentWithHtif = (io.incoherent :+ Bool(true).asInput)
  outmemsys.io.incoherent := incoherentWithHtif
  htif.io.cpu <> io.htif
  outmemsys.io.mem <> io.mem
  outmemsys.io.mem_backup_en <> io.mem_backup_en

  // Add networking headers and endpoint queues
  def convertAddrToBank(addr: Bits): UInt = {
    require(conf.bankIdLsb + log2Up(conf.nBanks) < conf.mif.addrBits, {println("Invalid bits for bank multiplexing.")})
    addr(conf.bankIdLsb + log2Up(conf.nBanks) - 1, conf.bankIdLsb)
  }

  (outmemsys.io.tiles :+ outmemsys.io.htif).zip(io.tiles :+ htif.io.mem).zipWithIndex.map { 
    case ((outer, client), i) => 
      outer.acquire <> Queue(TileLinkHeaderOverwriter(client.acquire, i, conf.nBanks, convertAddrToBank _))
      outer.release <> Queue(TileLinkHeaderOverwriter(client.release, i, conf.nBanks, convertAddrToBank _))
      outer.grant_ack <> Queue(TileLinkHeaderOverwriter(client.grant_ack, i))
      client.grant <> Queue(outer.grant, 1, pipe = true)
      client.probe <> Queue(outer.probe)
  }

  // pad out the HTIF using a divided clock
  val hio = Module((new SlowIO(512)) { Bits(width = htif_width+1) })
  hio.io.set_divisor.valid := htif.io.scr.wen && htif.io.scr.waddr === 63
  hio.io.set_divisor.bits := htif.io.scr.wdata
  htif.io.scr.rdata(63) := hio.io.divisor

  hio.io.out_fast.valid := htif.io.host.out.valid || outmemsys.io.mem_backup.req.valid
  hio.io.out_fast.bits := Cat(htif.io.host.out.valid, Mux(htif.io.host.out.valid, htif.io.host.out.bits, outmemsys.io.mem_backup.req.bits))
  htif.io.host.out.ready := hio.io.out_fast.ready
  outmemsys.io.mem_backup.req.ready := hio.io.out_fast.ready && !htif.io.host.out.valid
  io.host.out.valid := hio.io.out_slow.valid && hio.io.out_slow.bits(htif_width)
  io.host.out.bits := hio.io.out_slow.bits
  io.mem_backup.req.valid := hio.io.out_slow.valid && !hio.io.out_slow.bits(htif_width)
  hio.io.out_slow.ready := Mux(hio.io.out_slow.bits(htif_width), io.host.out.ready, io.mem_backup.req.ready)

  val mem_backup_resp_valid = io.mem_backup_en && io.mem_backup.resp.valid
  hio.io.in_slow.valid := mem_backup_resp_valid || io.host.in.valid
  hio.io.in_slow.bits := Cat(mem_backup_resp_valid, io.host.in.bits)
  io.host.in.ready := hio.io.in_slow.ready
  outmemsys.io.mem_backup.resp.valid := hio.io.in_fast.valid && hio.io.in_fast.bits(htif_width)
  outmemsys.io.mem_backup.resp.bits := hio.io.in_fast.bits
  htif.io.host.in.valid := hio.io.in_fast.valid && !hio.io.in_fast.bits(htif_width)
  htif.io.host.in.bits := hio.io.in_fast.bits
  hio.io.in_fast.ready := Mux(hio.io.in_fast.bits(htif_width), Bool(true), htif.io.host.in.ready)
  io.host.clk := hio.io.clk_slow
  io.host.clk_edge := Reg(next=io.host.clk && !Reg(next=io.host.clk))

  io.host.debug_stats_pcr := htif.io.host.debug_stats_pcr
}

class TopIO(htifWidth: Int)(implicit conf: MemoryIFConfiguration) extends Bundle  {
  val host    = new HostIO(htifWidth)
  val mem     = new MemIO
  val debug   = Vec.fill(NTILES) { new DpathDebugIo }
}

class VLSITopIO(htifWidth: Int)(implicit conf: MemoryIFConfiguration) extends TopIO(htifWidth)(conf) {
  val mem_backup_en = Bool(INPUT)
  val in_mem_ready = Bool(OUTPUT)
  val in_mem_valid = Bool(INPUT)
  val out_mem_ready = Bool(INPUT)
  val out_mem_valid = Bool(OUTPUT)
}


class MemDessert extends Module {
  implicit val mif = MemoryIFConfiguration(MEM_ADDR_BITS, MEM_DATA_BITS, MEM_TAG_BITS, MEM_DATA_BEATS)
  val io = new MemDesserIO(HTIF_WIDTH)
  val x = Module(new MemDesser(HTIF_WIDTH))
  io.narrow <> x.io.narrow
  io.wide <> x.io.wide
}


class Top extends Module {
  val co = if(ENABLE_SHARING) {
              if(ENABLE_CLEAN_EXCLUSIVE) new MESICoherence
              else new MSICoherence
            } else {
              if(ENABLE_CLEAN_EXCLUSIVE) new MEICoherence
              else new MICoherence
            }

  implicit val ln = LogicalNetworkConfiguration(log2Up(NTILES)+1, NBANKS, NTILES+1)
  implicit val as = AddressSpaceConfiguration(PADDR_BITS, VADDR_BITS, PGIDX_BITS, ASID_BITS, PERM_BITS)
  implicit val tl = TileLinkConfiguration(co = co, ln = ln, 
                                          addrBits = as.paddrBits-OFFSET_BITS, 
                                          clientXactIdBits = log2Up(NL2_REL_XACTS+NL2_ACQ_XACTS), 
                                          masterXactIdBits = 2*log2Up(NMSHRS*NTILES+1), 
                                          dataBits = CACHE_DATA_SIZE_IN_BYTES*8, 
                                          writeMaskBits = WRITE_MASK_BITS, 
                                          wordAddrBits = SUBWORD_ADDR_BITS, 
                                          atomicOpBits = ATOMIC_OP_BITS)
  implicit val l2 = L2CoherenceAgentConfiguration(tl, NL2_REL_XACTS, NL2_ACQ_XACTS)
  implicit val mif = MemoryIFConfiguration(MEM_ADDR_BITS, MEM_DATA_BITS, MEM_TAG_BITS, MEM_DATA_BEATS)
  implicit val uc = UncoreConfiguration(l2, tl, mif, NTILES, NBANKS, bankIdLsb = 5, nSCR = 64, offsetBits = OFFSET_BITS)

  val io = new VLSITopIO(HTIF_WIDTH)
  val resetSigs = Vec.fill(uc.nTiles){Bool()}

  // Rocket Tile --------------------

//  val ic = ICacheConfig(128, 2, ntlb = 8, nbtb = 38)
//  val dc = DCacheConfig(128, 4, ntlb = 8,
//                        nmshr = NMSHRS, nrpq = 16, nsdq = 17, states = co.nClientStates)
//  val vic = ICacheConfig(sets = 128, assoc = 1, tl = tl, as = as, btb = BTBConfig(as, 8))
//  val hc = hwacha.HwachaConfiguration(as, vic, dc, 8, 256, ndtlb = 8, nptlb = 2)
//  val fpu = if (HAS_FPU) Some(FPUConfig(sfmaLatency = 2, dfmaLatency = 3)) else None
//  val rc = RocketConfiguration(tl, as, ic, dc, fpu
//  //                             rocc = (c: RocketConfiguration) => (new hwacha.Hwacha(hc, c))
//                              )
//
//  val tileList = (0 until uc.nTiles).map(r => Module(new Tile(resetSignal = resetSigs(r))(rc)))

  // BOOM Tile --------------------

  val ic = ICacheConfig(sets = 128, assoc = 2, ntlb = 8, tl = tl, as = as, btb = BTBConfig(as, 64))
  val dc = DCacheConfig(sets = 128, ways = 4, 
                        tl = tl, as = as,
                        ntlb = 8, nmshr = NMSHRS, nrpq = 16, nsdq = 17, 
                        reqtagbits = -1, databits = -1)
  val fpu = if (HAS_FPU) Some(FPUConfig(sfmaLatency = 2, dfmaLatency = 3)) else None
  val rc = RocketConfiguration(tl, as, ic, dc, fpu, retireWidth = 2) // TODO retire width could be changed by BOOM, how do we get that information up here?
  val iwc = BOOM.IssueConfig()
  implicit val bc = BOOM.BOOMConfiguration (rc, iwc)
  val tileList = (0 until uc.nTiles).map(r => Module(new BOOM.BoomTile(resetSignal = resetSigs(r))(bc)))

  // ------------------------------

  val uncore = Module(new Uncore(HTIF_WIDTH))

  for (i <- 0 until uc.nTiles) {
    val hl = uncore.io.htif(i)
    val tl = uncore.io.tiles(i)
    val il = uncore.io.incoherent(i)

    resetSigs(i) := hl.reset
    val tile = tileList(i)
    tile.io.tilelink <> tl
    tile.io.debug <> io.debug(i)
    il := hl.reset
    tile.io.host.reset := Reg(next=Reg(next=hl.reset))
    tile.io.host.pcr_req <> Queue(hl.pcr_req, 1)
    tile.io.host.id := i
    hl.pcr_rep <> Queue(tile.io.host.pcr_rep, 1)
    hl.ipi_req <> Queue(tile.io.host.ipi_req, 1)
    tile.io.host.ipi_rep <> Queue(hl.ipi_rep, 1)
    hl.debug_stats_pcr := tile.io.host.debug_stats_pcr
  }

  io.host <> uncore.io.host

  uncore.io.mem_backup.resp.valid := io.in_mem_valid

  io.out_mem_valid := uncore.io.mem_backup.req.valid
  uncore.io.mem_backup.req.ready := io.out_mem_ready

  io.mem_backup_en <> uncore.io.mem_backup_en
  io.mem <> uncore.io.mem
}


