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
 


void __attribute__((noinline)) matmul_msi(const int lda,  const data_t A[], const data_t B[], data_t C[] ) {
  int i, j, k;

  for (i = 0; i < lda; i += 2) {
    for (j = coreid * (lda / ncores); j < (coreid + 1) * (lda / ncores); j += 4) {
    //for (j = 0; j < lda; j += 4) {
      register data_t c00 = 0, c01 = 0;
      register data_t c10 = 0, c11 = 0;
      register data_t c20 = 0, c21 = 0;
      register data_t c30 = 0, c31 = 0;

      register data_t a0, a1, a2, a3, b0, b1;
      for (k = 0; k < lda; k++) {
        a0 = A[j*lda + k + 0*lda];
        a1 = A[j*lda + k + 1*lda];
        a2 = A[j*lda + k + 2*lda];
        a3 = A[j*lda + k + 3*lda];

        b0 = B[k*lda + i + 0];
        b1 = B[k*lda + i + 1];
        /*if (coreid == 0) {
          printf("i = %d; j = %d; k = %d\n", i, j, k);
          printf("%d += %d * %d; %d += %d * %d\n", (int)c00, (int)a0, (int)b0, (int)c01, (int)a0, (int)b1);
          printf("%d += %d * %d; %d += %d * %d\n", (int)c10, (int)a1, (int)b0, (int)c11, (int)a1, (int)b1);
          printf("%d += %d * %d; %d += %d * %d\n", (int)c20, (int)a2, (int)b0, (int)c21, (int)a2, (int)b1);
          printf("%d += %d * %d; %d += %d * %d\n", (int)c30, (int)a3, (int)b0, (int)c31, (int)a3, (int)b1);
          printf("\n");
        }*/

        c00 += a0 * b0; c01 += a0 * b1;
        c10 += a1 * b0; c11 += a1 * b1;
        c20 += a2 * b0; c21 += a2 * b1;
        c30 += a3 * b0; c31 += a3 * b1;
      }

      C[i + j*lda + 0 + 0*lda] = c00; C[i + j*lda + 1 + 0*lda] = c01;
      C[i + j*lda + 0 + 1*lda] = c10; C[i + j*lda + 1 + 1*lda] = c11;
      C[i + j*lda + 0 + 2*lda] = c20; C[i + j*lda + 1 + 2*lda] = c21;
      C[i + j*lda + 0 + 3*lda] = c30; C[i + j*lda + 1 + 3*lda] = c31;
    }
  }
}

void __attribute__((noinline)) matmul_mi(const int lda,  const data_t A[], const data_t B[], data_t C[] ) {
  int i, j, k;

  int curhalf = coreid;
  for (i = 0; i < lda; i += 2) {
    for (j = coreid * (lda / ncores); j < (coreid + 1) * (lda / ncores); j += 4) {
      register float c00 = 0, c01 = 0;
      register float c10 = 0, c11 = 0;
      register float c20 = 0, c21 = 0;
      register float c30 = 0, c31 = 0;

      register float a0, a1, a2, a3, b0, b1;
      for (k = curhalf * (lda/2); k < curhalf * (lda/2)  + (lda/2); k++) {
        a0 = A[j*lda + k + 0*lda];
        a1 = A[j*lda + k + 1*lda];
        a2 = A[j*lda + k + 2*lda];
        a3 = A[j*lda + k + 3*lda];

        b0 = B[k*lda + i + 0];
        b1 = B[k*lda + i + 1];

        c00 += a0 * b0; c01 += a0 * b1;
        c10 += a1 * b0; c11 += a1 * b1;
        c20 += a2 * b0; c21 += a2 * b1;
        c30 += a3 * b0; c31 += a3 * b1;
      }

      C[i + j*lda + 0 + 0*lda] += c00; C[i + j*lda + 1 + 0*lda] += c01;
      C[i + j*lda + 0 + 1*lda] += c10; C[i + j*lda + 1 + 1*lda] += c11;
      C[i + j*lda + 0 + 2*lda] += c20; C[i + j*lda + 1 + 2*lda] += c21;
      C[i + j*lda + 0 + 3*lda] += c30; C[i + j*lda + 1 + 3*lda] += c31;
    }
  }

  barrier();
  curhalf++;
  curhalf %= ncores;

  for (i = 0; i < lda; i += 2) {
    for (j = coreid * (lda / ncores); j < (coreid + 1) * (lda / ncores); j += 4) {
      register float c00 = 0, c01 = 0;
      register float c10 = 0, c11 = 0;
      register float c20 = 0, c21 = 0;
      register float c30 = 0, c31 = 0;

      register float a0, a1, a2, a3, b0, b1;
      for (k = curhalf * (lda/2); k < curhalf * (lda/2)  + (lda/2); k++) {
        a0 = A[j*lda + k + 0*lda];
        a1 = A[j*lda + k + 1*lda];
        a2 = A[j*lda + k + 2*lda];
        a3 = A[j*lda + k + 3*lda];

        b0 = B[k*lda + i + 0];
        b1 = B[k*lda + i + 1];

        c00 += a0 * b0; c01 += a0 * b1;
        c10 += a1 * b0; c11 += a1 * b1;
        c20 += a2 * b0; c21 += a2 * b1;
        c30 += a3 * b0; c31 += a3 * b1;
      }

      C[i + j*lda + 0 + 0*lda] += c00; C[i + j*lda + 1 + 0*lda] += c01;
      C[i + j*lda + 0 + 1*lda] += c10; C[i + j*lda + 1 + 1*lda] += c11;
      C[i + j*lda + 0 + 2*lda] += c20; C[i + j*lda + 1 + 2*lda] += c21;
      C[i + j*lda + 0 + 3*lda] += c30; C[i + j*lda + 1 + 3*lda] += c31;
    }
  }
}

void __attribute__((noinline)) matmul(const int lda,  const data_t A[], const data_t B[], data_t C[] )
{
  matmul_mi(lda, A, B, C);
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

