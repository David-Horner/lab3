//**************************************************************************
// Multi-threaded Matrix Multiply benchmark
//--------------------------------------------------------------------------
// TA     : Christopher Celio
// Student: 
//
//
// This benchmark multiplies two 2-D arrays together and writes the results to
// a third vector. The input data (and reference data) should be generated
// using the matmul_gendata.pl perl script and dumped to a file named
// dataset.h. 


// print out arrays, etc.
//#define DEBUG

//--------------------------------------------------------------------------
// Includes 

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


//--------------------------------------------------------------------------
// Input/Reference Data

typedef float data_t;
#include "dataset.h"
 
  
//--------------------------------------------------------------------------
// Basic Utilities and Multi-thread Support

__thread unsigned long coreid;
unsigned long ncores;

#include "util.h"
   
#define stringify_1(s) #s
#define stringify(s) stringify_1(s)
#define stats(code) do { \
    unsigned long _c = -rdcycle(), _i = -rdinstret(); \
    code; \
    _c += rdcycle(), _i += rdinstret(); \
    if (coreid == 0) \
      printf("%s: %ld cycles, %ld.%ld cycles/iter, %ld.%ld CPI\n", \
             stringify(code), _c, _c/DIM_SIZE/DIM_SIZE/DIM_SIZE, 10*_c/DIM_SIZE/DIM_SIZE/DIM_SIZE%10, _c/_i, 10*_c/_i%10); \
  } while(0)
 

//--------------------------------------------------------------------------
// Helper functions
    
void printArray( char name[], int n, data_t arr[] )
{
   int i;
   if (coreid != 0)
      return;
  
   printf( " %10s :", name );
   for ( i = 0; i < n; i++ )
      printf( " %3ld ", (long) arr[i] );
   printf( "\n" );
}
      
void __attribute__((noinline)) verify(size_t n, const data_t* test, const data_t* correct)
{
   if (coreid != 0)
      return;

   size_t i;
   for (i = 0; i < n; i++)
   {
      if (test[i] != correct[i])
      {
         printf("FAILED test[%d]= %3ld, correct[%d]= %3ld\n", 
            i, (long)test[i], i, (long)correct[i]);
         exit(-1);
      }
   }
   
   return;
}
 
//--------------------------------------------------------------------------
// matmul function
 
// single-thread, naive version
void __attribute__((noinline)) matmul_naive(const int lda,  const data_t A[], const data_t B[], data_t C[] )
{
   int i, j, k;

   if (coreid > 0)
      return;
  
   for ( i = 0; i < lda; i++ )
      for ( j = 0; j < lda; j++ )  
      {
         for ( k = 0; k < lda; k++ ) 
         {
            C[i + j*lda] += A[j*lda + k] * B[k*lda + i];
         }
      }

}
 


void __attribute__((noinline)) matmul(const int lda,  const data_t A[], const data_t B[], data_t C[] )
{
   
   // ***************************** //
   // **** ADD YOUR CODE HERE ***** //
   // ***************************** //
   //
   // feel free to make a separate function for MI and MSI versions.

   int i, j, k, n, m;


   //matmul_naive(32, input1_data, input2_data, results_data); barrier(): 957424 cycles, 29.2 cycles/iter, 3.6 CPI
   //matmul(32, input1_data, input2_data, results_data); barrier(): 340408 cycles, 10.3 cycles/iter, 1.8 CPI

   for (n = 0; n < lda; n += 1) {
      for (m = 0; m < lda; m += 1) {
         bTranspose[lda*m + n] = B[lda*n + m];
         bTranspose[lda*n + m] = B[lda*m + n];
      }
   }
   barrier();

   for ( j = coreid; j < lda; j += 2*ncores ) {
      for ( i = 0; i < lda; i += 1 ){
         c1 = 0;     //global vars c1, c2
         c2 = 0;
         for ( k = 0; k < lda; k += 1 ) {
            c1 += A[j * lda + k] * bTranspose[i*lda + k];
            c2 += A[(j+2) * lda + k] * bTranspose[i*lda + k];
            
            //barrier();
         }

         C[i + j * lda] = c1;
         C[i + (j+2) * lda] = c2;
         barrier();
      }
   //barrier();
   }




   //matmul_naive(32, input1_data, input2_data, results_data); barrier(): 983609 cycles, 30.0 cycles/iter, 3.7 CPI
   //matmul(32, input1_data, input2_data, results_data); barrier(): 389942 cycles, 11.9 cycles/iter, 2.5 CPI

   /*
   for ( j = coreid; j < lda; j += 2*ncores ) {
      for ( i = 0; i < lda; i += 1 ){
         c1 = 0;     //global vars c1, c2
         c2 = 0;
         for ( k = 0; k < lda; k += 1 ) {
            c1 += A[j * lda + k] * B[k*lda + i];
            c2 += A[(j+2) * lda + k] * B[k*lda + i];
            
            //barrier();
         }

         C[i + j * lda] = c1;
         C[i + (j+2) * lda] = c2;
         barrier();
      }
   //barrier();
   }
   */

   // matmul_naive(32, input1_data, input2_data, results_data); barrier(): 973781 cycles, 29.7 cycles/iter, 3.7 CPI
   // matmul(32, input1_data, input2_data, results_data); barrier(): 461066 cycles, 14.0 cycles/iter, 3.5 CPI
   // for ( k = 0; k < lda; k += 1 ) {
   //    for ( j = coreid; j < lda; j += 2*ncores ) {      
   //       for ( i = 0; i < lda; i += 1 ){
   //          C[i + j * lda] += A[j * lda + k] * B[k*lda + i];
   //          C[i + (j+2) * lda] += A[(j+2) * lda + k] * B[k*lda + i];      
   //          //barrier();
   //       }
   //       barrier();
   //    }
   // //barrier();
   // }
   

   // matmul_naive(32, input1_data, input2_data, results_data); barrier(): 965136 cycles, 29.4 cycles/iter, 3.7 CPI
   // matmul(32, input1_data, input2_data, results_data); barrier(): 513779 cycles, 15.6 cycles/iter, 3.2 CPI

   // for ( j = coreid; j < lda; j += 2*ncores ) {
   //    for ( i = 0; i < lda; i += 1 ){
   //       for ( k = 0; k < lda; k += 1 ) {
   //          C[i + j * lda] += A[j * lda + k] * B[k*lda + i];
   //          C[i + (j+2) * lda] += A[(j+2) * lda + k] * B[k*lda + i];
            
   //          //barrier();
   //       }
   //       barrier();
   //    }
   // //barrier();
   //}


   // matmul_naive(32, input1_data, input2_data, results_data); barrier(): 937892 cycles, 28.6 cycles/iter, 3.6 CPI
   // matmul(32, input1_data, input2_data, results_data); barrier(): 576478 cycles, 17.5 cycles/iter, 3.5 CPI

   // for ( i = 0; i < lda; i += 1 ){
   //    for ( j = coreid; j < lda; j += 2*ncores ) {
   //       for ( k = 0; k < lda; k += 1 ) {
   //          C[i + j * lda] += A[j * lda + k] * B[k*lda + i];
   //          C[i + (j+2) * lda] += A[(j+2) * lda + k] * B[k*lda + i];
            
   //          //barrier();
   //       }
   //       barrier();
   //    }
   //    //barrier();
   // }

   //for ( i = coreid; i < lda; i += ncores ){
   //   for ( j = coreid; j < lda; j += ncores ) {
   //      for ( k = coreid; k < lda; k += ncores ) {
   //         C[i + j*lda] += A[j*lda + k] * B[k*lda + i];
   //      }
         //barrier();
   //   }
   //}
}

//--------------------------------------------------------------------------
// Main
//
// all threads start executing thread_entry(). Use their "coreid" to
// differentiate between threads (each thread is running on a separate core).
  
void thread_entry(int cid, int nc)
{
   coreid = cid;
   ncores = nc;

   // static allocates data in the binary, which is visible to both threads
   static data_t results_data[ARRAY_SIZE];


//   // Execute the provided, naive matmul
//   barrier();
//   stats(matmul_naive(DIM_SIZE, input1_data, input2_data, results_data); barrier());
// 
//   
//   // verify
//   verify(ARRAY_SIZE, results_data, verify_data);
//   
//   // clear results from the first trial
//   size_t i;
//   if (coreid == 0) 
//      for (i=0; i < ARRAY_SIZE; i++)
//         results_data[i] = 0;
//   barrier();

   
   // Execute your faster matmul
   barrier();
   stats(matmul(DIM_SIZE, input1_data, input2_data, results_data); barrier());
 
#ifdef DEBUG
   printArray("results:", ARRAY_SIZE, results_data);
   printArray("verify :", ARRAY_SIZE, verify_data);
#endif
   
   // verify
   verify(ARRAY_SIZE, results_data, verify_data);
   barrier();

   exit(0);
}

