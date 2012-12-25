#ifndef kernel_module
	#define kernel_module (1)
#endif

#if kernel_module
	//#include <linux/string.h>
	//#include <linux/slab.h>
	#include "gsfs.h"
	#define free(p) kfree(p)
	#define malloc(p) kmalloc(p,GFP_KERNEL)
	#define gettimeofday(p,q) do_gettimeofday(p);
#else
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/time.h>
	#include <signal.h>
	#include <time.h>
	#include <stdlib.h>
	#include <string.h>
	#include <stdio.h>
	#include <stdarg.h>

#endif

//bignum.h
/**
 * \file bignum.h
 *
 *  Copyright (C) 2006-2010, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef POLARSSL_BIGNUM_H
#define POLARSSL_BIGNUM_H

//#include <stdio.h>

#define POLARSSL_ERR_MPI_FILE_IO_ERROR                     0x0002
#define POLARSSL_ERR_MPI_BAD_INPUT_DATA                    0x0004
#define POLARSSL_ERR_MPI_INVALID_CHARACTER                 0x0006
#define POLARSSL_ERR_MPI_BUFFER_TOO_SMALL                  0x0008
#define POLARSSL_ERR_MPI_NEGATIVE_VALUE                    0x000A
#define POLARSSL_ERR_MPI_DIVISION_BY_ZERO                  0x000C
#define POLARSSL_ERR_MPI_NOT_ACCEPTABLE                    0x000E

#define MPI_CHK(f) if( ( ret = f ) != 0 ) goto cleanup

/*
 * Define the base integer type, architecture-wise
 */
#if !kernel_module
typedef unsigned long t_int;
typedef unsigned int t_dbl __attribute__((mode(TI)));
/**
 * \brief          MPI structure
 */
typedef struct
{
    int s;              /*!<  integer sign      */
    int n;              /*!<  total # of limbs  */
    t_int *p;           /*!<  pointer to limbs  */
}
mpi;

/**
 * \brief          Initialize one or more mpi
 */
void mpi_init( mpi *X, ... );

/**
 * \brief          Unallocate one or more mpi
 */
void mpi_free( mpi *X, ... );

/**
 * \brief          Copy the contents of Y into X
 *
 * \param X        Destination MPI
 * \param Y        Source MPI
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_copy( mpi *X, const mpi *Y );
#endif
/**
 * \brief          Enlarge to the specified number of limbs
 *
 * \param X        MPI to grow
 * \param nblimbs  The target number of limbs
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_grow( mpi *X, int nblimbs );

/**
 * \brief          Swap the contents of X and Y
 *
 * \param X        First MPI value
 * \param Y        Second MPI value
 */
void mpi_swap( mpi *X, mpi *Y );

/**
 * \brief          Set value from integer
 *
 * \param X        MPI to set
 * \param z        Value to use
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_lset( mpi *X, int z );

/**
 * \brief          Return the number of least significant bits
 *
 * \param X        MPI to use
 */
int mpi_lsb( const mpi *X );

/**
 * \brief          Return the number of most significant bits
 *
 * \param X        MPI to use
 */
int mpi_msb( const mpi *X );

/**
 * \brief          Return the total size in bytes
 *
 * \param X        MPI to use
 */
int mpi_size( const mpi *X );

/**
 * \brief          Import from an ASCII string
 *
 * \param X        Destination MPI
 * \param radix    Input numeric base
 * \param s        Null-terminated string buffer
 *
 * \return         0 if successful, or an POLARSSL_ERR_MPI_XXX error code
 */
int mpi_read_string( mpi *X, int radix, const char *s );

/**
 * \brief          Export into an ASCII string
 *
 * \param X        Source MPI
 * \param radix    Output numeric base
 * \param s        String buffer
 * \param slen     String buffer size
 *
 * \return         0 if successful, or an POLARSSL_ERR_MPI_XXX error code.
 *                 *slen is always updated to reflect the amount
 *                 of data that has (or would have) been written.
 *
 * \note           Call this function with *slen = 0 to obtain the
 *                 minimum required buffer size in *slen.
 */
int mpi_write_string( const mpi *X, int radix, char *s, int *slen );

/**
 * \brief          Read X from an opened file
 *
 * \param X        Destination MPI
 * \param radix    Input numeric base
 * \param fin      Input file handle
 *
 * \return         0 if successful, or an POLARSSL_ERR_MPI_XXX error code
 */
#if !kernel_module
	int mpi_read_file( mpi *X, int radix, FILE *fin );
#endif
/**
 * \brief          Write X into an opened file, or stdout if fout is NULL
 *
 * \param p        Prefix, can be NULL
 * \param X        Source MPI
 * \param radix    Output numeric base
 * \param fout     Output file handle (can be NULL)
 *
 * \return         0 if successful, or an POLARSSL_ERR_MPI_XXX error code
 *
 * \note           Set fout == NULL to print X on the console.
 */
#if !kernel_module
int mpi_write_file( const char *p, const mpi *X, int radix, FILE *fout );
#endif
/**
 * \brief          Import X from unsigned binary data, big endian
 *
 * \param X        Destination MPI
 * \param buf      Input buffer
 * \param buflen   Input buffer size
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_read_binary( mpi *X, const unsigned char *buf, int buflen );

/**
 * \brief          Export X into unsigned binary data, big endian
 *
 * \param X        Source MPI
 * \param buf      Output buffer
 * \param buflen   Output buffer size
 *
 * \return         0 if successful,
 *                 POLARSSL_ERR_MPI_BUFFER_TOO_SMALL if buf isn't large enough
 */
int mpi_write_binary( const mpi *X, unsigned char *buf, int buflen );

/**
 * \brief          Left-shift: X <<= count
 *
 * \param X        MPI to shift
 * \param count    Amount to shift
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_shift_l( mpi *X, int count );

/**
 * \brief          Right-shift: X >>= count
 *
 * \param X        MPI to shift
 * \param count    Amount to shift
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_shift_r( mpi *X, int count );

/**
 * \brief          Compare unsigned values
 *
 * \param X        Left-hand MPI
 * \param Y        Right-hand MPI
 *
 * \return         1 if |X| is greater than |Y|,
 *                -1 if |X| is lesser  than |Y| or
 *                 0 if |X| is equal to |Y|
 */
int mpi_cmp_abs( const mpi *X, const mpi *Y );

/**
 * \brief          Compare signed values
 *
 * \param X        Left-hand MPI
 * \param Y        Right-hand MPI
 *
 * \return         1 if X is greater than Y,
 *                -1 if X is lesser  than Y or
 *                 0 if X is equal to Y
 */
int mpi_cmp_mpi( const mpi *X, const mpi *Y );

/**
 * \brief          Compare signed values
 *
 * \param X        Left-hand MPI
 * \param z        The integer value to compare to
 *
 * \return         1 if X is greater than z,
 *                -1 if X is lesser  than z or
 *                 0 if X is equal to z
 */
int mpi_cmp_int( const mpi *X, int z );

/**
 * \brief          Unsigned addition: X = |A| + |B|
 *
 * \param X        Destination MPI
 * \param A        Left-hand MPI
 * \param B        Right-hand MPI
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_add_abs( mpi *X, const mpi *A, const mpi *B );

/**
 * \brief          Unsigned substraction: X = |A| - |B|
 *
 * \param X        Destination MPI
 * \param A        Left-hand MPI
 * \param B        Right-hand MPI
 *
 * \return         0 if successful,
 *                 POLARSSL_ERR_MPI_NEGATIVE_VALUE if B is greater than A
 */
int mpi_sub_abs( mpi *X, const mpi *A, const mpi *B );

/**
 * \brief          Signed addition: X = A + B
 *
 * \param X        Destination MPI
 * \param A        Left-hand MPI
 * \param B        Right-hand MPI
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_add_mpi( mpi *X, const mpi *A, const mpi *B );

/**
 * \brief          Signed substraction: X = A - B
 *
 * \param X        Destination MPI
 * \param A        Left-hand MPI
 * \param B        Right-hand MPI
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_sub_mpi( mpi *X, const mpi *A, const mpi *B );

/**
 * \brief          Signed addition: X = A + b
 *
 * \param X        Destination MPI
 * \param A        Left-hand MPI
 * \param b        The integer value to add
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_add_int( mpi *X, const mpi *A, int b );

/**
 * \brief          Signed substraction: X = A - b
 *
 * \param X        Destination MPI
 * \param A        Left-hand MPI
 * \param b        The integer value to subtract
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_sub_int( mpi *X, const mpi *A, int b );

/**
 * \brief          Baseline multiplication: X = A * B
 *
 * \param X        Destination MPI
 * \param A        Left-hand MPI
 * \param B        Right-hand MPI
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_mul_mpi( mpi *X, const mpi *A, const mpi *B );

/**
 * \brief          Baseline multiplication: X = A * b
 *                 Note: b is an unsigned integer type, thus
 *                 Negative values of b are ignored.
 *
 * \param X        Destination MPI
 * \param A        Left-hand MPI
 * \param b        The integer value to multiply with
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_mul_int( mpi *X, const mpi *A, t_int b );

/**
 * \brief          Division by mpi: A = Q * B + R
 *
 * \param Q        Destination MPI for the quotient
 * \param R        Destination MPI for the rest value
 * \param A        Left-hand MPI
 * \param B        Right-hand MPI
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed,
 *                 POLARSSL_ERR_MPI_DIVISION_BY_ZERO if B == 0
 *
 * \note           Either Q or R can be NULL.
 */
int mpi_div_mpi( mpi *Q, mpi *R, const mpi *A, const mpi *B );

/**
 * \brief          Division by int: A = Q * b + R
 *
 * \param Q        Destination MPI for the quotient
 * \param R        Destination MPI for the rest value
 * \param A        Left-hand MPI
 * \param b        Integer to divide by
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed,
 *                 POLARSSL_ERR_MPI_DIVISION_BY_ZERO if b == 0
 *
 * \note           Either Q or R can be NULL.
 */
int mpi_div_int( mpi *Q, mpi *R, const mpi *A, int b );

/**
 * \brief          Modulo: R = A mod B
 *
 * \param R        Destination MPI for the rest value
 * \param A        Left-hand MPI
 * \param B        Right-hand MPI
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed,
 *                 POLARSSL_ERR_MPI_DIVISION_BY_ZERO if B == 0,
 *                 POLARSSL_ERR_MPI_NEGATIVE_VALUE if B < 0
 */
int mpi_mod_mpi( mpi *R, const mpi *A, const mpi *B );

/**
 * \brief          Modulo: r = A mod b
 *
 * \param r        Destination t_int
 * \param A        Left-hand MPI
 * \param b        Integer to divide by
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed,
 *                 POLARSSL_ERR_MPI_DIVISION_BY_ZERO if b == 0,
 *                 POLARSSL_ERR_MPI_NEGATIVE_VALUE if b < 0
 */
int mpi_mod_int( t_int *r, const mpi *A, int b );

/**
 * \brief          Sliding-window exponentiation: X = A^E mod N
 *
 * \param X        Destination MPI 
 * \param A        Left-hand MPI
 * \param E        Exponent MPI
 * \param N        Modular MPI
 * \param _RR      Speed-up MPI used for recalculations
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed,
 *                 POLARSSL_ERR_MPI_BAD_INPUT_DATA if N is negative or even
 *
 * \note           _RR is used to avoid re-computing R*R mod N across
 *                 multiple calls, which speeds up things a bit. It can
 *                 be set to NULL if the extra performance is unneeded.
 */
int mpi_exp_mod( mpi *X, const mpi *A, const mpi *E, const mpi *N, mpi *_RR );

/**
 * \brief          Greatest common divisor: G = gcd(A, B)
 *
 * \param G        Destination MPI
 * \param A        Left-hand MPI
 * \param B        Right-hand MPI
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_gcd( mpi *G, const mpi *A, const mpi *B );

/**
 * \brief          Modular inverse: X = A^-1 mod N
 *
 * \param X        Destination MPI
 * \param A        Left-hand MPI
 * \param N        Right-hand MPI
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed,
 *                 POLARSSL_ERR_MPI_BAD_INPUT_DATA if N is negative or nil
                   POLARSSL_ERR_MPI_NOT_ACCEPTABLE if A has no inverse mod N
 */
int mpi_inv_mod( mpi *X, const mpi *A, const mpi *N );

/**
 * \brief          Miller-Rabin primality test
 *
 * \param X        MPI to check
 * \param f_rng    RNG function
 * \param p_rng    RNG parameter
 *
 * \return         0 if successful (probably prime),
 *                 1 if memory allocation failed,
 *                 POLARSSL_ERR_MPI_NOT_ACCEPTABLE if X is not prime
 */
int mpi_is_prime( mpi *X, int (*f_rng)(void *), void *p_rng );

/**
 * \brief          Prime number generation
 *
 * \param X        Destination MPI
 * \param nbits    Required size of X in bits
 * \param dh_flag  If 1, then (X-1)/2 will be prime too
 * \param f_rng    RNG function
 * \param p_rng    RNG parameter
 *
 * \return         0 if successful (probably prime),
 *                 1 if memory allocation failed,
 *                 POLARSSL_ERR_MPI_BAD_INPUT_DATA if nbits is < 3
 */
int mpi_gen_prime( mpi *X, int nbits, int dh_flag,
                   int (*f_rng)(void *), void *p_rng );

/**
 * \brief          Checkup routine
 *
 * \return         0 if successful, or 1 if the test failed
 */
int mpi_self_test( int verbose );


#endif /* bignum.h */

//bn_mul.h
/**
 * \file bn_mul.h
 *
 *  Copyright (C) 2006-2010, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 *      Multiply source vector [s] with b, add result
 *       to destination vector [d] and set carry c.
 *
 *      Currently supports:
 *
 *         . IA-32 (386+)         . AMD64 / EM64T
 *         . IA-32 (SSE2)         . Motorola 68000
 *         . PowerPC, 32-bit      . MicroBlaze
 *         . PowerPC, 64-bit      . TriCore
 *         . SPARC v8             . ARM v3+
 *         . Alpha                . MIPS32
 *         . C, longlong          . C, generic
 */
#ifndef POLARSSL_BN_MUL_H
#define POLARSSL_BN_MUL_H

//#include "config.h"

#define MULADDC_INIT                    \
{                                       \
    t_int s0, s1, b0, b1;               \
    t_int r0, r1, rx, ry;               \
    b0 = ( b << biH ) >> biH;           \
    b1 = ( b >> biH );

#define MULADDC_CORE                    \
    s0 = ( *s << biH ) >> biH;          \
    s1 = ( *s >> biH ); s++;            \
    rx = s0 * b1; r0 = s0 * b0;         \
    ry = s1 * b0; r1 = s1 * b1;         \
    r1 += ( rx >> biH );                \
    r1 += ( ry >> biH );                \
    rx <<= biH; ry <<= biH;             \
    r0 += rx; r1 += (r0 < rx);          \
    r0 += ry; r1 += (r0 < ry);          \
    r0 +=  c; r1 += (r0 <  c);          \
    r0 += *d; r1 += (r0 < *d);          \
    c = r1; *(d++) = r0;

#define MULADDC_STOP                    \
}


#endif /* bn_mul.h */


//bignum.c

/*
 *  Multi-precision integer library
 *
 *  Copyright (C) 2006-2010, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 *  This MPI implementation is based on:
 *
 *  http://www.cacr.math.uwaterloo.ca/hac/about/chap14.pdf
 *  http://www.stillhq.com/extracted/gnupg-api/mpi/
 *  http://math.libtomcrypt.com/files/tommath.pdf
 */

//#include "config.h"



//#include "bignum.h"
//#include "bn_mul.h"


#define ciL    ((int) sizeof(t_int))    /* chars in limb  */
#define biL    (ciL << 3)               /* bits  in limb  */
#define biH    (ciL << 2)               /* half limb size */

/*
 * Convert between bits/chars and number of limbs
 */
#define BITS_TO_LIMBS(i)  (((i) + biL - 1) / biL)
#define CHARS_TO_LIMBS(i) (((i) + ciL - 1) / ciL)

/*
 * Initialize one or more mpi
 */
void mpi_init( mpi *X, ... )
{
    va_list args;

    va_start( args, X );

    while( X != NULL )
    {
        X->s = 1;
        X->n = 0;
        X->p = NULL;

        X = va_arg( args, mpi* );
    }

    va_end( args );
}

/*
 * Unallocate one or more mpi
 */
void mpi_free( mpi *X, ... )
{
    va_list args;

    va_start( args, X );

    while( X != NULL )
    {
        if( X->p != NULL )
        {
            memset( X->p, 0, X->n * ciL );
            free( X->p );
        }

        X->s = 1;
        X->n = 0;
        X->p = NULL;

        X = va_arg( args, mpi* );
    }

    va_end( args );
}

/*
 * Enlarge to the specified number of limbs
 */
int mpi_grow( mpi *X, int nblimbs )
{
    t_int *p;
    
    if( X->n < nblimbs )
    {
        if( ( p = (t_int *) malloc( nblimbs * ciL ) ) == NULL )
            return( 1 );

        memset( p, 0, nblimbs * ciL );

        if( X->p != NULL )
        {
            memcpy( p, X->p, X->n * ciL );
            memset( X->p, 0, X->n * ciL );
            free( X->p );
        }
	
        X->n = nblimbs;
        X->p = p;
    }
	//if(X->n>=10)printf("ggg:%d\t",X->n);
    return( 0 );
}

/*
 * Copy the contents of Y into X
 */
int mpi_copy( mpi *X, const mpi *Y )
{
    int ret, i;

    if( X == Y )
        return( 0 );

    for( i = Y->n - 1; i > 0; i-- )
        if( Y->p[i] != 0 )
            break;
    i++;

    X->s = Y->s;

    MPI_CHK( mpi_grow( X, i ) );

    memset( X->p, 0, X->n * ciL );
    memcpy( X->p, Y->p, i * ciL );

cleanup:

    return( ret );
}

/*
 * Swap the contents of X and Y
 */
void mpi_swap( mpi *X, mpi *Y )
{
    mpi T;

    memcpy( &T,  X, sizeof( mpi ) );
    memcpy(  X,  Y, sizeof( mpi ) );
    memcpy(  Y, &T, sizeof( mpi ) );
}

/*
 * Set value from integer
 */
int mpi_lset( mpi *X, int z )
{
    int ret;

    MPI_CHK( mpi_grow( X, 1 ) );
    memset( X->p, 0, X->n * ciL );

    X->p[0] = ( z < 0 ) ? -z : z;
    X->s    = ( z < 0 ) ? -1 : 1;

cleanup:

    return( ret );
}

/*
 * Return the number of least significant bits
 */
int mpi_lsb( const mpi *X )
{
    int i, j, count = 0;

    for( i = 0; i < X->n; i++ )
        for( j = 0; j < (int) biL; j++, count++ )
            if( ( ( X->p[i] >> j ) & 1 ) != 0 )
                return( count );

    return( 0 );
}

/*
 * Return the number of most significant bits
 */
int mpi_msb( const mpi *X )
{
    int i, j;

    for( i = X->n - 1; i > 0; i-- )
        if( X->p[i] != 0 )
            break;

    for( j = biL - 1; j >= 0; j-- )
        if( ( ( X->p[i] >> j ) & 1 ) != 0 )
            break;

    return( ( i * biL ) + j + 1 );
}

/*
 * Return the total size in bytes
 */
int mpi_size( const mpi *X )
{
    return( ( mpi_msb( X ) + 7 ) >> 3 );
}

/*
 * Convert an ASCII character to digit value
 */
static int mpi_get_digit( t_int *d, int radix, char c )
{
    *d = 255;

    if( c >= 0x30 && c <= 0x39 ) *d = c - 0x30;
    if( c >= 0x41 && c <= 0x46 ) *d = c - 0x37;
    if( c >= 0x61 && c <= 0x66 ) *d = c - 0x57;

    if( *d >= (t_int) radix )
        return( POLARSSL_ERR_MPI_INVALID_CHARACTER );

    return( 0 );
}

/*
 * Import from an ASCII string
 */
int mpi_read_string( mpi *X, int radix, const char *s )
{
    int ret, i, j, n, slen;
    t_int d;
    mpi T;

    if( radix < 2 || radix > 16 )
        return( POLARSSL_ERR_MPI_BAD_INPUT_DATA );

    mpi_init( &T, NULL );

    slen = strlen( s );

    if( radix == 16 )
    {
        n = BITS_TO_LIMBS( slen << 2 );
	
        MPI_CHK( mpi_grow( X, n ) );
        MPI_CHK( mpi_lset( X, 0 ) );

        for( i = slen - 1, j = 0; i >= 0; i--, j++ )
        {
            if( i == 0 && s[i] == '-' )
            {
                X->s = -1;
                break;
            }

            MPI_CHK( mpi_get_digit( &d, radix, s[i] ) );
            X->p[j / (2 * ciL)] |= d << ( (j % (2 * ciL)) << 2 );
        }
    }
    else
    {
        MPI_CHK( mpi_lset( X, 0 ) );

        for( i = 0; i < slen; i++ )
        {
            if( i == 0 && s[i] == '-' )
            {
                X->s = -1;
                continue;
            }

            MPI_CHK( mpi_get_digit( &d, radix, s[i] ) );
            MPI_CHK( mpi_mul_int( &T, X, radix ) );

            if( X->s == 1 )
            {
                MPI_CHK( mpi_add_int( X, &T, d ) );
            }
            else
            {
                MPI_CHK( mpi_sub_int( X, &T, d ) );
            }
        }
    }

cleanup:

    mpi_free( &T, NULL );

    return( ret );
}

/*
 * Helper to write the digits high-order first
 */
static int mpi_write_hlp( mpi *X, int radix, char **p )
{
    int ret;
    t_int r;

    if( radix < 2 || radix > 16 )
        return( POLARSSL_ERR_MPI_BAD_INPUT_DATA );

    MPI_CHK( mpi_mod_int( &r, X, radix ) );
    MPI_CHK( mpi_div_int( X, NULL, X, radix ) );

    if( mpi_cmp_int( X, 0 ) != 0 )
        MPI_CHK( mpi_write_hlp( X, radix, p ) );

    if( r < 10 )
        *(*p)++ = (char)( r + 0x30 );
    else
        *(*p)++ = (char)( r + 0x37 );

cleanup:

    return( ret );
}

/*
 * Export into an ASCII string
 */
int mpi_write_string( const mpi *X, int radix, char *s, int *slen )
{
    int ret = 0, n;
    char *p;
    mpi T;

    if( radix < 2 || radix > 16 )
        return( POLARSSL_ERR_MPI_BAD_INPUT_DATA );

    n = mpi_msb( X );
    if( radix >=  4 ) n >>= 1;
    if( radix >= 16 ) n >>= 1;
    n += 3;

    if( *slen < n )
    {
        *slen = n;
        return( POLARSSL_ERR_MPI_BUFFER_TOO_SMALL );
    }

    p = s;
    mpi_init( &T, NULL );

    if( X->s == -1 )
        *p++ = '-';

    if( radix == 16 )
    {
        int c, i, j, k;

        for( i = X->n - 1, k = 0; i >= 0; i-- )
        {
            for( j = ciL - 1; j >= 0; j-- )
            {
                c = ( X->p[i] >> (j << 3) ) & 0xFF;

                if( c == 0 && k == 0 && (i + j) != 0 )
                    continue;

                p += sprintf( p, "%02X", c );
                k = 1;
            }
        }
    }
    else
    {
        MPI_CHK( mpi_copy( &T, X ) );

        if( T.s == -1 )
            T.s = 1;

        MPI_CHK( mpi_write_hlp( &T, radix, &p ) );
    }

    *p++ = '\0';
    *slen = p - s;

cleanup:

    mpi_free( &T, NULL );

    return( ret );
}

/*
 * Read X from an opened file
 */
#if !kernel_module
int mpi_read_file( mpi *X, int radix, FILE *fin )
{
    t_int d;
    int slen;
    char *p;
    char s[1024];

    memset( s, 0, sizeof( s ) );
    if( fgets( s, sizeof( s ) - 1, fin ) == NULL )
        return( POLARSSL_ERR_MPI_FILE_IO_ERROR );

    slen = strlen( s );
    if( s[slen - 1] == '\n' ) { slen--; s[slen] = '\0'; }
    if( s[slen - 1] == '\r' ) { slen--; s[slen] = '\0'; }

    p = s + slen;
    while( --p >= s )
        if( mpi_get_digit( &d, radix, *p ) != 0 )
            break;

    return( mpi_read_string( X, radix, p + 1 ) );
}

/*
 * Write X into an opened file (or stdout if fout == NULL)
 */
int mpi_write_file( const char *p, const mpi *X, int radix, FILE *fout )
{
    int n, ret;
    size_t slen;
    size_t plen;
    char s[2048];

    n = sizeof( s );
    memset( s, 0, n );
    n -= 2;

    MPI_CHK( mpi_write_string( X, radix, s, (int *) &n ) );

    if( p == NULL ) p = "";

    plen = strlen( p );
    slen = strlen( s );
    s[slen++] = '\r';
    s[slen++] = '\n';

    if( fout != NULL )
    {
        if( fwrite( p, 1, plen, fout ) != plen ||
            fwrite( s, 1, slen, fout ) != slen )
            return( POLARSSL_ERR_MPI_FILE_IO_ERROR );
    }
    else
        printf( "%s%s", p, s );

cleanup:

    return( ret );
}
#endif
/*
 * Import X from unsigned binary data, big endian
 */
int mpi_read_binary( mpi *X, const unsigned char *buf, int buflen )
{
    int ret, i, j, n;

    for( n = 0; n < buflen; n++ )
        if( buf[n] != 0 )
            break;

    MPI_CHK( mpi_grow( X, CHARS_TO_LIMBS( buflen - n ) ) );
    MPI_CHK( mpi_lset( X, 0 ) );

    for( i = buflen - 1, j = 0; i >= n; i--, j++ )
        X->p[j / ciL] |= ((t_int) buf[i]) << ((j % ciL) << 3);

cleanup:

    return( ret );
}

/*
 * Export X into unsigned binary data, big endian
 */
int mpi_write_binary( const mpi *X, unsigned char *buf, int buflen )
{
    int i, j, n;

    n = mpi_size( X );

    if( buflen < n )
        return( POLARSSL_ERR_MPI_BUFFER_TOO_SMALL );

    memset( buf, 0, buflen );

    for( i = buflen - 1, j = 0; n > 0; i--, j++, n-- )
        buf[i] = (unsigned char)( X->p[j / ciL] >> ((j % ciL) << 3) );

    return( 0 );
}

/*
 * Left-shift: X <<= count
 */
int mpi_shift_l( mpi *X, int count )
{
    int ret, i, v0, t1;
    t_int r0 = 0, r1;

    v0 = count / (biL    );
    t1 = count & (biL - 1);

    i = mpi_msb( X ) + count;

    if( X->n * (int) biL < i )
        MPI_CHK( mpi_grow( X, BITS_TO_LIMBS( i ) ) );

    ret = 0;

    /*
     * shift by count / limb_size
     */
    if( v0 > 0 )
    {
        for( i = X->n - 1; i >= v0; i-- )
            X->p[i] = X->p[i - v0];

        for( ; i >= 0; i-- )
            X->p[i] = 0;
    }

    /*
     * shift by count % limb_size
     */
    if( t1 > 0 )
    {
        for( i = v0; i < X->n; i++ )
        {
            r1 = X->p[i] >> (biL - t1);
            X->p[i] <<= t1;
            X->p[i] |= r0;
            r0 = r1;
        }
    }

cleanup:

    return( ret );
}

/*
 * Right-shift: X >>= count
 */
int mpi_shift_r( mpi *X, int count )
{
    int i, v0, v1;
    t_int r0 = 0, r1;

    v0 = count /  biL;
    v1 = count & (biL - 1);

    /*
     * shift by count / limb_size
     */
    if( v0 > 0 )
    {
        for( i = 0; i < X->n - v0; i++ )
            X->p[i] = X->p[i + v0];

        for( ; i < X->n; i++ )
            X->p[i] = 0;
    }

    /*
     * shift by count % limb_size
     */
    if( v1 > 0 )
    {
        for( i = X->n - 1; i >= 0; i-- )
        {
            r1 = X->p[i] << (biL - v1);
            X->p[i] >>= v1;
            X->p[i] |= r0;
            r0 = r1;
        }
    }

    return( 0 );
}

/*
 * Compare unsigned values
 */
int mpi_cmp_abs( const mpi *X, const mpi *Y )
{
    int i, j;

    for( i = X->n - 1; i >= 0; i-- )
        if( X->p[i] != 0 )
            break;

    for( j = Y->n - 1; j >= 0; j-- )
        if( Y->p[j] != 0 )
            break;

    if( i < 0 && j < 0 )
        return( 0 );

    if( i > j ) return(  1 );
    if( j > i ) return( -1 );

    for( ; i >= 0; i-- )
    {
        if( X->p[i] > Y->p[i] ) return(  1 );
        if( X->p[i] < Y->p[i] ) return( -1 );
    }

    return( 0 );
}

/*
 * Compare signed values
 */
int mpi_cmp_mpi( const mpi *X, const mpi *Y )
{
    int i, j;

    for( i = X->n - 1; i >= 0; i-- )
        if( X->p[i] != 0 )
            break;

    for( j = Y->n - 1; j >= 0; j-- )
        if( Y->p[j] != 0 )
            break;

    if( i < 0 && j < 0 )
        return( 0 );

    if( i > j ) return(  X->s );
    if( j > i ) return( -X->s );

    if( X->s > 0 && Y->s < 0 ) return(  1 );
    if( Y->s > 0 && X->s < 0 ) return( -1 );

    for( ; i >= 0; i-- )
    {
        if( X->p[i] > Y->p[i] ) return(  X->s );
        if( X->p[i] < Y->p[i] ) return( -X->s );
    }

    return( 0 );
}

/*
 * Compare signed values
 */
int mpi_cmp_int( const mpi *X, int z )
{
    mpi Y;
    t_int p[1];

    *p  = ( z < 0 ) ? -z : z;
    Y.s = ( z < 0 ) ? -1 : 1;
    Y.n = 1;
    Y.p = p;

    return( mpi_cmp_mpi( X, &Y ) );
}

/*
 * Unsigned addition: X = |A| + |B|  (HAC 14.7)
 */
int mpi_add_abs( mpi *X, const mpi *A, const mpi *B )
{
    int ret, i, j;
    t_int *o, *p, c;

    if( X == B )
    {
        const mpi *T = A; A = X; B = T;
    }

    if( X != A )
        MPI_CHK( mpi_copy( X, A ) );
   
    /*
     * X should always be positive as a result of unsigned additions.
     */
    X->s = 1;

    for( j = B->n - 1; j >= 0; j-- )
        if( B->p[j] != 0 )
            break;

    MPI_CHK( mpi_grow( X, j + 1 ) );

    o = B->p; p = X->p; c = 0;

    for( i = 0; i <= j; i++, o++, p++ )
    {
        *p +=  c; c  = ( *p <  c );
        *p += *o; c += ( *p < *o );
    }

    while( c != 0 )
    {
        if( i >= X->n )
        {
            MPI_CHK( mpi_grow( X, i + 1 ) );
            p = X->p + i;
        }

        *p += c; c = ( *p < c ); i++;
    }

cleanup:

    return( ret );
}

/*
 * Helper for mpi substraction
 */
static void mpi_sub_hlp( int n, t_int *s, t_int *d )
{
    int i;
    t_int c, z;

    for( i = c = 0; i < n; i++, s++, d++ )
    {
        z = ( *d <  c );     *d -=  c;
        c = ( *d < *s ) + z; *d -= *s;
    }

    while( c != 0 )
    {
        z = ( *d < c ); *d -= c;
        c = z; i++; d++;
    }
}

/*
 * Unsigned substraction: X = |A| - |B|  (HAC 14.9)
 */
int mpi_sub_abs( mpi *X, const mpi *A, const mpi *B )
{
    mpi TB;
    int ret, n;

    if( mpi_cmp_abs( A, B ) < 0 )
        return( POLARSSL_ERR_MPI_NEGATIVE_VALUE );

    mpi_init( &TB, NULL );

    if( X == B )
    {
        MPI_CHK( mpi_copy( &TB, B ) );
        B = &TB;
    }

    if( X != A )
        MPI_CHK( mpi_copy( X, A ) );

    /*
     * X should always be positive as a result of unsigned substractions.
     */
    X->s = 1;

    ret = 0;

    for( n = B->n - 1; n >= 0; n-- )
        if( B->p[n] != 0 )
            break;

    mpi_sub_hlp( n + 1, B->p, X->p );

cleanup:

    mpi_free( &TB, NULL );

    return( ret );
}

/*
 * Signed addition: X = A + B
 */
int mpi_add_mpi( mpi *X, const mpi *A, const mpi *B )
{
    int ret, s = A->s;

    if( A->s * B->s < 0 )
    {
        if( mpi_cmp_abs( A, B ) >= 0 )
        {
            MPI_CHK( mpi_sub_abs( X, A, B ) );
            X->s =  s;
        }
        else
        {
            MPI_CHK( mpi_sub_abs( X, B, A ) );
            X->s = -s;
        }
    }
    else
    {
        MPI_CHK( mpi_add_abs( X, A, B ) );
        X->s = s;
    }

cleanup:

    return( ret );
}

/*
 * Signed substraction: X = A - B
 */
int mpi_sub_mpi( mpi *X, const mpi *A, const mpi *B )
{
    int ret, s = A->s;

    if( A->s * B->s > 0 )
    {
        if( mpi_cmp_abs( A, B ) >= 0 )
        {
            MPI_CHK( mpi_sub_abs( X, A, B ) );
            X->s =  s;
        }
        else
        {
            MPI_CHK( mpi_sub_abs( X, B, A ) );
            X->s = -s;
        }
    }
    else
    {
        MPI_CHK( mpi_add_abs( X, A, B ) );
        X->s = s;
    }

cleanup:

    return( ret );
}

/*
 * Signed addition: X = A + b
 */
int mpi_add_int( mpi *X, const mpi *A, int b )
{
    mpi _B;
    t_int p[1];

    p[0] = ( b < 0 ) ? -b : b;
    _B.s = ( b < 0 ) ? -1 : 1;
    _B.n = 1;
    _B.p = p;

    return( mpi_add_mpi( X, A, &_B ) );
}

/*
 * Signed substraction: X = A - b
 */
int mpi_sub_int( mpi *X, const mpi *A, int b )
{
    mpi _B;
    t_int p[1];

    p[0] = ( b < 0 ) ? -b : b;
    _B.s = ( b < 0 ) ? -1 : 1;
    _B.n = 1;
    _B.p = p;

    return( mpi_sub_mpi( X, A, &_B ) );
}

/*
 * Helper for mpi multiplication
 */ 
static void mpi_mul_hlp( int i, t_int *s, t_int *d, t_int b )
{
    t_int c = 0, t = 0;

    for( ; i >= 16; i -= 16 )
    {
        MULADDC_INIT
        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE

        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE
        MULADDC_STOP
    }

    for( ; i >= 8; i -= 8 )
    {
        MULADDC_INIT
        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE

        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE
        MULADDC_STOP
    }

    for( ; i > 0; i-- )
    {
        MULADDC_INIT
        MULADDC_CORE
        MULADDC_STOP
    }

    t++;

    do {
        *d += c; c = ( *d < c ); d++;
    }
    while( c != 0 );
}

/*
 * Baseline multiplication: X = A * B  (HAC 14.12)
 */
int mpi_mul_mpi( mpi *X, const mpi *A, const mpi *B )
{
    int ret, i, j;
    mpi TA, TB;

    mpi_init( &TA, &TB, NULL );

    if( X == A ) { MPI_CHK( mpi_copy( &TA, A ) ); A = &TA; }
    if( X == B ) { MPI_CHK( mpi_copy( &TB, B ) ); B = &TB; }

    for( i = A->n - 1; i >= 0; i-- )
        if( A->p[i] != 0 )
            break;

    for( j = B->n - 1; j >= 0; j-- )
        if( B->p[j] != 0 )
            break;

    MPI_CHK( mpi_grow( X, i + j + 2 ) );
    MPI_CHK( mpi_lset( X, 0 ) );

    for( i++; j >= 0; j-- )
        mpi_mul_hlp( i, A->p, X->p + j, B->p[j] );

    X->s = A->s * B->s;

cleanup:

    mpi_free( &TB, &TA, NULL );

    return( ret );
}

/*
 * Baseline multiplication: X = A * b
 */
int mpi_mul_int( mpi *X, const mpi *A, t_int b )
{
    mpi _B;
    t_int p[1];

    _B.s = 1;
    _B.n = 1;
    _B.p = p;
    p[0] = b;

    return( mpi_mul_mpi( X, A, &_B ) );
}

/*
 * Division by mpi: A = Q * B + R  (HAC 14.20)
 */
int mpi_div_mpi( mpi *Q, mpi *R, const mpi *A, const mpi *B )
{
    int ret, i, n, t, k;
    mpi X, Y, Z, T1, T2;

    if( mpi_cmp_int( B, 0 ) == 0 )
        return( POLARSSL_ERR_MPI_DIVISION_BY_ZERO );

    mpi_init( &X, &Y, &Z, &T1, &T2, NULL );

    if( mpi_cmp_abs( A, B ) < 0 )
    {
        if( Q != NULL ) MPI_CHK( mpi_lset( Q, 0 ) );
        if( R != NULL ) MPI_CHK( mpi_copy( R, A ) );
        return( 0 );
    }

    MPI_CHK( mpi_copy( &X, A ) );
    MPI_CHK( mpi_copy( &Y, B ) );
    X.s = Y.s = 1;

    MPI_CHK( mpi_grow( &Z, A->n + 2 ) );
    MPI_CHK( mpi_lset( &Z,  0 ) );
    MPI_CHK( mpi_grow( &T1, 2 ) );
    MPI_CHK( mpi_grow( &T2, 3 ) );

    k = mpi_msb( &Y ) % biL;
    if( k < (int) biL - 1 )
    {
        k = biL - 1 - k;
        MPI_CHK( mpi_shift_l( &X, k ) );
        MPI_CHK( mpi_shift_l( &Y, k ) );
    }
    else k = 0;

    n = X.n - 1;
    t = Y.n - 1;
    mpi_shift_l( &Y, biL * (n - t) );

    while( mpi_cmp_mpi( &X, &Y ) >= 0 )
    {
        Z.p[n - t]++;
        mpi_sub_mpi( &X, &X, &Y );
    }
    mpi_shift_r( &Y, biL * (n - t) );

    for( i = n; i > t ; i-- )
    {
        if( X.p[i] >= Y.p[t] )
            Z.p[i - t - 1] = ~0;
        else
        {
            /*
             * __udiv_qrnnd_c, from gmp/longlong.h
             */
            t_int q0, q1, r0, r1;
            t_int d0, d1, d, m;

            d  = Y.p[t];
            d0 = ( d << biH ) >> biH;
            d1 = ( d >> biH );

            q1 = X.p[i] / d1;
            r1 = X.p[i] - d1 * q1;
            r1 <<= biH;
            r1 |= ( X.p[i - 1] >> biH );

            m = q1 * d0;
            if( r1 < m )
            {
                q1--, r1 += d;
                while( r1 >= d && r1 < m )
                    q1--, r1 += d;
            }
            r1 -= m;

            q0 = r1 / d1;
            r0 = r1 - d1 * q0;
            r0 <<= biH;
            r0 |= ( X.p[i - 1] << biH ) >> biH;

            m = q0 * d0;
            if( r0 < m )
            {
                q0--, r0 += d;
                while( r0 >= d && r0 < m )
                    q0--, r0 += d;
            }
            r0 -= m;

            Z.p[i - t - 1] = ( q1 << biH ) | q0;

        }

        Z.p[i - t - 1]++;
        do
        {
            Z.p[i - t - 1]--;

            MPI_CHK( mpi_lset( &T1, 0 ) );
            T1.p[0] = (t < 1) ? 0 : Y.p[t - 1];
            T1.p[1] = Y.p[t];
            MPI_CHK( mpi_mul_int( &T1, &T1, Z.p[i - t - 1] ) );

            MPI_CHK( mpi_lset( &T2, 0 ) );
            T2.p[0] = (i < 2) ? 0 : X.p[i - 2];
            T2.p[1] = (i < 1) ? 0 : X.p[i - 1];
            T2.p[2] = X.p[i];
        }
        while( mpi_cmp_mpi( &T1, &T2 ) > 0 );

        MPI_CHK( mpi_mul_int( &T1, &Y, Z.p[i - t - 1] ) );
        MPI_CHK( mpi_shift_l( &T1,  biL * (i - t - 1) ) );
        MPI_CHK( mpi_sub_mpi( &X, &X, &T1 ) );

        if( mpi_cmp_int( &X, 0 ) < 0 )
        {
            MPI_CHK( mpi_copy( &T1, &Y ) );
            MPI_CHK( mpi_shift_l( &T1, biL * (i - t - 1) ) );
            MPI_CHK( mpi_add_mpi( &X, &X, &T1 ) );
            Z.p[i - t - 1]--;
        }
    }

    if( Q != NULL )
    {
        mpi_copy( Q, &Z );
        Q->s = A->s * B->s;
    }

    if( R != NULL )
    {
        mpi_shift_r( &X, k );
        mpi_copy( R, &X );

        R->s = A->s;
        if( mpi_cmp_int( R, 0 ) == 0 )
            R->s = 1;
    }

cleanup:

    mpi_free( &X, &Y, &Z, &T1, &T2, NULL );

    return( ret );
}

/*
 * Division by int: A = Q * b + R
 *
 * Returns 0 if successful
 *         1 if memory allocation failed
 *         POLARSSL_ERR_MPI_DIVISION_BY_ZERO if b == 0
 */
int mpi_div_int( mpi *Q, mpi *R, const mpi *A, int b )
{
    mpi _B;
    t_int p[1];

    p[0] = ( b < 0 ) ? -b : b;
    _B.s = ( b < 0 ) ? -1 : 1;
    _B.n = 1;
    _B.p = p;

    return( mpi_div_mpi( Q, R, A, &_B ) );
}

/*
 * Modulo: R = A mod B
 */
int mpi_mod_mpi( mpi *R, const mpi *A, const mpi *B )
{
    int ret;

    if( mpi_cmp_int( B, 0 ) < 0 )
        return POLARSSL_ERR_MPI_NEGATIVE_VALUE;

    MPI_CHK( mpi_div_mpi( NULL, R, A, B ) );

    while( mpi_cmp_int( R, 0 ) < 0 )
      MPI_CHK( mpi_add_mpi( R, R, B ) );

    while( mpi_cmp_mpi( R, B ) >= 0 )
      MPI_CHK( mpi_sub_mpi( R, R, B ) );

cleanup:

    return( ret );
}

/*
 * Modulo: r = A mod b
 */
int mpi_mod_int( t_int *r, const mpi *A, int b )
{
    int i;
    t_int x, y, z;

    if( b == 0 )
        return( POLARSSL_ERR_MPI_DIVISION_BY_ZERO );

    if( b < 0 )
        return POLARSSL_ERR_MPI_NEGATIVE_VALUE;

    /*
     * handle trivial cases
     */
    if( b == 1 )
    {
        *r = 0;
        return( 0 );
    }

    if( b == 2 )
    {
        *r = A->p[0] & 1;
        return( 0 );
    }

    /*
     * general case
     */
    for( i = A->n - 1, y = 0; i >= 0; i-- )
    {
        x  = A->p[i];
        y  = ( y << biH ) | ( x >> biH );
        z  = y / b;
        y -= z * b;

        x <<= biH;
        y  = ( y << biH ) | ( x >> biH );
        z  = y / b;
        y -= z * b;
    }

    /*
     * If A is negative, then the current y represents a negative value.
     * Flipping it to the positive side.
     */
    if( A->s < 0 && y != 0 )
        y = b - y;

    *r = y;

    return( 0 );
}

/*
 * Fast Montgomery initialization (thanks to Tom St Denis)
 */
static void mpi_montg_init( t_int *mm, const mpi *N )
{
    t_int x, m0 = N->p[0];

    x  = m0;
    x += ( ( m0 + 2 ) & 4 ) << 1;
    x *= ( 2 - ( m0 * x ) );

    if( biL >= 16 ) x *= ( 2 - ( m0 * x ) );
    if( biL >= 32 ) x *= ( 2 - ( m0 * x ) );
    if( biL >= 64 ) x *= ( 2 - ( m0 * x ) );

    *mm = ~x + 1;
}

/*
 * Montgomery multiplication: A = A * B * R^-1 mod N  (HAC 14.36)
 */
static void mpi_montmul( mpi *A, const mpi *B, const mpi *N, t_int mm, const mpi *T )
{
    int i, n, m;
    t_int u0, u1, *d;

    memset( T->p, 0, T->n * ciL );

    d = T->p;
    n = N->n;
    m = ( B->n < n ) ? B->n : n;

    for( i = 0; i < n; i++ )
    {
        /*
         * T = (T + u0*B + u1*N) / 2^biL
         */
        u0 = A->p[i];
        u1 = ( d[0] + u0 * B->p[0] ) * mm;

        mpi_mul_hlp( m, B->p, d, u0 );
        mpi_mul_hlp( n, N->p, d, u1 );

        *d++ = u0; d[n + 1] = 0;
    }

    memcpy( A->p, d, (n + 1) * ciL );

    if( mpi_cmp_abs( A, N ) >= 0 )
        mpi_sub_hlp( n, N->p, A->p );
    else
        /* prevent timing attacks */
        mpi_sub_hlp( n, A->p, T->p );
}

/*
 * Montgomery reduction: A = A * R^-1 mod N
 */
static void mpi_montred( mpi *A, const mpi *N, t_int mm, const mpi *T )
{
    t_int z = 1;
    mpi U;

    U.n = U.s = z;
    U.p = &z;

    mpi_montmul( A, &U, N, mm, T );
}

/*
 * Sliding-window exponentiation: X = A^E mod N  (HAC 14.85)
 */
int mpi_exp_mod( mpi *X, const mpi *A, const mpi *E, const mpi *N, mpi *_RR )
{
    int ret, i, j, wsize, wbits;
    int bufsize, nblimbs, nbits;
    t_int ei, mm, state;
    mpi RR, T, W[64];

    if( mpi_cmp_int( N, 0 ) < 0 || ( N->p[0] & 1 ) == 0 )
        return( POLARSSL_ERR_MPI_BAD_INPUT_DATA );

    /*
     * Init temps and window size
     */
    mpi_montg_init( &mm, N );
    mpi_init( &RR, &T, NULL );
    memset( W, 0, sizeof( W ) );

    i = mpi_msb( E );

    wsize = ( i > 671 ) ? 6 : ( i > 239 ) ? 5 :
            ( i >  79 ) ? 4 : ( i >  23 ) ? 3 : 1;

    j = N->n + 1;
    MPI_CHK( mpi_grow( X, j ) );
    MPI_CHK( mpi_grow( &W[1],  j ) );
    MPI_CHK( mpi_grow( &T, j * 2 ) );

    /*
     * If 1st call, pre-compute R^2 mod N
     */
    if( _RR == NULL || _RR->p == NULL )
    {
        MPI_CHK( mpi_lset( &RR, 1 ) );
        MPI_CHK( mpi_shift_l( &RR, N->n * 2 * biL ) );
        MPI_CHK( mpi_mod_mpi( &RR, &RR, N ) );

        if( _RR != NULL )
            memcpy( _RR, &RR, sizeof( mpi ) );
    }
    else
        memcpy( &RR, _RR, sizeof( mpi ) );

    /*
     * W[1] = A * R^2 * R^-1 mod N = A * R mod N
     */
    if( mpi_cmp_mpi( A, N ) >= 0 )
        mpi_mod_mpi( &W[1], A, N );
    else   mpi_copy( &W[1], A );

    mpi_montmul( &W[1], &RR, N, mm, &T );

    /*
     * X = R^2 * R^-1 mod N = R mod N
     */
    MPI_CHK( mpi_copy( X, &RR ) );
    mpi_montred( X, N, mm, &T );

    if( wsize > 1 )
    {
        /*
         * W[1 << (wsize - 1)] = W[1] ^ (wsize - 1)
         */
        j =  1 << (wsize - 1);

        MPI_CHK( mpi_grow( &W[j], N->n + 1 ) );
        MPI_CHK( mpi_copy( &W[j], &W[1]    ) );

        for( i = 0; i < wsize - 1; i++ )
            mpi_montmul( &W[j], &W[j], N, mm, &T );
    
        /*
         * W[i] = W[i - 1] * W[1]
         */
        for( i = j + 1; i < (1 << wsize); i++ )
        {
            MPI_CHK( mpi_grow( &W[i], N->n + 1 ) );
            MPI_CHK( mpi_copy( &W[i], &W[i - 1] ) );

            mpi_montmul( &W[i], &W[1], N, mm, &T );
        }
    }

    nblimbs = E->n;
    bufsize = 0;
    nbits   = 0;
    wbits   = 0;
    state   = 0;

    while( 1 )
    {
        if( bufsize == 0 )
        {
            if( nblimbs-- == 0 )
                break;

            bufsize = sizeof( t_int ) << 3;
        }

        bufsize--;

        ei = (E->p[nblimbs] >> bufsize) & 1;

        /*
         * skip leading 0s
         */
        if( ei == 0 && state == 0 )
            continue;

        if( ei == 0 && state == 1 )
        {
            /*
             * out of window, square X
             */
            mpi_montmul( X, X, N, mm, &T );
            continue;
        }

        /*
         * add ei to current window
         */
        state = 2;

        nbits++;
        wbits |= (ei << (wsize - nbits));

        if( nbits == wsize )
        {
            /*
             * X = X^wsize R^-1 mod N
             */
            for( i = 0; i < wsize; i++ )
                mpi_montmul( X, X, N, mm, &T );

            /*
             * X = X * W[wbits] R^-1 mod N
             */
            mpi_montmul( X, &W[wbits], N, mm, &T );

            state--;
            nbits = 0;
            wbits = 0;
        }
    }

    /*
     * process the remaining bits
     */
    for( i = 0; i < nbits; i++ )
    {
        mpi_montmul( X, X, N, mm, &T );

        wbits <<= 1;

        if( (wbits & (1 << wsize)) != 0 )
            mpi_montmul( X, &W[1], N, mm, &T );
    }

    /*
     * X = A^E * R * R^-1 mod N = A^E mod N
     */
    mpi_montred( X, N, mm, &T );

cleanup:

    for( i = (1 << (wsize - 1)); i < (1 << wsize); i++ )
        mpi_free( &W[i], NULL );

    if( _RR != NULL )
         mpi_free( &W[1], &T, NULL );
    else mpi_free( &W[1], &T, &RR, NULL );

    return( ret );
}

/*
 * Greatest common divisor: G = gcd(A, B)  (HAC 14.54)
 */
int mpi_gcd( mpi *G, const mpi *A, const mpi *B )
{
    int ret, lz, lzt;
    mpi TG, TA, TB;

    mpi_init( &TG, &TA, &TB, NULL );

    MPI_CHK( mpi_copy( &TA, A ) );
    MPI_CHK( mpi_copy( &TB, B ) );

    lz = mpi_lsb( &TA );
    lzt = mpi_lsb( &TB );

    if ( lzt < lz )
        lz = lzt;

    MPI_CHK( mpi_shift_r( &TA, lz ) );
    MPI_CHK( mpi_shift_r( &TB, lz ) );

    TA.s = TB.s = 1;

    while( mpi_cmp_int( &TA, 0 ) != 0 )
    {
        MPI_CHK( mpi_shift_r( &TA, mpi_lsb( &TA ) ) );
        MPI_CHK( mpi_shift_r( &TB, mpi_lsb( &TB ) ) );

        if( mpi_cmp_mpi( &TA, &TB ) >= 0 )
        {
            MPI_CHK( mpi_sub_abs( &TA, &TA, &TB ) );
            MPI_CHK( mpi_shift_r( &TA, 1 ) );
        }
        else
        {
            MPI_CHK( mpi_sub_abs( &TB, &TB, &TA ) );
            MPI_CHK( mpi_shift_r( &TB, 1 ) );
        }
    }

    MPI_CHK( mpi_shift_l( &TB, lz ) );
    MPI_CHK( mpi_copy( G, &TB ) );

cleanup:

    mpi_free( &TB, &TA, &TG, NULL );

    return( ret );
}


/*
 * Modular inverse: X = A^-1 mod N  (HAC 14.61 / 14.64)
 */
int mpi_inv_mod( mpi *X, const mpi *A, const mpi *N )
{
    int ret;
    mpi G, TA, TU, U1, U2, TB, TV, V1, V2;

    if( mpi_cmp_int( N, 0 ) <= 0 )
        return( POLARSSL_ERR_MPI_BAD_INPUT_DATA );

    mpi_init( &TA, &TU, &U1, &U2, &G,
              &TB, &TV, &V1, &V2, NULL );

    MPI_CHK( mpi_gcd( &G, A, N ) );

    if( mpi_cmp_int( &G, 1 ) != 0 )
    {
        ret = POLARSSL_ERR_MPI_NOT_ACCEPTABLE;
        goto cleanup;
    }

    MPI_CHK( mpi_mod_mpi( &TA, A, N ) );
    MPI_CHK( mpi_copy( &TU, &TA ) );
    MPI_CHK( mpi_copy( &TB, N ) );
    MPI_CHK( mpi_copy( &TV, N ) );

    MPI_CHK( mpi_lset( &U1, 1 ) );
    MPI_CHK( mpi_lset( &U2, 0 ) );
    MPI_CHK( mpi_lset( &V1, 0 ) );
    MPI_CHK( mpi_lset( &V2, 1 ) );

    do
    {
        while( ( TU.p[0] & 1 ) == 0 )
        {
            MPI_CHK( mpi_shift_r( &TU, 1 ) );

            if( ( U1.p[0] & 1 ) != 0 || ( U2.p[0] & 1 ) != 0 )
            {
                MPI_CHK( mpi_add_mpi( &U1, &U1, &TB ) );
                MPI_CHK( mpi_sub_mpi( &U2, &U2, &TA ) );
            }

            MPI_CHK( mpi_shift_r( &U1, 1 ) );
            MPI_CHK( mpi_shift_r( &U2, 1 ) );
        }

        while( ( TV.p[0] & 1 ) == 0 )
        {
            MPI_CHK( mpi_shift_r( &TV, 1 ) );

            if( ( V1.p[0] & 1 ) != 0 || ( V2.p[0] & 1 ) != 0 )
            {
                MPI_CHK( mpi_add_mpi( &V1, &V1, &TB ) );
                MPI_CHK( mpi_sub_mpi( &V2, &V2, &TA ) );
            }

            MPI_CHK( mpi_shift_r( &V1, 1 ) );
            MPI_CHK( mpi_shift_r( &V2, 1 ) );
        }

        if( mpi_cmp_mpi( &TU, &TV ) >= 0 )
        {
            MPI_CHK( mpi_sub_mpi( &TU, &TU, &TV ) );
            MPI_CHK( mpi_sub_mpi( &U1, &U1, &V1 ) );
            MPI_CHK( mpi_sub_mpi( &U2, &U2, &V2 ) );
        }
        else
        {
            MPI_CHK( mpi_sub_mpi( &TV, &TV, &TU ) );
            MPI_CHK( mpi_sub_mpi( &V1, &V1, &U1 ) );
            MPI_CHK( mpi_sub_mpi( &V2, &V2, &U2 ) );
        }
    }
    while( mpi_cmp_int( &TU, 0 ) != 0 );

    while( mpi_cmp_int( &V1, 0 ) < 0 )
        MPI_CHK( mpi_add_mpi( &V1, &V1, N ) );

    while( mpi_cmp_mpi( &V1, N ) >= 0 )
        MPI_CHK( mpi_sub_mpi( &V1, &V1, N ) );

    MPI_CHK( mpi_copy( X, &V1 ) );

cleanup:

    mpi_free( &V2, &V1, &TV, &TB, &G,
              &U2, &U1, &TU, &TA, NULL );

    return( ret );
}

static const int small_prime[] =
{
        3,    5,    7,   11,   13,   17,   19,   23,
       29,   31,   37,   41,   43,   47,   53,   59,
       61,   67,   71,   73,   79,   83,   89,   97,
      101,  103,  107,  109,  113,  127,  131,  137,
      139,  149,  151,  157,  163,  167,  173,  179,
      181,  191,  193,  197,  199,  211,  223,  227,
      229,  233,  239,  241,  251,  257,  263,  269,
      271,  277,  281,  283,  293,  307,  311,  313,
      317,  331,  337,  347,  349,  353,  359,  367,
      373,  379,  383,  389,  397,  401,  409,  419,
      421,  431,  433,  439,  443,  449,  457,  461,
      463,  467,  479,  487,  491,  499,  503,  509,
      521,  523,  541,  547,  557,  563,  569,  571,
      577,  587,  593,  599,  601,  607,  613,  617,
      619,  631,  641,  643,  647,  653,  659,  661,
      673,  677,  683,  691,  701,  709,  719,  727,
      733,  739,  743,  751,  757,  761,  769,  773,
      787,  797,  809,  811,  821,  823,  827,  829,
      839,  853,  857,  859,  863,  877,  881,  883,
      887,  907,  911,  919,  929,  937,  941,  947,
      953,  967,  971,  977,  983,  991,  997, -103
};

/*
 * Miller-Rabin primality test  (HAC 4.24)
 */
int mpi_is_prime( mpi *X, int (*f_rng)(void *), void *p_rng )
{
    int ret, i, j, n, s, xs;
    mpi W, R, T, A, RR;
    unsigned char *p;

    if( mpi_cmp_int( X, 0 ) == 0 ||
        mpi_cmp_int( X, 1 ) == 0 )
        return( POLARSSL_ERR_MPI_NOT_ACCEPTABLE );

    if( mpi_cmp_int( X, 2 ) == 0 )
        return( 0 );

    mpi_init( &W, &R, &T, &A, &RR, NULL );

    xs = X->s; X->s = 1;

    /*
     * test trivial factors first
     */
    if( ( X->p[0] & 1 ) == 0 )
        return( POLARSSL_ERR_MPI_NOT_ACCEPTABLE );

    for( i = 0; small_prime[i] > 0; i++ )
    {
        t_int r;

        if( mpi_cmp_int( X, small_prime[i] ) <= 0 )
            return( 0 );

        MPI_CHK( mpi_mod_int( &r, X, small_prime[i] ) );

        if( r == 0 )
            return( POLARSSL_ERR_MPI_NOT_ACCEPTABLE );
    }

    /*
     * W = |X| - 1
     * R = W >> lsb( W )
     */
    MPI_CHK( mpi_sub_int( &W, X, 1 ) );
    s = mpi_lsb( &W );
    MPI_CHK( mpi_copy( &R, &W ) );
    MPI_CHK( mpi_shift_r( &R, s ) );

    i = mpi_msb( X );
    /*
     * HAC, table 4.4
     */
    n = ( ( i >= 1300 ) ?  2 : ( i >=  850 ) ?  3 :
          ( i >=  650 ) ?  4 : ( i >=  350 ) ?  8 :
          ( i >=  250 ) ? 12 : ( i >=  150 ) ? 18 : 27 );

    for( i = 0; i < n; i++ )
    {
        /*
         * pick a random A, 1 < A < |X| - 1
         */
        MPI_CHK( mpi_grow( &A, X->n ) );

        p = (unsigned char *) A.p;
        for( j = 0; j < A.n * ciL; j++ )
            *p++ = (unsigned char) f_rng( p_rng );

        j = mpi_msb( &A ) - mpi_msb( &W );
        MPI_CHK( mpi_shift_r( &A, j + 1 ) );
        A.p[0] |= 3;

        /*
         * A = A^R mod |X|
         */
        MPI_CHK( mpi_exp_mod( &A, &A, &R, X, &RR ) );

        if( mpi_cmp_mpi( &A, &W ) == 0 ||
            mpi_cmp_int( &A,  1 ) == 0 )
            continue;

        j = 1;
        while( j < s && mpi_cmp_mpi( &A, &W ) != 0 )
        {
            /*
             * A = A * A mod |X|
             */
            MPI_CHK( mpi_mul_mpi( &T, &A, &A ) );
            MPI_CHK( mpi_mod_mpi( &A, &T, X  ) );

            if( mpi_cmp_int( &A, 1 ) == 0 )
                break;

            j++;
        }

        /*
         * not prime if A != |X| - 1 or A == 1
         */
        if( mpi_cmp_mpi( &A, &W ) != 0 ||
            mpi_cmp_int( &A,  1 ) == 0 )
        {
            ret = POLARSSL_ERR_MPI_NOT_ACCEPTABLE;
            break;
        }
    }

cleanup:

    X->s = xs;

    mpi_free( &RR, &A, &T, &R, &W, NULL );

    return( ret );
}

/*
 * Prime number generation
 */
int mpi_gen_prime( mpi *X, int nbits, int dh_flag,
                   int (*f_rng)(void *), void *p_rng )
{
    int ret, k, n;
    unsigned char *p;
    mpi Y;

    if( nbits < 3 )
        return( POLARSSL_ERR_MPI_BAD_INPUT_DATA );

    mpi_init( &Y, NULL );

    n = BITS_TO_LIMBS( nbits );

    MPI_CHK( mpi_grow( X, n ) );
    MPI_CHK( mpi_lset( X, 0 ) );

    p = (unsigned char *) X->p;
    for( k = 0; k < X->n * ciL; k++ )
        *p++ = (unsigned char) f_rng( p_rng );

    k = mpi_msb( X );
    if( k < nbits ) MPI_CHK( mpi_shift_l( X, nbits - k ) );
    if( k > nbits ) MPI_CHK( mpi_shift_r( X, k - nbits ) );

    X->p[0] |= 3;

    if( dh_flag == 0 )
    {
        while( ( ret = mpi_is_prime( X, f_rng, p_rng ) ) != 0 )
        {
            if( ret != POLARSSL_ERR_MPI_NOT_ACCEPTABLE )
                goto cleanup;

            MPI_CHK( mpi_add_int( X, X, 2 ) );
        }
    }
    else
    {
        MPI_CHK( mpi_sub_int( &Y, X, 1 ) );
        MPI_CHK( mpi_shift_r( &Y, 1 ) );

        while( 1 )
        {
            if( ( ret = mpi_is_prime( X, f_rng, p_rng ) ) == 0 )
            {
                if( ( ret = mpi_is_prime( &Y, f_rng, p_rng ) ) == 0 )
                    break;

                if( ret != POLARSSL_ERR_MPI_NOT_ACCEPTABLE )
                    goto cleanup;
            }

            if( ret != POLARSSL_ERR_MPI_NOT_ACCEPTABLE )
                goto cleanup;

            MPI_CHK( mpi_add_int( &Y, X, 1 ) );
            MPI_CHK( mpi_add_int(  X, X, 2 ) );
            MPI_CHK( mpi_shift_r( &Y, 1 ) );
        }
    }

cleanup:

    mpi_free( &Y, NULL );

    return( ret );
}





/*
 *  Portable interface to the CPU cycle counter
 *
 *  Copyright (C) 2006-2010, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

//#include "config.h"
//#define POLARSSL_TIMING_C



//#include "timing.h"
//timing.h
/**
 * \file timing.h
 *
 *  Copyright (C) 2006-2010, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef POLARSSL_TIMING_H
#define POLARSSL_TIMING_H

/**
 * \brief          timer structure
 */
struct hr_time
{
    unsigned char opaque[32];
};


extern int alarmed;

/**
 * \brief          Return the CPU cycle counter value
 */
unsigned long hardclock( void );

/**
 * \brief          Return the elapsed time in milliseconds
 *
 * \param val      points to a timer structure
 * \param reset    if set to 1, the timer is restarted
 */
unsigned long get_timer( struct hr_time *val, int reset );

/**
 * \brief          Setup an alarm clock
 *
 * \param seconds  delay before the "alarmed" flag is set
 */
void set_alarm( int seconds );

/**
 * \brief          Sleep for a certain amount of time
 *
 * \param          Delay in milliseconds
 */
void m_sleep( int milliseconds );


#endif /* timing.h */



//timing.c
struct _hr_time
{
    struct timeval start;
};

static int hardclock_init = 0;
static struct timeval tv_init;

unsigned long hardclock( void )
{
    struct timeval tv_cur;

    if( hardclock_init == 0 )
    {
        gettimeofday( &tv_init,0 );
        hardclock_init = 1;
    }

    gettimeofday( &tv_cur,0);
    return( ( tv_cur.tv_sec  - tv_init.tv_sec  ) * 1000000
          + ( tv_cur.tv_usec - tv_init.tv_usec ) );
}

int alarmed = 0;

#if !kernel_module
unsigned long get_timer( struct hr_time *val, int reset )
{
    unsigned long delta;
    struct timeval offset;
    struct _hr_time *t = (struct _hr_time *) val;

    gettimeofday( &offset, NULL );

    delta = ( offset.tv_sec  - t->start.tv_sec  ) * 1000
          + ( offset.tv_usec - t->start.tv_usec ) / 1000;

    if( reset )
    {
        t->start.tv_sec  = offset.tv_sec;
        t->start.tv_usec = offset.tv_usec;
    }

    return( delta );
}

static void sighandler( int signum )
{   
    alarmed = 1;
    signal( signum, sighandler );
}

void set_alarm( int seconds )
{
    alarmed = 0;
    signal( SIGALRM, sighandler );
    alarm( seconds );
}

void m_sleep( int milliseconds )
{
    struct timeval tv;

    tv.tv_sec  = milliseconds / 1000;
    tv.tv_usec = milliseconds * 1000;

    select( 0, NULL, NULL, NULL, &tv );
}
#endif

/**
 * \file rsa.h
 *
 *  Copyright (C) 2006-2010, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */



//rsa.h
#ifndef POLARSSL_RSA_H
#define POLARSSL_RSA_H


/*
 * RSA Error codes
 */
#define POLARSSL_ERR_RSA_BAD_INPUT_DATA                    -0x0400
#define POLARSSL_ERR_RSA_INVALID_PADDING                   -0x0410
#define POLARSSL_ERR_RSA_KEY_GEN_FAILED                    -0x0420
#define POLARSSL_ERR_RSA_KEY_CHECK_FAILED                  -0x0430
#define POLARSSL_ERR_RSA_PUBLIC_FAILED                     -0x0440
#define POLARSSL_ERR_RSA_PRIVATE_FAILED                    -0x0450
#define POLARSSL_ERR_RSA_VERIFY_FAILED                     -0x0460
#define POLARSSL_ERR_RSA_OUTPUT_TOO_LARGE                  -0x0470
#define POLARSSL_ERR_RSA_RNG_FAILED                        -0x0480

/*
 * PKCS#1 constants
 */
#define SIG_RSA_RAW     0
#define SIG_RSA_MD2     2
#define SIG_RSA_MD4     3
#define SIG_RSA_MD5     4
#define SIG_RSA_SHA1	5
#define SIG_RSA_SHA224	14
#define SIG_RSA_SHA256	11
#define	SIG_RSA_SHA384	12
#define SIG_RSA_SHA512	13

#define RSA_PUBLIC      0
#define RSA_PRIVATE     1

#define RSA_PKCS_V15    0
#define RSA_PKCS_V21    1

#define RSA_SIGN        1
#define RSA_CRYPT       2

#define ASN1_STR_CONSTRUCTED_SEQUENCE	"\x30"
#define ASN1_STR_NULL			        "\x05"
#define ASN1_STR_OID			        "\x06"
#define ASN1_STR_OCTET_STRING		    "\x04"

#define OID_DIGEST_ALG_MDX	        "\x2A\x86\x48\x86\xF7\x0D\x02\x00"
#define OID_HASH_ALG_SHA1	        "\x2b\x0e\x03\x02\x1a"
#define OID_HASH_ALG_SHA2X	        "\x60\x86\x48\x01\x65\x03\x04\x02\x00"

#define OID_ISO_MEMBER_BODIES	    "\x2a"
#define OID_ISO_IDENTIFIED_ORG	    "\x2b"

/*
 * ISO Member bodies OID parts
 */
#define OID_COUNTRY_US		        "\x86\x48"
#define OID_RSA_DATA_SECURITY	    "\x86\xf7\x0d"

/*
 * ISO Identified organization OID parts
 */
#define OID_OIW_SECSIG_SHA1	        "\x0e\x03\x02\x1a"

/*
 * DigestInfo ::= SEQUENCE {
 *   digestAlgorithm DigestAlgorithmIdentifier,
 *   digest Digest }
 *
 * DigestAlgorithmIdentifier ::= AlgorithmIdentifier
 *
 * Digest ::= OCTET STRING
 */
#define ASN1_HASH_MDX					        \
(							                    \
    ASN1_STR_CONSTRUCTED_SEQUENCE "\x20"		\
      ASN1_STR_CONSTRUCTED_SEQUENCE "\x0C"		\
        ASN1_STR_OID "\x08"				        \
	  OID_DIGEST_ALG_MDX				        \
	ASN1_STR_NULL "\x00"				        \
      ASN1_STR_OCTET_STRING "\x10"			    \
)

#define ASN1_HASH_SHA1					        \
    ASN1_STR_CONSTRUCTED_SEQUENCE "\x21"		\
      ASN1_STR_CONSTRUCTED_SEQUENCE "\x09"		\
        ASN1_STR_OID "\x05"				        \
	  OID_HASH_ALG_SHA1				            \
        ASN1_STR_NULL "\x00"				    \
      ASN1_STR_OCTET_STRING "\x14"

#define ASN1_HASH_SHA2X					        \
    ASN1_STR_CONSTRUCTED_SEQUENCE "\x11"		\
      ASN1_STR_CONSTRUCTED_SEQUENCE "\x0d"		\
        ASN1_STR_OID "\x09"				        \
	  OID_HASH_ALG_SHA2X				        \
        ASN1_STR_NULL "\x00"				    \
      ASN1_STR_OCTET_STRING "\x00"
#if !kernel_module
	/**
	* \brief          RSA context structure
	*/
	typedef struct
	{
	int ver;                    /*!<  always 0          */
	int len;                    /*!<  size(N) in chars  */

	mpi N;                      /*!<  public modulus    */
	mpi E;                      /*!<  public exponent   */

	mpi D;                      /*!<  private exponent  */
	mpi P;                      /*!<  1st prime factor  */
	mpi Q;                      /*!<  2nd prime factor  */
	mpi DP;                     /*!<  D % (P - 1)       */
	mpi DQ;                     /*!<  D % (Q - 1)       */
	mpi QP;                     /*!<  1 / (Q % P)       */

	mpi RN;                     /*!<  cached R^2 mod N  */
	mpi RP;                     /*!<  cached R^2 mod P  */
	mpi RQ;                     /*!<  cached R^2 mod Q  */

	int padding;                /*!<  1.5 or OAEP/PSS   */
	int hash_id;                /*!<  hash identifier   */
	}
	rsa_context;

	/**
	* \brief          Initialize an RSA context
	*
	* \param ctx      RSA context to be initialized
	* \param padding  RSA_PKCS_V15 or RSA_PKCS_V21
	* \param hash_id  RSA_PKCS_V21 hash identifier
	*
	* \note           The hash_id parameter is actually ignored
	*                 when using RSA_PKCS_V15 padding.
	*
	* \note           Currently, RSA_PKCS_V21 padding
	*                 is not supported.
	*/
	void rsa_init( rsa_context *ctx);
	
	/**
	* \brief          Free the components of an RSA key
	*
	* \param ctx      RSA Context to free
	*/
	void rsa_free( rsa_context *ctx );


#endif
/**
 * \brief          Generate an RSA keypair
 *
 * \param ctx      RSA context that will hold the key
 * \param f_rng    RNG function
 * \param p_rng    RNG parameter
 * \param nbits    size of the public key in bits
 * \param exponent public exponent (e.g., 65537)
 *
 * \note           rsa_init() must be called beforehand to setup
 *                 the RSA context.
 *
 * \return         0 if successful, or an POLARSSL_ERR_RSA_XXX error code
 */
int rsa_gen_key( rsa_context *ctx,
                 int (*f_rng)(void *),
                 void *p_rng,
                 int nbits, int exponent );

/**
 * \brief          Check a public RSA key
 *
 * \param ctx      RSA context to be checked
 *
 * \return         0 if successful, or an POLARSSL_ERR_RSA_XXX error code
 */
int rsa_check_pubkey( const rsa_context *ctx );

/**
 * \brief          Check a private RSA key
 *
 * \param ctx      RSA context to be checked
 *
 * \return         0 if successful, or an POLARSSL_ERR_RSA_XXX error code
 */
int rsa_check_privkey( const rsa_context *ctx );

/**
 * \brief          Do an RSA public key operation
 *
 * \param ctx      RSA context
 * \param input    input buffer
 * \param output   output buffer
 *
 * \return         0 if successful, or an POLARSSL_ERR_RSA_XXX error code
 *
 * \note           This function does NOT take care of message
 *                 padding. Also, be sure to set input[0] = 0 or assure that
 *                 input is smaller than N.
 *
 * \note           The input and output buffers must be large
 *                 enough (eg. 128 bytes if RSA-1024 is used).
 */
int rsa_public( rsa_context *ctx,
                const unsigned char *input,
                unsigned char *output );

/**
 * \brief          Do an RSA private key operation
 *
 * \param ctx      RSA context
 * \param input    input buffer
 * \param output   output buffer
 *
 * \return         0 if successful, or an POLARSSL_ERR_RSA_XXX error code
 *
 * \note           The input and output buffers must be large
 *                 enough (eg. 128 bytes if RSA-1024 is used).
 */
int rsa_private( rsa_context *ctx,
                 const unsigned char *input,
                 unsigned char *output );

/**
 * \brief          Add the message padding, then do an RSA operation
 *
 * \param ctx      RSA context
 * \param f_rng    RNG function
 * \param p_rng    RNG parameter
 * \param mode     RSA_PUBLIC or RSA_PRIVATE
 * \param ilen     contains the plaintext length
 * \param input    buffer holding the data to be encrypted
 * \param output   buffer that will hold the ciphertext
 *
 * \return         0 if successful, or an POLARSSL_ERR_RSA_XXX error code
 *
 * \note           The output buffer must be as large as the size
 *                 of ctx->N (eg. 128 bytes if RSA-1024 is used).
 */
int rsa_pkcs1_encrypt( rsa_context *ctx,
                       int (*f_rng)(void *),
                       void *p_rng,
                       int mode, int  ilen,
                       const unsigned char *input,
                       unsigned char *output );

/**
 * \brief          Do an RSA operation, then remove the message padding
 *
 * \param ctx      RSA context
 * \param mode     RSA_PUBLIC or RSA_PRIVATE
 * \param input    buffer holding the encrypted data
 * \param output   buffer that will hold the plaintext
 * \param olen     will contain the plaintext length
 * \param output_max_len	maximum length of the output buffer
 *
 * \return         0 if successful, or an POLARSSL_ERR_RSA_XXX error code
 *
 * \note           The output buffer must be as large as the size
 *                 of ctx->N (eg. 128 bytes if RSA-1024 is used) otherwise
 *                 an error is thrown.
 */
int rsa_pkcs1_decrypt( rsa_context *ctx,
                       int mode, int *olen,
                       const unsigned char *input,
                       unsigned char *output,
		               int output_max_len );

/**
 * \brief          Do a private RSA to sign a message digest
 *
 * \param ctx      RSA context
 * \param mode     RSA_PUBLIC or RSA_PRIVATE
 * \param hash_id  SIG_RSA_RAW, SIG_RSA_MD{2,4,5} or SIG_RSA_SHA{1,224,256,384,512}
 * \param hashlen  message digest length (for SIG_RSA_RAW only)
 * \param hash     buffer holding the message digest
 * \param sig      buffer that will hold the ciphertext
 *
 * \return         0 if the signing operation was successful,
 *                 or an POLARSSL_ERR_RSA_XXX error code
 *
 * \note           The "sig" buffer must be as large as the size
 *                 of ctx->N (eg. 128 bytes if RSA-1024 is used).
 */
int rsa_pkcs1_sign( rsa_context *ctx,
                    int mode,
                    int hash_id,
                    int hashlen,
                    const unsigned char *hash,
                    unsigned char *sig );

/**
 * \brief          Do a public RSA and check the message digest
 *
 * \param ctx      points to an RSA public key
 * \param mode     RSA_PUBLIC or RSA_PRIVATE
 * \param hash_id  SIG_RSA_RAW, SIG_RSA_MD{2,4,5} or SIG_RSA_SHA{1,224,256,384,512}
 * \param hashlen  message digest length (for SIG_RSA_RAW only)
 * \param hash     buffer holding the message digest
 * \param sig      buffer holding the ciphertext
 *
 * \return         0 if the verify operation was successful,
 *                 or an POLARSSL_ERR_RSA_XXX error code
 *
 * \note           The "sig" buffer must be as large as the size
 *                 of ctx->N (eg. 128 bytes if RSA-1024 is used).
 */
int rsa_pkcs1_verify( rsa_context *ctx,
                      int mode,
                      int hash_id,
                      int hashlen,
                      const unsigned char *hash,
                      unsigned char *sig );



#endif /* rsa.h */


//rsa.c

/*
 *  The RSA public-key cryptosystem
 *
 *  Copyright (C) 2006-2010, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 *  RSA was designed by Ron Rivest, Adi Shamir and Len Adleman.
 *
 *  http://theory.lcs.mit.edu/~rivest/rsapaper.pdf
 *  http://www.cacr.math.uwaterloo.ca/hac/about/chap8.pdf
 */

//#include "config.h"



//#include "rsa.h"


//#include "bignum.c"


/*
 * Initialize an RSA context
 */
void rsa_init( rsa_context *ctx)
{
    memset( ctx, 0, sizeof( rsa_context ) );

    ctx->padding = RSA_PKCS_V15;
    ctx->hash_id = 0;
}



/*
 * Generate an RSA keypair
 */
int rsa_gen_key( rsa_context *ctx,
        int (*f_rng)(void *),
        void *p_rng,
        int nbits, int exponent )
{
    int ret;
    mpi P1, Q1, H, G;

    if( f_rng == NULL || nbits < 128 || exponent < 3 )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    mpi_init( &P1, &Q1, &H, &G, NULL );

    /*
     * find primes P and Q with Q < P so that:
     * GCD( E, (P-1)*(Q-1) ) == 1
     */
    MPI_CHK( mpi_lset( &ctx->E, exponent ) );

    do
    {
        MPI_CHK( mpi_gen_prime( &ctx->P, ( nbits + 1 ) >> 1, 0, 
                                f_rng, p_rng ) );

        MPI_CHK( mpi_gen_prime( &ctx->Q, ( nbits + 1 ) >> 1, 0,
                                f_rng, p_rng ) );

        if( mpi_cmp_mpi( &ctx->P, &ctx->Q ) < 0 )
            mpi_swap( &ctx->P, &ctx->Q );

        if( mpi_cmp_mpi( &ctx->P, &ctx->Q ) == 0 )
            continue;

        MPI_CHK( mpi_mul_mpi( &ctx->N, &ctx->P, &ctx->Q ) );
        if( mpi_msb( &ctx->N ) != nbits )
            continue;

        MPI_CHK( mpi_sub_int( &P1, &ctx->P, 1 ) );
        MPI_CHK( mpi_sub_int( &Q1, &ctx->Q, 1 ) );
        MPI_CHK( mpi_mul_mpi( &H, &P1, &Q1 ) );
        MPI_CHK( mpi_gcd( &G, &ctx->E, &H  ) );
    }
    while( mpi_cmp_int( &G, 1 ) != 0 );

    /*
     * D  = E^-1 mod ((P-1)*(Q-1))
     * DP = D mod (P - 1)
     * DQ = D mod (Q - 1)
     * QP = Q^-1 mod P
     */
    MPI_CHK( mpi_inv_mod( &ctx->D , &ctx->E, &H  ) );
    MPI_CHK( mpi_mod_mpi( &ctx->DP, &ctx->D, &P1 ) );
    MPI_CHK( mpi_mod_mpi( &ctx->DQ, &ctx->D, &Q1 ) );
    MPI_CHK( mpi_inv_mod( &ctx->QP, &ctx->Q, &ctx->P ) );

    ctx->len = ( mpi_msb( &ctx->N ) + 7 ) >> 3;

cleanup:

    mpi_free( &G, &H, &Q1, &P1, NULL );

    if( ret != 0 )
    {
        rsa_free( ctx );
        return( POLARSSL_ERR_RSA_KEY_GEN_FAILED | ret );
    }

    return( 0 );   
}


/*
 * Check a public RSA key
 */
int rsa_check_pubkey( const rsa_context *ctx )
{
    if( !ctx->N.p || !ctx->E.p )
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );

    if( ( ctx->N.p[0] & 1 ) == 0 || 
        ( ctx->E.p[0] & 1 ) == 0 )
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );

    if( mpi_msb( &ctx->N ) < 128 ||
        mpi_msb( &ctx->N ) > 4096 )
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );

    if( mpi_msb( &ctx->E ) < 2 ||
        mpi_msb( &ctx->E ) > 64 )
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );

    return( 0 );
}

/*
 * Check a private RSA key
 */
int rsa_check_privkey( const rsa_context *ctx )
{
    int ret;
    mpi PQ, DE, P1, Q1, H, I, G, G2, L1, L2;

    if( ( ret = rsa_check_pubkey( ctx ) ) != 0 )
        return( ret );

    if( !ctx->P.p || !ctx->Q.p || !ctx->D.p )
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );

    mpi_init( &PQ, &DE, &P1, &Q1, &H, &I, &G, &G2, &L1, &L2, NULL );

    MPI_CHK( mpi_mul_mpi( &PQ, &ctx->P, &ctx->Q ) );
    MPI_CHK( mpi_mul_mpi( &DE, &ctx->D, &ctx->E ) );
    MPI_CHK( mpi_sub_int( &P1, &ctx->P, 1 ) );
    MPI_CHK( mpi_sub_int( &Q1, &ctx->Q, 1 ) );
    MPI_CHK( mpi_mul_mpi( &H, &P1, &Q1 ) );
    MPI_CHK( mpi_gcd( &G, &ctx->E, &H  ) );

    MPI_CHK( mpi_gcd( &G2, &P1, &Q1 ) );
    MPI_CHK( mpi_div_mpi( &L1, &L2, &H, &G2 ) );  
    MPI_CHK( mpi_mod_mpi( &I, &DE, &L1  ) );

    /*
     * Check for a valid PKCS1v2 private key
     */
    if( mpi_cmp_mpi( &PQ, &ctx->N ) == 0 &&
        mpi_cmp_int( &L2, 0 ) == 0 &&
        mpi_cmp_int( &I, 1 ) == 0 &&
        mpi_cmp_int( &G, 1 ) == 0 )
    {
        mpi_free( &G, &I, &H, &Q1, &P1, &DE, &PQ, &G2, &L1, &L2, NULL );
        return( 0 );
    }

    
cleanup:

    mpi_free( &G, &I, &H, &Q1, &P1, &DE, &PQ, &G2, &L1, &L2, NULL );
    return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED | ret );
}

/*
 * Do an RSA public key operation
 */
int rsa_public( rsa_context *ctx,
                const unsigned char *input,
                unsigned char *output )
{
    int ret, olen;
    mpi T;

    mpi_init( &T, NULL );

    MPI_CHK( mpi_read_binary( &T, input, ctx->len ) );

    if( mpi_cmp_mpi( &T, &ctx->N ) >= 0 )
    {
        mpi_free( &T, NULL );
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );
    }

    olen = ctx->len;
    MPI_CHK( mpi_exp_mod( &T, &T, &ctx->E, &ctx->N, &ctx->RN ) );
    MPI_CHK( mpi_write_binary( &T, output, olen ) );

cleanup:

    mpi_free( &T, NULL );

    if( ret != 0 )
        return( POLARSSL_ERR_RSA_PUBLIC_FAILED | ret );

    return( 0 );
}

/*
 * Do an RSA private key operation
 */
int rsa_private( rsa_context *ctx,
                 const unsigned char *input,
                 unsigned char *output )
{
    int ret, olen;
    mpi T, T1, T2;

    mpi_init( &T, &T1, &T2, NULL );

    MPI_CHK( mpi_read_binary( &T, input, ctx->len ) );

    if( mpi_cmp_mpi( &T, &ctx->N ) >= 0 )
    {
        mpi_free( &T, NULL );
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );
    }


    /*
     * faster decryption using the CRT
     *
     * T1 = input ^ dP mod P
     * T2 = input ^ dQ mod Q
     */
    MPI_CHK( mpi_exp_mod( &T1, &T, &ctx->DP, &ctx->P, &ctx->RP ) );
    MPI_CHK( mpi_exp_mod( &T2, &T, &ctx->DQ, &ctx->Q, &ctx->RQ ) );

    /*
     * T = (T1 - T2) * (Q^-1 mod P) mod P
     */
    MPI_CHK( mpi_sub_mpi( &T, &T1, &T2 ) );
    MPI_CHK( mpi_mul_mpi( &T1, &T, &ctx->QP ) );
    MPI_CHK( mpi_mod_mpi( &T, &T1, &ctx->P ) );

    /*
     * output = T2 + T * Q
     */
    MPI_CHK( mpi_mul_mpi( &T1, &T, &ctx->Q ) );
    MPI_CHK( mpi_add_mpi( &T, &T2, &T1 ) );

    olen = ctx->len;
    MPI_CHK( mpi_write_binary( &T, output, olen ) );

cleanup:

    mpi_free( &T, &T1, &T2, NULL );

    if( ret != 0 )
        return( POLARSSL_ERR_RSA_PRIVATE_FAILED | ret );

    return( 0 );
}

/*
 * Add the message padding, then do an RSA operation
 */
int rsa_pkcs1_encrypt( rsa_context *ctx,
                       int (*f_rng)(void *),
                       void *p_rng,
                       int mode, int  ilen,
                       const unsigned char *input,
                       unsigned char *output )
{
    int nb_pad, olen;
    unsigned char *p = output;

    olen = ctx->len;

    switch( ctx->padding )
    {
        case RSA_PKCS_V15:

            if( ilen < 0 || olen < ilen + 11 || f_rng == NULL )
                return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

            nb_pad = olen - 3 - ilen;

            *p++ = 0;
            *p++ = RSA_CRYPT;

            while( nb_pad-- > 0 )
            {
                int rng_dl = 100;

                do {
                    *p = (unsigned char) f_rng( p_rng );
                } while( *p == 0 && --rng_dl );

                // Check if RNG failed to generate data
                //
                if( rng_dl == 0 )
                    return POLARSSL_ERR_RSA_RNG_FAILED;

                p++;
            }
            *p++ = 0;
            memcpy( p, input, ilen );
            break;

        default:

            return( POLARSSL_ERR_RSA_INVALID_PADDING );
    }

    return( ( mode == RSA_PUBLIC )
            ? rsa_public(  ctx, output, output )
            : rsa_private( ctx, output, output ) );
}

/*
 * Do an RSA operation, then remove the message padding
 */
int rsa_pkcs1_decrypt( rsa_context *ctx,
                       int mode, int *olen,
                       const unsigned char *input,
                       unsigned char *output,
                       int output_max_len)
{
    int ret, ilen;
    unsigned char *p;
    unsigned char buf[1024];

    ilen = ctx->len;

    if( ilen < 16 || ilen > (int) sizeof( buf ) )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    ret = ( mode == RSA_PUBLIC )
          ? rsa_public(  ctx, input, buf )
          : rsa_private( ctx, input, buf );

    if( ret != 0 )
        return( ret );

    p = buf;

    switch( ctx->padding )
    {
        case RSA_PKCS_V15:

            if( *p++ != 0 || *p++ != RSA_CRYPT )
                return( POLARSSL_ERR_RSA_INVALID_PADDING );

            while( *p != 0 )
            {
                if( p >= buf + ilen - 1 )
                    return( POLARSSL_ERR_RSA_INVALID_PADDING );
                p++;
            }
            p++;
            break;

        default:

            return( POLARSSL_ERR_RSA_INVALID_PADDING );
    }

    if (ilen - (int)(p - buf) > output_max_len)
    	return( POLARSSL_ERR_RSA_OUTPUT_TOO_LARGE );

    *olen = ilen - (int)(p - buf);
    memcpy( output, p, *olen );

    return( 0 );
}

/*
 * Do an RSA operation to sign the message digest
 */
int rsa_pkcs1_sign( rsa_context *ctx,
                    int mode,
                    int hash_id,
                    int hashlen,
                    const unsigned char *hash,
                    unsigned char *sig )
{
    int nb_pad, olen;
    unsigned char *p = sig;

    olen = ctx->len;

    switch( ctx->padding )
    {
        case RSA_PKCS_V15:

            switch( hash_id )
            {
                case SIG_RSA_RAW:
                    nb_pad = olen - 3 - hashlen;
                    break;

                case SIG_RSA_MD2:
                case SIG_RSA_MD4:
                case SIG_RSA_MD5:
                    nb_pad = olen - 3 - 34;
                    break;

                case SIG_RSA_SHA1:
                    nb_pad = olen - 3 - 35;
                    break;

                case SIG_RSA_SHA224:
                    nb_pad = olen - 3 - 47;
                    break;

                case SIG_RSA_SHA256:
                    nb_pad = olen - 3 - 51;
                    break;

                case SIG_RSA_SHA384:
                    nb_pad = olen - 3 - 67;
                    break;

                case SIG_RSA_SHA512:
                    nb_pad = olen - 3 - 83;
                    break;


                default:
                    return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );
            }

            if( nb_pad < 8 )
                return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

            *p++ = 0;
            *p++ = RSA_SIGN;
            memset( p, 0xFF, nb_pad );
            p += nb_pad;
            *p++ = 0;
            break;

        default:

            return( POLARSSL_ERR_RSA_INVALID_PADDING );
    }

    switch( hash_id )
    {
        case SIG_RSA_RAW:
            memcpy( p, hash, hashlen );
            break;

        case SIG_RSA_MD2:
            memcpy( p, ASN1_HASH_MDX, 18 );
            memcpy( p + 18, hash, 16 );
            p[13] = 2; break;

        case SIG_RSA_MD4:
            memcpy( p, ASN1_HASH_MDX, 18 );
            memcpy( p + 18, hash, 16 );
            p[13] = 4; break;

        case SIG_RSA_MD5:
            memcpy( p, ASN1_HASH_MDX, 18 );
            memcpy( p + 18, hash, 16 );
            p[13] = 5; break;

        case SIG_RSA_SHA1:
            memcpy( p, ASN1_HASH_SHA1, 15 );
            memcpy( p + 15, hash, 20 );
            break;

        case SIG_RSA_SHA224:
            memcpy( p, ASN1_HASH_SHA2X, 19 );
            memcpy( p + 19, hash, 28 );
            p[1] += 28; p[14] = 4; p[18] += 28; break;

        case SIG_RSA_SHA256:
            memcpy( p, ASN1_HASH_SHA2X, 19 );
            memcpy( p + 19, hash, 32 );
            p[1] += 32; p[14] = 1; p[18] += 32; break;

        case SIG_RSA_SHA384:
            memcpy( p, ASN1_HASH_SHA2X, 19 );
            memcpy( p + 19, hash, 48 );
            p[1] += 48; p[14] = 2; p[18] += 48; break;

        case SIG_RSA_SHA512:
            memcpy( p, ASN1_HASH_SHA2X, 19 );
            memcpy( p + 19, hash, 64 );
            p[1] += 64; p[14] = 3; p[18] += 64; break;

        default:
            return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );
    }

    return( ( mode == RSA_PUBLIC )
            ? rsa_public(  ctx, sig, sig )
            : rsa_private( ctx, sig, sig ) );
}

/*
 * Do an RSA operation and check the message digest
 */
int rsa_pkcs1_verify( rsa_context *ctx,
                      int mode,
                      int hash_id,
                      int hashlen,
                      const unsigned char *hash,
                      unsigned char *sig )
{
    int ret, len, siglen;
    unsigned char *p, c;
    unsigned char buf[1024];

    siglen = ctx->len;

    if( siglen < 16 || siglen > (int) sizeof( buf ) )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    ret = ( mode == RSA_PUBLIC )
          ? rsa_public(  ctx, sig, buf )
          : rsa_private( ctx, sig, buf );

    if( ret != 0 )
        return( ret );

    p = buf;

    switch( ctx->padding )
    {
        case RSA_PKCS_V15:

            if( *p++ != 0 || *p++ != RSA_SIGN )
                return( POLARSSL_ERR_RSA_INVALID_PADDING );

            while( *p != 0 )
            {
                if( p >= buf + siglen - 1 || *p != 0xFF )
                    return( POLARSSL_ERR_RSA_INVALID_PADDING );
                p++;
            }
            p++;
            break;

        default:

            return( POLARSSL_ERR_RSA_INVALID_PADDING );
    }

    len = siglen - (int)( p - buf );

    if( len == 34 )
    {
        c = p[13];
        p[13] = 0;

        if( memcmp( p, ASN1_HASH_MDX, 18 ) != 0 )
            return( POLARSSL_ERR_RSA_VERIFY_FAILED );

        if( ( c == 2 && hash_id == SIG_RSA_MD2 ) ||
            ( c == 4 && hash_id == SIG_RSA_MD4 ) ||
            ( c == 5 && hash_id == SIG_RSA_MD5 ) )
        {
            if( memcmp( p + 18, hash, 16 ) == 0 ) 
                return( 0 );
            else
                return( POLARSSL_ERR_RSA_VERIFY_FAILED );
        }
    }

    if( len == 35 && hash_id == SIG_RSA_SHA1 )
    {
        if( memcmp( p, ASN1_HASH_SHA1, 15 ) == 0 &&
            memcmp( p + 15, hash, 20 ) == 0 )
            return( 0 );
        else
            return( POLARSSL_ERR_RSA_VERIFY_FAILED );
    }
    if( ( len == 19 + 28 && p[14] == 4 && hash_id == SIG_RSA_SHA224 ) ||
        ( len == 19 + 32 && p[14] == 1 && hash_id == SIG_RSA_SHA256 ) ||
        ( len == 19 + 48 && p[14] == 2 && hash_id == SIG_RSA_SHA384 ) ||
        ( len == 19 + 64 && p[14] == 3 && hash_id == SIG_RSA_SHA512 ) )
    {
    	c = p[1] - 17;
        p[1] = 17;
        p[14] = 0;

        if( p[18] == c &&
                memcmp( p, ASN1_HASH_SHA2X, 18 ) == 0 &&
                memcmp( p + 19, hash, c ) == 0 )
            return( 0 );
        else
            return( POLARSSL_ERR_RSA_VERIFY_FAILED );
    }

    if( len == hashlen && hash_id == SIG_RSA_RAW )
    {
        if( memcmp( p, hash, hashlen ) == 0 )
            return( 0 );
        else
            return( POLARSSL_ERR_RSA_VERIFY_FAILED );
    }

    return( POLARSSL_ERR_RSA_INVALID_PADDING );
}

/*
 * Free the components of an RSA key
 */
void rsa_free( rsa_context *ctx )
{
    mpi_free( &ctx->RQ, &ctx->RP, &ctx->RN,
              &ctx->QP, &ctx->DQ, &ctx->DP,
              &ctx->Q,  &ctx->P,  &ctx->D,
              &ctx->E,  &ctx->N,  NULL );
}

//havege.h
/**
 * \file havege.h
 *
 *  Copyright (C) 2006-2010, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef POLARSSL_HAVEGE_H
#define POLARSSL_HAVEGE_H

#define COLLECT_SIZE 1024

/**
 * \brief          HAVEGE state structure
 */
typedef struct
{
    int PT1, PT2, offset[2];
    int pool[COLLECT_SIZE];
    int WALK[8192];
}
havege_state;

/**
 * \brief          HAVEGE initialization
 *
 * \param hs       HAVEGE state to be initialized
 */
void havege_init( havege_state *hs );

/**
 * \brief          HAVEGE rand function
 *
 * \param p_rng    A HAVEGE state
 *
 * \return         A random int
 */
int havege_rand( void *p_rng );

#endif /* havege.h */


//havege.c
/*
 *  HAVEGE: HArdware Volatile Entropy Gathering and Expansion
 *
 *  Copyright (C) 2006-2010, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 *  The HAVEGE RNG was designed by Andre Seznec in 2002.
 *
 *  http://www.irisa.fr/caps/projects/hipsor/publi.php
 *
 *  Contact: seznec(at)irisa_dot_fr - orocheco(at)irisa_dot_fr
 */

//#include <string.h>
//#include <time.h>

//#include "config.h"
//#include "havege.h"
//#include "timing.h"

/* ------------------------------------------------------------------------
 * On average, one iteration accesses two 8-word blocks in the havege WALK
 * table, and generates 16 words in the RES array.
 *
 * The data read in the WALK table is updated and permuted after each use.
 * The result of the hardware clock counter read is used  for this update.
 *
 * 25 conditional tests are present.  The conditional tests are grouped in
 * two nested  groups of 12 conditional tests and 1 test that controls the
 * permutation; on average, there should be 6 tests executed and 3 of them
 * should be mispredicted.
 * ------------------------------------------------------------------------
 */

#define SWAP(X,Y) { int *T = X; X = Y; Y = T; }

#define TST1_ENTER if( PTEST & 1 ) { PTEST ^= 3; PTEST >>= 1;
#define TST2_ENTER if( PTEST & 1 ) { PTEST ^= 3; PTEST >>= 1;

#define TST1_LEAVE U1++; }
#define TST2_LEAVE U2++; }

#define ONE_ITERATION                                   \
                                                        \
    PTEST = PT1 >> 20;                                  \
                                                        \
    TST1_ENTER  TST1_ENTER  TST1_ENTER  TST1_ENTER      \
    TST1_ENTER  TST1_ENTER  TST1_ENTER  TST1_ENTER      \
    TST1_ENTER  TST1_ENTER  TST1_ENTER  TST1_ENTER      \
                                                        \
    TST1_LEAVE  TST1_LEAVE  TST1_LEAVE  TST1_LEAVE      \
    TST1_LEAVE  TST1_LEAVE  TST1_LEAVE  TST1_LEAVE      \
    TST1_LEAVE  TST1_LEAVE  TST1_LEAVE  TST1_LEAVE      \
                                                        \
    PTX = (PT1 >> 18) & 7;                              \
    PT1 &= 0x1FFF;                                      \
    PT2 &= 0x1FFF;                                      \
    CLK = (int) hardclock();                            \
                                                        \
    i = 0;                                              \
    A = &WALK[PT1    ]; RES[i++] ^= *A;                 \
    B = &WALK[PT2    ]; RES[i++] ^= *B;                 \
    C = &WALK[PT1 ^ 1]; RES[i++] ^= *C;                 \
    D = &WALK[PT2 ^ 4]; RES[i++] ^= *D;                 \
                                                        \
    IN = (*A >> (1)) ^ (*A << (31)) ^ CLK;              \
    *A = (*B >> (2)) ^ (*B << (30)) ^ CLK;              \
    *B = IN ^ U1;                                       \
    *C = (*C >> (3)) ^ (*C << (29)) ^ CLK;              \
    *D = (*D >> (4)) ^ (*D << (28)) ^ CLK;              \
                                                        \
    A = &WALK[PT1 ^ 2]; RES[i++] ^= *A;                 \
    B = &WALK[PT2 ^ 2]; RES[i++] ^= *B;                 \
    C = &WALK[PT1 ^ 3]; RES[i++] ^= *C;                 \
    D = &WALK[PT2 ^ 6]; RES[i++] ^= *D;                 \
                                                        \
    if( PTEST & 1 ) SWAP( A, C );                       \
                                                        \
    IN = (*A >> (5)) ^ (*A << (27)) ^ CLK;              \
    *A = (*B >> (6)) ^ (*B << (26)) ^ CLK;              \
    *B = IN; CLK = (int) hardclock();                   \
    *C = (*C >> (7)) ^ (*C << (25)) ^ CLK;              \
    *D = (*D >> (8)) ^ (*D << (24)) ^ CLK;              \
                                                        \
    A = &WALK[PT1 ^ 4];                                 \
    B = &WALK[PT2 ^ 1];                                 \
                                                        \
    PTEST = PT2 >> 1;                                   \
                                                        \
    PT2 = (RES[(i - 8) ^ PTY] ^ WALK[PT2 ^ PTY ^ 7]);   \
    PT2 = ((PT2 & 0x1FFF) & (~8)) ^ ((PT1 ^ 8) & 0x8);  \
    PTY = (PT2 >> 10) & 7;                              \
                                                        \
    TST2_ENTER  TST2_ENTER  TST2_ENTER  TST2_ENTER      \
    TST2_ENTER  TST2_ENTER  TST2_ENTER  TST2_ENTER      \
    TST2_ENTER  TST2_ENTER  TST2_ENTER  TST2_ENTER      \
                                                        \
    TST2_LEAVE  TST2_LEAVE  TST2_LEAVE  TST2_LEAVE      \
    TST2_LEAVE  TST2_LEAVE  TST2_LEAVE  TST2_LEAVE      \
    TST2_LEAVE  TST2_LEAVE  TST2_LEAVE  TST2_LEAVE      \
                                                        \
    C = &WALK[PT1 ^ 5];                                 \
    D = &WALK[PT2 ^ 5];                                 \
                                                        \
    RES[i++] ^= *A;                                     \
    RES[i++] ^= *B;                                     \
    RES[i++] ^= *C;                                     \
    RES[i++] ^= *D;                                     \
                                                        \
    IN = (*A >> ( 9)) ^ (*A << (23)) ^ CLK;             \
    *A = (*B >> (10)) ^ (*B << (22)) ^ CLK;             \
    *B = IN ^ U2;                                       \
    *C = (*C >> (11)) ^ (*C << (21)) ^ CLK;             \
    *D = (*D >> (12)) ^ (*D << (20)) ^ CLK;             \
                                                        \
    A = &WALK[PT1 ^ 6]; RES[i++] ^= *A;                 \
    B = &WALK[PT2 ^ 3]; RES[i++] ^= *B;                 \
    C = &WALK[PT1 ^ 7]; RES[i++] ^= *C;                 \
    D = &WALK[PT2 ^ 7]; RES[i++] ^= *D;                 \
                                                        \
    IN = (*A >> (13)) ^ (*A << (19)) ^ CLK;             \
    *A = (*B >> (14)) ^ (*B << (18)) ^ CLK;             \
    *B = IN;                                            \
    *C = (*C >> (15)) ^ (*C << (17)) ^ CLK;             \
    *D = (*D >> (16)) ^ (*D << (16)) ^ CLK;             \
                                                        \
    PT1 = ( RES[(i - 8) ^ PTX] ^                        \
            WALK[PT1 ^ PTX ^ 7] ) & (~1);               \
    PT1 ^= (PT2 ^ 0x10) & 0x10;                         \
                                                        \
    for( n++, i = 0; i < 16; i++ )                      \
        hs->pool[n % COLLECT_SIZE] ^= RES[i];

/*
 * Entropy gathering function
 */
static void havege_fill( havege_state *hs )
{
    int i, n = 0;
    int  U1,  U2, *A, *B, *C, *D;
    int PT1, PT2, *WALK, RES[16];
    int PTX, PTY, CLK, PTEST, IN;

    WALK = hs->WALK;
    PT1  = hs->PT1;
    PT2  = hs->PT2;

    PTX  = U1 = 0;
    PTY  = U2 = 0;

    memset( RES, 0, sizeof( RES ) );

    while( n < COLLECT_SIZE * 4 )
    {
        ONE_ITERATION
        ONE_ITERATION
        ONE_ITERATION
        ONE_ITERATION
    }

    hs->PT1 = PT1;
    hs->PT2 = PT2;

    hs->offset[0] = 0;
    hs->offset[1] = COLLECT_SIZE / 2;
}

/*
 * HAVEGE initialization
 */
void havege_init( havege_state *hs )
{
    memset( hs, 0, sizeof( havege_state ) );

    havege_fill( hs );
}

/*
 * HAVEGE rand function
 */
int havege_rand( void *p_rng )
{
    int ret;
    havege_state *hs = (havege_state *) p_rng;

    if( hs->offset[1] >= COLLECT_SIZE )
        havege_fill( hs );

    ret  = hs->pool[hs->offset[0]++];
    ret ^= hs->pool[hs->offset[1]++];

    return( ret );
}

#if !kernel_module
	int generate_rsa_1024(int KEY_SIZE,int EXPONENT, char* private_name, char* public_name){
		int ret;
		rsa_context rsa;
		havege_state hs;
		FILE *fpub  = NULL;
		FILE *fpriv = NULL;

		printf( "\n  . Seeding the random number generator..." );
		fflush( stdout );

		havege_init( &hs );
		printf( " ok\n  . Generating the RSA key [ %d-bit ]...", KEY_SIZE );
		fflush( stdout );

		rsa_init( &rsa );

		if( ( ret = rsa_gen_key( &rsa, havege_rand, &hs, KEY_SIZE, EXPONENT ) ) != 0 ){
			printf( " failed\n  ! rsa_gen_key returned %d\n\n", ret );
			goto exit;
		}
		printf( " ok\n  . Exporting the public  key in %s....",public_name );
		fflush( stdout );

		if( ( fpub = fopen( public_name, "wb+" ) ) == NULL ){
			printf( " failed\n  ! could not open %s for writing\n\n",public_name);
			ret = 1;
			goto exit;
		}
		if( 	( ret = mpi_write_file( "N = ", &rsa.N, 16, fpub ) ) != 0 ||
			( ret = mpi_write_file( "E = ", &rsa.E, 16, fpub ) ) != 0 ){
			printf( " failed\n  ! mpi_write_file returned %d\n\n", ret );
			goto exit;
		}
		printf( " ok\n  . Exporting the private key in %s ...",private_name );
		fflush( stdout );

		if( ( fpriv = fopen( private_name, "wb+" ) ) == NULL )
		{
			printf( " failed\n  ! could not open %s for writing\n",private_name );
			ret = 1;
			goto exit;
		}

		if(	( ret = mpi_write_file( "N = " , &rsa.N , 16, fpriv ) ) != 0 ||
			( ret = mpi_write_file( "E = " , &rsa.E , 16, fpriv ) ) != 0 ||
			( ret = mpi_write_file( "D = " , &rsa.D , 16, fpriv ) ) != 0 ||
			( ret = mpi_write_file( "P = " , &rsa.P , 16, fpriv ) ) != 0 ||
			( ret = mpi_write_file( "Q = " , &rsa.Q , 16, fpriv ) ) != 0 ||
			( ret = mpi_write_file( "DP = ", &rsa.DP, 16, fpriv ) ) != 0 ||
			( ret = mpi_write_file( "DQ = ", &rsa.DQ, 16, fpriv ) ) != 0 ||
			( ret = mpi_write_file( "QP = ", &rsa.QP, 16, fpriv ) ) != 0 ){
			printf( " failed\n  ! mpi_write_file returned %d\n\n", ret );
			goto exit;
		}
		printf( " ok\n\n" );

	exit:
		if( fpub  != NULL )
			fclose( fpub );
		if( fpriv != NULL )
			fclose( fpriv );
		rsa_free( &rsa );
		return( ret );
	}
	
	rsa_context* read_rsa_public_key_from_file(char* file_name){
		FILE* f;
		rsa_context *rsa=malloc(sizeof(rsa_context));
		int ret=1;
		
		printf( "\n  . Reading public key from %s... ",file_name );
		fflush( stdout );
		if( ( f = fopen( file_name, "rb" ) ) == NULL ){
			printf( " failed\n  ! Could not open %s\n",file_name );
			return rsa;
		}
		
		rsa_init( rsa);
		if( ( ret = mpi_read_file( &rsa->N, 16, f ) ) != 0 ||
			( ret = mpi_read_file( &rsa->E, 16, f ) ) != 0 ){
			printf( " failed\n  ! mpi_read_file returned %d\n\n", ret );
			return rsa;
		}
		printf( " ok \n ");
		fflush(stdout);
		rsa->len = ( mpi_msb( &rsa->N ) + 7 ) >> 3;
		fclose( f );
		return rsa;
	}
	
	rsa_context* read_rsa_private_key_from_file(char* file_name){
		rsa_context* rsa=malloc(sizeof(rsa_context));
		FILE *f;
		int ret;
		printf( "\n  . Reading private key from %s... ",file_name );
		fflush( stdout );
		
		if( ( f = fopen( file_name, "rb" ) ) == NULL ){
			printf( " failed\n  ! Could not open %s\n\n",file_name );
			return rsa;
		}
		rsa_init(rsa);
		//printf("***%d\n",f);
		if( ( ret = mpi_read_file( &rsa->N , 16, f ) ) != 0 ||
			( ret = mpi_read_file( &rsa->E , 16, f ) ) != 0 ||
			( ret = mpi_read_file( &rsa->D , 16, f ) ) != 0 ||
			( ret = mpi_read_file( &rsa->P , 16, f ) ) != 0 ||
			( ret = mpi_read_file( &rsa->Q , 16, f ) ) != 0 ||
			( ret = mpi_read_file( &rsa->DP, 16, f ) ) != 0 ||
			( ret = mpi_read_file( &rsa->DQ, 16, f ) ) != 0 ||
			( ret = mpi_read_file( &rsa->QP, 16, f ) ) != 0 ){
				printf( " failed\n  ! mpi_read_file returned %d\n\n", ret );
				return rsa;
		}
		printf( " ok\n ");
		fflush(stdout);
		rsa->len = ( mpi_msb( &rsa->N ) + 7 ) >> 3;
		fclose( f );
		return rsa;
	}
#endif

havege_state hs;
void rsa_1024_init(void){
	havege_init( &hs );
}

int rsa_1024_encrypt(rsa_context* rsa,int mode,int ilen,
			    const unsigned char* input,unsigned char* output){
	return rsa_pkcs1_encrypt(rsa,havege_rand,&hs,mode,ilen,input,output);
}

int rsa_1024_decrypt(rsa_context* rsa,int mode, int* olen,
			   const unsigned char* input,unsigned char *output,
			   int output_max_len){
	return rsa_pkcs1_decrypt(rsa,mode,olen,input,output,output_max_len);
}

#if kernel_module

int mpi_copy_fu( mpi *X, const mpi *Y )
{
    int ret=0, i;

    if( X == Y )
        return( 0 );

    for( i = Y->n - 1; i > 0; i-- )
        if( Y->p[i] != 0 )
            break;
    i++;

    X->s = Y->s;

    MPI_CHK( mpi_grow( X, i ) );

    memset( X->p, 0, X->n * ciL );
   
    
    ret=copy_from_user( X->p, Y->p, i * ciL );
    //printk("<0>" "copy_fu:ret:%d\n",ret);

cleanup:

    return( ret );
}

int mpi_copy_fu2( mpi *X, const mpi *Y )
{
    int ret=0;

    X->s = Y->s;
    X->n =  Y->n;

    X->p=kzalloc(sizeof(t_int)*Y->n,GFP_KERNEL);
    
    ret=copy_from_user( X->p, Y->p, sizeof(t_int)*Y->n );
    
    //printk("<0>" "copy_fu2:ret:%d\n",ret);

    return( ret );
}


#endif