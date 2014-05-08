
/*
===============================================================================

This C source file is part of TestFloat, Release 2a, a package of programs
for testing the correctness of floating-point arithmetic complying to the
IEC/IEEE Standard for Floating-Point.

Written by John R. Hauser.  More information is available through the Web
page `http://HTTP.CS.Berkeley.EDU/~jhauser/arithmetic/TestFloat.html'.

THIS SOFTWARE IS DISTRIBUTED AS IS, FOR FREE.  Although reasonable effort
has been made to avoid it, THIS SOFTWARE MAY CONTAIN FAULTS THAT WILL AT
TIMES RESULT IN INCORRECT BEHAVIOR.  USE OF THIS SOFTWARE IS RESTRICTED TO
PERSONS AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL RESPONSIBILITY FOR ANY
AND ALL LOSSES, COSTS, OR OTHER PROBLEMS ARISING FROM ITS USE.

Derivative works are acceptable, even for commercial purposes, so long as
(1) they include prominent notice that the work is derivative, and (2) they
include prominent notice akin to these four paragraphs for those parts of
this code that are retained.

===============================================================================
*/

#include <fenv.h>
#include <stdio.h>
#include "softfloat.h"
#include "genCases.h"
#include "verCases.h"
#include "writeCase.h"
#include "testLoops.h"

#pragma STDC FENV_ACCESS ON

void
 test_abz_f64(
     float64_t trueFunction( float64_t, float64_t ),
     float64_t testFunction( float64_t, float64_t )
 )
{
    int count;
    float64_t trueZ;
    int_fast8_t trueFlags;
    float64_t testZ;
    int_fast8_t testFlags;

    genCases_f64_ab_init();
    genCases_writeTestsTotal( testLoops_forever );
    verCases_errorCount = 0;
    verCases_tenThousandsCount = 0;
    count = 10000;
    while ( ! genCases_done || testLoops_forever ) {
        genCases_f64_ab_next();
        *testLoops_trueFlagsPtr = 0;
        trueZ = trueFunction( genCases_f64_a, genCases_f64_b );
        trueFlags = *testLoops_trueFlagsPtr;
        testLoops_testFlagsFunction();
        testZ = testFunction( genCases_f64_a, genCases_f64_b );
        testFlags = testLoops_testFlagsFunction();
        --count;
        if ( ! count ) {
            verCases_perTenThousand();
            count = 10000;
        }
        if ( ! f64_same( trueZ, testZ ) || ( trueFlags != testFlags ) ) {
            if (
                   ! verCases_checkNaNs
                && (    f64_isSignalingNaN( genCases_f64_a )
                     || f64_isSignalingNaN( genCases_f64_b ) )
            ) {
                trueFlags |= softfloat_flag_invalid;
            }
            if (
                   verCases_checkNaNs
                || ! f64_isNaN( trueZ )
                || ! f64_isNaN( testZ )
                || f64_isSignalingNaN( testZ )
                || ( trueFlags != testFlags )
            ) {
                ++verCases_errorCount;
                verCases_writeErrorFound( 10000 - count );
                writeCase_ab_f64( genCases_f64_a, genCases_f64_b, "\n\t" );
                writeCase_z_f64( trueZ, trueFlags, testZ, testFlags );
                if ( verCases_errorCount == verCases_maxErrorCount ) break;
            }
        }
    }
    verCases_writeTestsPerformed( 10000 - count );

}
