
SOURCE_DIR = ../../source
SOFTFLOAT_DIR = ../../../SoftFloat-3
PLATFORM_TYPE = 8086
PLATFORM = Darwin

SOFTFLOAT_H = \
  $(SOFTFLOAT_DIR)/source/$(PLATFORM_TYPE)/softfloat_types.h \
  $(SOFTFLOAT_DIR)/source/softfloat.h
SOFTFLOAT_LIB = $(SOFTFLOAT_DIR)/build/$(PLATFORM)/softfloat$(LIB)

C_INCLUDES = \
  -I. -I$(SOURCE_DIR) -I$(SOFTFLOAT_DIR)/source/$(PLATFORM_TYPE) \
  -I$(SOFTFLOAT_DIR)/source
#COMPILE_ASM = gcc -c -o $@
COMPILE_C = \
  gcc -c -Werror-implicit-function-declaration "-DINLINE=extern inline" \
    $(C_INCLUDES) -iquote -O2 -o $@
COMPILE_SLOWFLOAT_C = \
  gcc -c -Werror-implicit-function-declaration $(C_INCLUDES) -iquote -O3 -o $@
MAKELIB = ar crs $@
LINK = gcc -o $@
DELETE = rm -f

OBJ = .o
LIB = .a
EXE =

.PHONY: all
all: \
  testsoftfloat$(EXE) \
  timesoftfloat$(EXE) \
  testfloat_gen$(EXE) \
  testfloat_ver$(EXE) \
  testfloat$(EXE) \

OBJS_GENCASES = \
  genCases_ui32$(OBJ) \
  genCases_ui64$(OBJ) \
  genCases_i32$(OBJ) \
  genCases_i64$(OBJ) \
  genCases_f32$(OBJ) \
  genCases_f64$(OBJ) \

OBJS_WRITECASE = \
  writeCase_a_ui32$(OBJ) \
  writeCase_a_ui64$(OBJ) \
  writeCase_a_f32$(OBJ) \
  writeCase_ab_f32$(OBJ) \
  writeCase_abc_f32$(OBJ) \
  writeCase_a_f64$(OBJ) \
  writeCase_ab_f64$(OBJ) \
  writeCase_abc_f64$(OBJ) \
  writeCase_z_bool$(OBJ) \
  writeCase_z_ui32$(OBJ) \
  writeCase_z_ui64$(OBJ) \
  writeCase_z_f32$(OBJ) \
  writeCase_z_f64$(OBJ) \

OBJS_TEST = \
  test_a_ui32_z_f32$(OBJ) \
  test_a_ui32_z_f64$(OBJ) \
  test_a_ui64_z_f32$(OBJ) \
  test_a_ui64_z_f64$(OBJ) \
  test_a_i32_z_f32$(OBJ) \
  test_a_i32_z_f64$(OBJ) \
  test_a_i64_z_f32$(OBJ) \
  test_a_i64_z_f64$(OBJ) \
  test_a_f32_z_ui32_rx$(OBJ) \
  test_a_f32_z_ui64_rx$(OBJ) \
  test_a_f32_z_i32_rx$(OBJ) \
  test_a_f32_z_i64_rx$(OBJ) \
  test_a_f32_z_ui32_x$(OBJ) \
  test_a_f32_z_ui64_x$(OBJ) \
  test_a_f32_z_i32_x$(OBJ) \
  test_a_f32_z_i64_x$(OBJ) \
  test_a_f32_z_f64$(OBJ) \
  test_az_f32$(OBJ) \
  test_az_f32_rx$(OBJ) \
  test_abz_f32$(OBJ) \
  test_abcz_f32$(OBJ) \
  test_ab_f32_z_bool$(OBJ) \
  test_a_f64_z_ui32_rx$(OBJ) \
  test_a_f64_z_ui64_rx$(OBJ) \
  test_a_f64_z_i32_rx$(OBJ) \
  test_a_f64_z_i64_rx$(OBJ) \
  test_a_f64_z_ui32_x$(OBJ) \
  test_a_f64_z_ui64_x$(OBJ) \
  test_a_f64_z_i32_x$(OBJ) \
  test_a_f64_z_i64_x$(OBJ) \
  test_a_f64_z_f32$(OBJ) \
  test_az_f64$(OBJ) \
  test_az_f64_rx$(OBJ) \
  test_abz_f64$(OBJ) \
  test_abcz_f64$(OBJ) \
  test_ab_f64_z_bool$(OBJ) \

OBJS_LIB = \
  fail$(OBJ) \
  functions_common$(OBJ) \
  functionInfos$(OBJ) \
  standardFunctionInfos$(OBJ) \
  random$(OBJ) \
  genCases_common$(OBJ) \
  $(OBJS_GENCASES) \
  genCases_writeTestsTotal$(OBJ) \
  verCases_common$(OBJ) \
  verCases_writeFunctionName$(OBJ) \
  readHex$(OBJ) \
  writeHex$(OBJ) \
  writeCase_common$(OBJ) \
  $(OBJS_WRITECASE) \
  testLoops_common$(OBJ) \
  $(OBJS_TEST) \

fail$(OBJ): $(SOURCE_DIR)/fail.h
functions_common$(OBJ): $(SOFTFLOAT_H) $(SOURCE_DIR)/functions.h
functionInfos$(OBJ): $(SOURCE_DIR)/functions.h
standardFunctionInfos$(OBJ): $(SOURCE_DIR)/functions.h
random$(OBJ): $(SOURCE_DIR)/random.h
genCases_common$(OBJ): $(SOURCE_DIR)/fail.h $(SOURCE_DIR)/genCases.h
$(OBJS_GENCASES): \
  $(SOURCE_DIR)/random.h $(SOFTFLOAT_H) $(SOURCE_DIR)/genCases.h
genCases_writeTestsTotal$(OBJ): $(SOURCE_DIR)/genCases.h
verCases_common$(OBJ): $(SOURCE_DIR)/verCases.h
verCases_writeFunctionName$(OBJ): $(SOURCE_DIR)/verCases.h
readHex$(OBJ): $(SOURCE_DIR)/readHex.h
writeHex$(OBJ): $(SOFTFLOAT_H) $(SOURCE_DIR)/writeHex.h
writeCase_common$(OBJ): $(SOURCE_DIR)/writeCase.h
$(OBJS_WRITECASE): \
  $(SOFTFLOAT_H) $(SOURCE_DIR)/writeHex.h $(SOURCE_DIR)/writeCase.h
testLoops_common$(OBJ): $(SOURCE_DIR)/testLoops.h
$(OBJS_TEST): \
  $(SOFTFLOAT_H) $(SOURCE_DIR)/genCases.h $(SOURCE_DIR)/verCases.h \
  $(SOURCE_DIR)/writeCase.h $(SOURCE_DIR)/testLoops.h
$(OBJS_LIB): %.o: $(SOURCE_DIR)/%.c
	$(COMPILE_C) $(SOURCE_DIR)/$*.c
testfloat$(LIB): $(OBJS_LIB)
	$(MAKELIB) $^

OBJS_TESTSOFTFLOAT = slowfloat$(OBJ) testsoftfloat$(OBJ) writeCase_common$(OBJ)

slowfloat$(OBJ): \
  $(SOFTFLOAT_H) $(SOURCE_DIR)/slowfloat.h $(SOURCE_DIR)/slowfloat.c
	$(COMPILE_SLOWFLOAT_C) $(SOURCE_DIR)/slowfloat.c
testsoftfloat$(OBJ): \
  $(SOURCE_DIR)/fail.h $(SOFTFLOAT_H) $(SOURCE_DIR)/slowfloat.h \
  $(SOURCE_DIR)/functions.h $(SOURCE_DIR)/genCases.h $(SOURCE_DIR)/verCases.h \
  $(SOURCE_DIR)/writeCase.h $(SOURCE_DIR)/testLoops.h \
  $(SOURCE_DIR)/testsoftfloat.c
	$(COMPILE_C) $(SOURCE_DIR)/testsoftfloat.c

testsoftfloat$(EXE): $(OBJS_TESTSOFTFLOAT) testfloat$(LIB) $(SOFTFLOAT_LIB)
	$(LINK) $^

OBJS_TIMESOFTFLOAT = timesoftfloat$(OBJ) writeCase_common$(OBJ)

timesoftfloat$(OBJ): \
  $(SOURCE_DIR)/fail.h $(SOFTFLOAT_H) $(SOURCE_DIR)/functions.h \
  $(SOURCE_DIR)/timesoftfloat.c
	$(COMPILE_C) $(SOURCE_DIR)/timesoftfloat.c

timesoftfloat$(EXE): $(OBJS_TIMESOFTFLOAT) testfloat$(LIB) $(SOFTFLOAT_LIB)
	$(LINK) $^

OBJS_TESTFLOAT_GEN = genLoops$(OBJ) testfloat_gen$(OBJ) writeCase_common$(OBJ)

genLoops$(OBJ): \
  $(SOURCE_DIR)/fail.h $(SOFTFLOAT_H) $(SOURCE_DIR)/genCases.h \
  $(SOURCE_DIR)/writeHex.h $(SOURCE_DIR)/genLoops.h $(SOURCE_DIR)/genLoops.c
	$(COMPILE_C) $(SOURCE_DIR)/genLoops.c
testfloat_gen$(OBJ): \
  $(SOURCE_DIR)/fail.h $(SOFTFLOAT_H) $(SOURCE_DIR)/functions.h \
  $(SOURCE_DIR)/genCases.h $(SOURCE_DIR)/genLoops.h \
  $(SOURCE_DIR)/testfloat_gen.c
	$(COMPILE_C) $(SOURCE_DIR)/testfloat_gen.c

testfloat_gen$(EXE): $(OBJS_TESTFLOAT_GEN) testfloat$(LIB) $(SOFTFLOAT_LIB)
	$(LINK) $^

OBJS_TESTFLOAT_VER = verLoops$(OBJ) testfloat_ver$(OBJ) writeCase_common$(OBJ)

verLoops$(OBJ): \
  $(SOURCE_DIR)/fail.h $(SOFTFLOAT_H) $(SOURCE_DIR)/readHex.h \
  $(SOURCE_DIR)/verCases.h $(SOURCE_DIR)/writeCase.h $(SOURCE_DIR)/verLoops.h \
  $(SOURCE_DIR)/verLoops.c
	$(COMPILE_C) $(SOURCE_DIR)/verLoops.c
testfloat_ver$(OBJ): \
  $(SOURCE_DIR)/fail.h $(SOFTFLOAT_H) $(SOURCE_DIR)/functions.h \
  $(SOURCE_DIR)/verCases.h $(SOURCE_DIR)/writeCase.h $(SOURCE_DIR)/verLoops.h \
  $(SOURCE_DIR)/testfloat_ver.c
	$(COMPILE_C) $(SOURCE_DIR)/testfloat_ver.c

testfloat_ver$(EXE): $(OBJS_TESTFLOAT_VER) testfloat$(LIB) $(SOFTFLOAT_LIB)
	$(LINK) $^

OBJS_TESTFLOAT = systfloat$(OBJ) systfloat_functions$(OBJ) testfloat$(OBJ) writeCase_common$(OBJ)

systfloat$(OBJ): \
  $(SOFTFLOAT_H) $(SOURCE_DIR)/systfloat_config.h $(SOURCE_DIR)/systfloat.h \
  $(SOURCE_DIR)/systfloat.c
	$(COMPILE_C) $(SOURCE_DIR)/systfloat.c
systfloat_functions$(OBJ): \
  $(SOURCE_DIR)/systfloat_config.h $(SOURCE_DIR)/systfloat.h \
  $(SOURCE_DIR)/functions.h $(SOURCE_DIR)/systfloat_functions.c
	$(COMPILE_C) $(SOURCE_DIR)/systfloat_functions.c
testfloat$(OBJ): \
  $(SOURCE_DIR)/fail.h $(SOFTFLOAT_H) $(SOURCE_DIR)/systfloat_config.h \
  $(SOURCE_DIR)/systfloat.h $(SOURCE_DIR)/functions.h \
  $(SOURCE_DIR)/genCases.h $(SOURCE_DIR)/verCases.h $(SOURCE_DIR)/writeCase.h \
  $(SOURCE_DIR)/testLoops.h $(SOURCE_DIR)/testfloat.c
	$(COMPILE_C) $(SOURCE_DIR)/testfloat.c

testfloat$(EXE): $(OBJS_TESTFLOAT) testfloat$(LIB) $(SOFTFLOAT_LIB)
	$(LINK) $^ -lm


#*** OLD
#
#systmodes$(OBJ): milieu.h ../systmodes.h systmodes.c
#	$(COMPILE_C) systmodes.c
#
#systflags$(OBJ): milieu.h ../systflags.h systflags.c
#	$(COMPILE_C) systflags.c
#
#systfloat$(OBJ): systfloat.S
#	$(COMPILE_ASM) systfloat.S
#
#


.PHONY: clean
clean:
	$(DELETE) $(OBJS_LIB) testfloat$(LIB)
	$(DELETE) $(OBJS_TESTSOFTFLOAT) testsoftfloat$(EXE)
	$(DELETE) $(OBJS_TIMESOFTFLOAT) timesoftfloat$(EXE)
	$(DELETE) $(OBJS_TESTFLOAT_GEN) testfloat_gen$(EXE)
	$(DELETE) $(OBJS_TESTFLOAT_VER) testfloat_ver$(EXE)
	$(DELETE) $(OBJS_TESTFLOAT) testfloat$(EXE)

