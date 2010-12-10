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
 * Define default value for unrolling factor
 */
#ifndef HPL_LASWP00N_DEPTH
#define    HPL_LASWP00N_DEPTH       32
#define    HPL_LASWP00N_LOG2_DEPTH   5
#endif

/*
 * .. Local Variables ..
 */
   register double            r;
   double                     * a0, * a1;
   const int                  incA = (int)( (unsigned int)(LDA) <<
                                            HPL_LASWP00N_LOG2_DEPTH );
   int                        ip, nr, nu;
   register int               i, j;
/* ..
 * .. Executable Statements ..
 */
   if( ( M <= 0 ) || ( N <= 0 ) ) return;

   nr = N - ( nu = (int)( ( (unsigned int)(N) >> HPL_LASWP00N_LOG2_DEPTH )
                          << HPL_LASWP00N_LOG2_DEPTH ) );

   for( j = 0; j < nu; j += HPL_LASWP00N_DEPTH, A += incA )
   {
      for( i = 0; i < M; i++ )
      {
         if( i != ( ip = IPIV[i] ) )
         {
            a0 = A + i; a1 = A + ip;

            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
#if ( HPL_LASWP00N_DEPTH >  1 )
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
#endif
#if ( HPL_LASWP00N_DEPTH >  2 )
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
#endif
#if ( HPL_LASWP00N_DEPTH >  4 )
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
#endif
#if ( HPL_LASWP00N_DEPTH >  8 )
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
#endif
#if ( HPL_LASWP00N_DEPTH > 16 )
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
            r = *a0; *a0 = *a1; *a1 = r; a0 += LDA; a1 += LDA;
#endif
         }
      }
   }

   if( nr > 0 )
   {
      for( i = 0; i < M; i++ )
      {
         if( i != ( ip = IPIV[i] ) )
         {
            a0 = A + i; a1 = A + ip;
            for( j = 0; j < nr; j++, a0 += LDA, a1 += LDA )
            { r = *a0; *a0 = *a1; *a1 = r; }
         }
      }
   }
