/*
 *  -- High Performance Computing Linpack Benchmark (HPL-GPU)
 *     HPL-GPU - 1.0 - 2010
 *
 *     David Rohr
 *     Matthias Kretz
 *     Matthias Bach
 *     Goethe Universität, Frankfurt am Main
 *     Frankfurt Institute for Advanced Studies
 *     (C) Copyright 2010 All Rights Reserved
 *
 *     Antoine P. Petitet
 *     University of Tennessee, Knoxville
 *     Innovative Computing Laboratory
 *     (C) Copyright 2000-2008 All Rights Reserved
 *
 *  -- Copyright notice and Licensing terms:
 *
 *  Redistribution  and  use in  source and binary forms, with or without
 *  modification, are  permitted provided  that the following  conditions
 *  are met:
 *
 *  1. Redistributions  of  source  code  must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce  the above copyright
 *  notice, this list of conditions,  and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *
 *  3. All  advertising  materials  mentioning  features  or  use of this
 *  software must display the following acknowledgements:
 *  This  product  includes  software  developed  at  the  University  of
 *  Tennessee, Knoxville, Innovative Computing Laboratory.
 *  This product  includes software  developed at the Frankfurt Institute
 *  for Advanced Studies.
 *
 *  4. The name of the  University,  the name of the  Laboratory,  or the
 *  names  of  its  contributors  may  not  be used to endorse or promote
 *  products  derived   from   this  software  without  specific  written
 *  permission.
 *
 *  -- Disclaimer:
 *
 *  THIS  SOFTWARE  IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,  INCLUDING,  BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE UNIVERSITY
 *  OR  CONTRIBUTORS  BE  LIABLE FOR ANY  DIRECT,  INDIRECT,  INCIDENTAL,
 *  SPECIAL,  EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES  (INCLUDING,  BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA OR PROFITS; OR BUSINESS INTERRUPTION)  HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT,  STRICT LIABILITY,  OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ======================================================================
 */

/*
 * Include files
 */
#include "glibc_hacks.h"
#include "hpl.h"
#include <sys/mman.h>
#include "util_cal.h"
#include <pthread.h>

#define FASTRAND_THREADS 24
int fastrand_seed;
volatile int fastrand_done[FASTRAND_THREADS];
double* fastrand_A;
size_t fastrand_size;

void* fastmatgen_slave(void* arg)
{
   int num = (int) (size_t) arg;
   

   cpu_set_t mask;
   CPU_ZERO(&mask);
   CPU_SET(num, &mask);
   sched_setaffinity(0, sizeof(cpu_set_t), &mask);

   size_t fastrand_num = fastrand_seed + 65537 * num;
   const size_t fastrand_mul = 84937482743;
   const size_t fastrand_add = 138493846343;
   const size_t fastrand_mod = 538948374763;
   
   size_t sizeperthread = fastrand_size / FASTRAND_THREADS;
   
   double* A = fastrand_A + num * sizeperthread;
   size_t size = (num == FASTRAND_THREADS - 1) ? (fastrand_size - (FASTRAND_THREADS - 1) * sizeperthread) : sizeperthread;
   
   for (size_t i = 0;i < size;i++)
   {
	fastrand_num = (fastrand_num * fastrand_mul + fastrand_add) % fastrand_mod;
	A[i] = (double) -0.5 + (double) fastrand_num / (double)fastrand_mod;
   }
   
   
   fastrand_done[num] = 1;
   return(NULL);
}

void fastmatgen(int SEED, double* A, size_t size)
{
    fastrand_seed = SEED;
    fastrand_A = A;
    fastrand_size = size;
    memset((void*) fastrand_done, 0, FASTRAND_THREADS * sizeof(int));
    
    cpu_set_t oldmask;
    sched_getaffinity(0, sizeof(cpu_set_t), &oldmask);
    
    for (int i = 0;i < FASTRAND_THREADS - 1;i++)
    {
	pthread_t thr;
	pthread_create(&thr, NULL, fastmatgen_slave, (void*) (size_t) i);
    }
    fastmatgen_slave((void*) (size_t) (FASTRAND_THREADS - 1));
        
    for (int i = 0;i < FASTRAND_THREADS;i++)
    {
	while (fastrand_done[i] == 0)
	{
	}
    }
    sched_setaffinity(0, sizeof(cpu_set_t), &oldmask);
}

void HPL_pdtest
(
   HPL_T_test *                     TEST,
   HPL_T_grid *                     GRID,
   HPL_T_palg *                     ALGO,
   const int                        N,
   const int                        NB,
   const int                        SEED
)
{
/* 
 * Purpose
 * =======
 *
 * HPL_pdtest performs  one  test  given a set of parameters such as the
 * process grid, the  problem size, the distribution blocking factor ...
 * This function generates  the data, calls  and times the linear system
 * solver,  checks  the  accuracy  of the  obtained vector solution  and
 * writes this information to the file pointed to by TEST->outfp.
 *
 * Arguments
 * =========
 *
 * TEST    (global input)                HPL_T_test *
 *         On entry,  TEST  points  to a testing data structure:  outfp
 *         specifies the output file where the results will be printed.
 *         It is only defined and used by the process  0  of the  grid.
 *         thrsh  specifies  the  threshhold value  for the test ratio.
 *         Concretely, a test is declared "PASSED"  if and only if the
 *         following inequality is satisfied:
 *         ||Ax-b||_oo / ( epsil *
 *                         ( || x ||_oo * || A ||_oo + || b ||_oo ) *
 *                          N )  < thrsh.
 *         epsil  is the  relative machine precision of the distributed
 *         computer. Finally the test counters, kfail, kpass, kskip and
 *         ktest are updated as follows:  if the test passes,  kpass is
 *         incremented by one;  if the test fails, kfail is incremented
 *         by one; if the test is skipped, kskip is incremented by one.
 *         ktest is left unchanged.
 *
 * GRID    (local input)                 HPL_T_grid *
 *         On entry,  GRID  points  to the data structure containing the
 *         process grid information.
 *
 * ALGO    (global input)                HPL_T_palg *
 *         On entry,  ALGO  points to  the data structure containing the
 *         algorithmic parameters to be used for this test.
 *
 * N       (global input)                const int
 *         On entry,  N specifies the order of the coefficient matrix A.
 *         N must be at least zero.
 *
 * NB      (global input)                const int
 *         On entry,  NB specifies the blocking factor used to partition
 *         and distribute the matrix A. NB must be larger than one.
 *
 * SEED    (global input)                const int
 *         On entry, SEED specifies the seed to be used for matrix
 *         generation. SEED is greater than zero.
 *
 * ---------------------------------------------------------------------
 */ 
/*
 * .. Local Variables ..
 */
#ifdef HPL_DETAILED_TIMING
   double                     HPL_w[HPL_TIMING_N];
   double                     HPL_c[HPL_TIMING_N];
#endif
   HPL_T_pmat                 mat;
   double                     walltime[1];
   double                     cputime[1];
   int                        info[3];
   double                     Anorm1, AnormI, Gflops, Xnorm1, XnormI,
                              BnormI, resid0, resid1;
   double                     * Bptr;
   void                       * vptr = NULL;
   static int                 first=1;
   int                        ii, ip2, mycol, myrow, npcol, nprow, nq;
   char                       ctop, cpfact, crfact;
/* ..
 * .. Executable Statements ..
 */
   (void) HPL_grid_info( GRID, &nprow, &npcol, &myrow, &mycol );

   mat.n  = N; mat.nb = NB; mat.info = 0;
   mat.mp = HPL_numrow( N, NB, NB, myrow, nprow );
   nq     = HPL_numcol( N, NB, NB, mycol, npcol, GRID );
   mat.nq = nq + 1;
/*
 * Allocate matrix, right-hand-side, and vector solution x. [ A | b ] is
 * N by N+1.  One column is added in every process column for the solve.
 * The  result  however  is stored in a 1 x N vector replicated in every
 * process row. In every process, A is lda * (nq+1), x is 1 * nq and the
 * workspace is mp. 
 *
 * Ensure that lda is a multiple of ALIGN and not a power of 2
 */
 
   mat.ld = ( ( Mmax( 1, mat.mp ) - 1 ) / ALGO->align ) * ALGO->align;
   
   //Make sure LDA is an uneven multiple of cache line size
   if (mat.ld % 64) mat.ld += 64 - mat.ld % 64;
   if (mat.ld % 128 == 0) mat.ld += 64;
   
   do
   {
      ii = ( mat.ld += ALGO->align ); ip2 = 1;
      while( ii > 1 ) { ii >>= 1; ip2 <<= 1; }
   }
   while( mat.ld == ip2 );
/*
 * Allocate dynamic memory
 */
   /*vptr = (void*)malloc( ( (size_t)(ALGO->align) + (size_t)(mat.ld+1) * (size_t)(mat.nq) ) * sizeof(double) );*/
   vptr = CALDGEMM_alloc( ((size_t)(ALGO->align) + (size_t)(mat.ld+1) * (size_t)(mat.nq)) * sizeof(double) );
                         
   info[0] = (vptr == NULL); info[1] = myrow; info[2] = mycol;
   (void) HPL_all_reduce( (void *)(info), 3, HPL_INT, HPL_max,
                          GRID->all_comm );
   if( info[0] != 0 )
   {
      if( ( myrow == 0 ) && ( mycol == 0 ) )
         HPL_pwarn( TEST->outfp, __LINE__, "HPL_pdtest",
                    "[%d,%d] %s", info[1], info[2],
                    "Memory allocation failed for A, x and b. Skip." );
      (TEST->kskip)++;
      return;
   }
/*
 * generate matrix and right-hand-side, [ A | b ] which is N by N+1.
 */
   mat.A  = (double *) HPL_PTR( vptr, ((size_t)(ALGO->align) * sizeof(double) ) );
   mat.X  = Mptr( mat.A, 0, mat.nq, mat.ld );
#ifndef HPL_FASTINIT
   HPL_pdmatgen( GRID, N, N+1, NB, mat.A, mat.ld, SEED );
#else
#ifndef QON_TEST
   fastmatgen( SEED + myrow * npcol + mycol, mat.A, mat.X - mat.A);
#else
   srand(453534);
   for (int i = 0;i < mat.n;i++)
   {
	for (int j = 0;j < mat.n + 1;j++)
	{
	    int II, JJ, P, Q;
	    HPL_infog2l(i, j, mat.nb, mat.nb, mat.nb, mat.nb, 0, 0, GRID->myrow, GRID->mycol, GRID->nprow, GRID->npcol, &II, &JJ, &P, &Q, GRID);
	    double rval = (double) rand() / (double) RAND_MAX - 0.5;
	    if (P == GRID->myrow && Q == GRID->mycol)
	    {
		mat.A[JJ * mat.ld + II] = rval;
	    }
	}
    }
#endif
#endif

/*
 * Solve linear system
 */
   HPL_ptimer_boot(); (void) HPL_barrier( GRID->all_comm );
   HPL_ptimer( 0 );
   HPL_pdgesv( GRID, ALGO, &mat );
   HPL_ptimer( 0 );
/*
 * Gather max of all CPU and WALL clock timings and print timing results
 */
   HPL_ptimer_combine( GRID->all_comm, HPL_AMAX_PTIME, HPL_WALL_PTIME,
                       1, 0, walltime );
   HPL_ptimer_combine( GRID->all_comm, HPL_AMAX_PTIME, HPL_CPU_TIME,
                       1, 0, cputime );

   if( ( myrow == 0 ) && ( mycol == 0 ) )
   {
      if( first )
      {
         HPL_fprintf( TEST->outfp, "%s%s\n",
                      "========================================",
                      "========================================" );
         HPL_fprintf( TEST->outfp, "%s%s\n",
                      "T/V                N    NB     P     Q",
                      "               Time    CPU          Gflops" );
         HPL_fprintf( TEST->outfp, "%s%s\n",
                      "----------------------------------------",
                      "----------------------------------------" );
         if( TEST->thrsh <= HPL_rzero ) first = 0;
      }
/*
 * 2/3 N^3 - 1/2 N^2 flops for LU factorization + 2 N^2 flops for solve.
 * Print WALL time
 */
      Gflops = ( ( (double)(N) /   1.0e+9 ) * 
                 ( (double)(N) / walltime[0] ) ) * 
                 ( ( 2.0 / 3.0 ) * (double)(N) + ( 3.0 / 2.0 ) );

      cpfact = ( ( (HPL_T_FACT)(ALGO->pfact) == 
                   (HPL_T_FACT)(HPL_LEFT_LOOKING) ) ?  (char)('L') :
                 ( ( (HPL_T_FACT)(ALGO->pfact) == (HPL_T_FACT)(HPL_CROUT) ) ?
                   (char)('C') : (char)('R') ) );
      crfact = ( ( (HPL_T_FACT)(ALGO->rfact) == 
                   (HPL_T_FACT)(HPL_LEFT_LOOKING) ) ?  (char)('L') :
                 ( ( (HPL_T_FACT)(ALGO->rfact) == (HPL_T_FACT)(HPL_CROUT) ) ? 
                   (char)('C') : (char)('R') ) );

      if(      ALGO->btopo == HPL_1RING   ) ctop = '0';
      else if( ALGO->btopo == HPL_1RING_M ) ctop = '1';
      else if( ALGO->btopo == HPL_2RING   ) ctop = '2';
      else if( ALGO->btopo == HPL_2RING_M ) ctop = '3';
      else if( ALGO->btopo == HPL_BLONG   ) ctop = '4';
      else if( ALGO->btopo == HPL_BLONG_M ) ctop = '5';
      else /* if( ALGO->btopo == HPL_MPI_BCAST ) */ ctop = '6';

      if( walltime[0] > HPL_rzero )
         HPL_fprintf( TEST->outfp,
             "W%c%1d%c%c%1d%c%1d%12d %5d %5d %5d %18.2f %6.2f %15.3e\n",
             ( GRID->order == HPL_ROW_MAJOR ? 'R' : 'C' ),
             ALGO->depth, ctop, crfact, ALGO->nbdiv, cpfact, ALGO->nbmin,
             N, NB, nprow, npcol, walltime[0], cputime[0], Gflops );
#ifdef HPL_PRINT_AVG_MATRIX_SIZE
      float avgSize = (float) ( N ) * N * 8 / nprow / npcol / 1024 / 1024 / 1024;
      HPL_fprintf( TEST->outfp, "Avg. matri size per node: %.2f GiB\n", avgSize );
#endif
   }
#ifdef HPL_DETAILED_TIMING
   HPL_ptimer_combine( GRID->all_comm, HPL_AMAX_PTIME, HPL_WALL_PTIME,
                       HPL_TIMING_N, HPL_TIMING_BEG, HPL_w );
   HPL_ptimer_combine( GRID->all_comm, HPL_AMAX_PTIME, HPL_CPU_TIME,
                       HPL_TIMING_N, HPL_TIMING_BEG, HPL_c );
   if( ( myrow == 0 ) && ( mycol == 0 ) )
   {
      HPL_fprintf( TEST->outfp, "%s%s\n",
                   "--VVV--VVV--VVV--VVV--VVV--VVV--VVV--V",
                   "VV--VVV--VVV--VVV--VVV--VVV--VVV--VVV-" );
/*
 * Recursive panel factorization
 */
      if( HPL_w[HPL_TIMING_RPFACT-HPL_TIMING_BEG] > HPL_rzero )
         HPL_fprintf( TEST->outfp,
                      "Max aggregated wall time rfact . . . : %18.2f %6.2f %4.2f\n",
                      HPL_w[HPL_TIMING_RPFACT-HPL_TIMING_BEG], HPL_c[HPL_TIMING_RPFACT-HPL_TIMING_BEG],
                      HPL_c[HPL_TIMING_RPFACT-HPL_TIMING_BEG] / HPL_w[HPL_TIMING_RPFACT-HPL_TIMING_BEG]
                 );
/*
 * Update
 */
      if( HPL_w[HPL_TIMING_UPDATE-HPL_TIMING_BEG] > HPL_rzero )
         HPL_fprintf( TEST->outfp,
                      "Max aggregated wall time update  . . : %18.2f %6.2f %4.2f\n",
                      HPL_w[HPL_TIMING_UPDATE-HPL_TIMING_BEG], HPL_c[HPL_TIMING_UPDATE-HPL_TIMING_BEG],
                      HPL_c[HPL_TIMING_UPDATE-HPL_TIMING_BEG] / HPL_w[HPL_TIMING_UPDATE-HPL_TIMING_BEG]
                 );
/*
 * Update (swap)
 */
      if( HPL_w[HPL_TIMING_LASWP-HPL_TIMING_BEG] > HPL_rzero )
         HPL_fprintf( TEST->outfp,
                      "+ Max aggregated wall time laswp . . : %18.2f %6.2f %4.2f\n",
                      HPL_w[HPL_TIMING_LASWP-HPL_TIMING_BEG], HPL_c[HPL_TIMING_LASWP-HPL_TIMING_BEG],
                      HPL_c[HPL_TIMING_LASWP-HPL_TIMING_BEG] / HPL_w[HPL_TIMING_LASWP-HPL_TIMING_BEG]
                 );
/*
 * Upper triangular system solve
 */
      if( HPL_w[HPL_TIMING_PTRSV-HPL_TIMING_BEG] > HPL_rzero )
         HPL_fprintf( TEST->outfp,
                      "Max aggregated wall time up tr sv  . : %18.2f %6.2f %4.2f\n",
                      HPL_w[HPL_TIMING_PTRSV-HPL_TIMING_BEG], HPL_c[HPL_TIMING_PTRSV-HPL_TIMING_BEG],
                      HPL_c[HPL_TIMING_PTRSV-HPL_TIMING_BEG] / HPL_w[HPL_TIMING_PTRSV-HPL_TIMING_BEG]
                 );

      if( TEST->thrsh <= HPL_rzero )
         HPL_fprintf( TEST->outfp, "%s%s\n",
                      "========================================",
                      "========================================" );
   }
#endif
/*
 * Quick return, if I am not interested in checking the computations
 */
   if( TEST->thrsh <= HPL_rzero )
   { (TEST->kpass)++; if( vptr ) CALDGEMM_free( vptr ); return; }
/*
 * Check info returned by solve
 */
   if( mat.info != 0 )
   {
      if( ( myrow == 0 ) && ( mycol == 0 ) )
         HPL_pwarn( TEST->outfp, __LINE__, "HPL_pdtest", "%s %d, %s", 
                    "Error code returned by solve is", mat.info, "skip" );
      (TEST->kskip)++;
      if( vptr ) CALDGEMM_free( vptr ); return;
   }
/*
 * Check computation, re-generate [ A | b ], compute norm 1 and inf of A and x,
 * and norm inf of b - A x. Display residual checks.
 */
#if !defined(HPL_FASTINIT) | !defined(HPL_FASTVERIFY)
   HPL_pdmatgen( GRID, N, N+1, NB, mat.A, mat.ld, SEED );
#else
   fastmatgen( SEED + myrow * npcol + mycol, mat.A, mat.X - mat.A);
#endif

   Anorm1 = HPL_pdlange( GRID, HPL_NORM_1, N, N, NB, mat.A, mat.ld );
   AnormI = HPL_pdlange( GRID, HPL_NORM_I, N, N, NB, mat.A, mat.ld );
/*
 * Because x is distributed in process rows, switch the norms
 */
   XnormI = HPL_pdlange( GRID, HPL_NORM_1, 1, N, NB, mat.X, 1 );
   Xnorm1 = HPL_pdlange( GRID, HPL_NORM_I, 1, N, NB, mat.X, 1 );
/*
 * If I am in the col that owns b, (1) compute local BnormI, (2) all_reduce to
 * find the max (in the col). Then (3) broadcast along the rows so that every
 * process has BnormI. Note that since we use a uniform distribution in [-0.5,0.5]
 * for the entries of B, it is very likely that BnormI (<=,~) 0.5.
 */
   Bptr = Mptr( mat.A, 0, nq, mat.ld );
   if( mycol == HPL_indxg2p_col( N, NB, NB, npcol, GRID ) ){
      if( mat.mp > 0 )
      {
         BnormI = Bptr[HPL_idamax( mat.mp, Bptr, 1 )]; BnormI = Mabs( BnormI );
      }
      else
      {
         BnormI = HPL_rzero;
      }
      (void) HPL_all_reduce( (void *)(&BnormI), 1, HPL_DOUBLE, HPL_max,
                             GRID->col_comm );
   }
   (void) HPL_broadcast( (void *)(&BnormI), 1, HPL_DOUBLE,
                          HPL_indxg2p_col( N, NB, NB, npcol, GRID ),
                          GRID->row_comm );
/*
 * If I own b, compute ( b - A x ) and ( - A x ) otherwise
 */
   if( mycol == HPL_indxg2p_col( N, NB, NB, npcol, GRID ) )
   {
      HPL_dgemv( HplColumnMajor, HplNoTrans, mat.mp, nq, -HPL_rone,
                 mat.A, mat.ld, mat.X, 1, HPL_rone, Bptr, 1 );
   }
   else if( nq > 0 )
   {
      HPL_dgemv( HplColumnMajor, HplNoTrans, mat.mp, nq, -HPL_rone,
                 mat.A, mat.ld, mat.X, 1, HPL_rzero, Bptr, 1 );
   }
   else { for( ii = 0; ii < mat.mp; ii++ ) Bptr[ii] = HPL_rzero; }
/*
 * Reduce the distributed residual in process column 0
 */
   if( mat.mp > 0 )
      (void) HPL_reduce( Bptr, mat.mp, HPL_DOUBLE, HPL_sum, 0,
                         GRID->row_comm );
/*
 * Compute || b - A x ||_oo
 */
   resid0 = HPL_pdlange( GRID, HPL_NORM_I, N, 1, NB, Bptr, mat.ld );
/*
 * Computes and displays norms, residuals ...
 */
   if( N <= 0 )
   {
      resid1 = HPL_rzero;
   }
   else
   {
      resid1 = resid0 / ( TEST->epsil * ( AnormI * XnormI + BnormI ) * (double)(N) );
   }

   if( resid1 < TEST->thrsh ) (TEST->kpass)++;
   else                       (TEST->kfail)++;

   if( ( myrow == 0 ) && ( mycol == 0 ) )
   {
      HPL_fprintf( TEST->outfp, "%s%s\n",
                   "----------------------------------------",
                   "----------------------------------------" );
      HPL_fprintf( TEST->outfp, "%s%16.7f%s%s\n",
         "||Ax-b||_oo/(eps*(||A||_oo*||x||_oo+||b||_oo)*N)= ", resid1,
         " ...... ", ( resid1 < TEST->thrsh ? "PASSED" : "FAILED" ) );

      if( resid1 >= TEST->thrsh ) 
      {
         HPL_fprintf( TEST->outfp, "%s%18.6f\n",
         "||Ax-b||_oo  . . . . . . . . . . . . . . . . . = ", resid0 );
         HPL_fprintf( TEST->outfp, "%s%18.6f\n",
         "||A||_oo . . . . . . . . . . . . . . . . . . . = ", AnormI );
         HPL_fprintf( TEST->outfp, "%s%18.6f\n",
         "||A||_1  . . . . . . . . . . . . . . . . . . . = ", Anorm1 );
         HPL_fprintf( TEST->outfp, "%s%18.6f\n",
         "||x||_oo . . . . . . . . . . . . . . . . . . . = ", XnormI );
         HPL_fprintf( TEST->outfp, "%s%18.6f\n",
         "||x||_1  . . . . . . . . . . . . . . . . . . . = ", Xnorm1 );
         HPL_fprintf( TEST->outfp, "%s%18.6f\n",
         "||b||_oo . . . . . . . . . . . . . . . . . . . = ", BnormI );
      }
   }
   if( vptr ) CALDGEMM_free( vptr );
/*
 * End of HPL_pdtest
 */
}
