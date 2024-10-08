/***************************************************************************/
/*                                                                         */
/*  ftcalc.c                                                               */
/*                                                                         */
/*    Arithmetic computations (body).                                      */
/*                                                                         */
/*  Copyright 1996-2000 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* Support for 1-complement arithmetic has been totally dropped in this  */
  /* release.  You can still write your own code if you need it.           */
  /*                                                                       */
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* Implementing basic computation routines.                              */
  /*                                                                       */
  /* FT_MulDiv(), FT_MulFix(), and FT_DivFix() are declared in freetype.h. */
  /*                                                                       */
  /*************************************************************************/


#include <freetype/internal/ftcalc.h>
#include <freetype/internal/ftdebug.h>
#include <freetype/internal/ftobjs.h>  /* for ABS() */


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_calc


#ifdef FT_CONFIG_OPTION_OLD_CALCS

  static const FT_Long  ft_square_roots[63] =
  {
       1L,    1L,    2L,     3L,     4L,     5L,     8L,    11L,
      16L,   22L,   32L,    45L,    64L,    90L,   128L,   181L,
     256L,  362L,  512L,   724L,  1024L,  1448L,  2048L,  2896L,
    4096L, 5892L, 8192L, 11585L, 16384L, 23170L, 32768L, 46340L,

      65536L,   92681L,  131072L,   185363L,   262144L,   370727L,
     524288L,  741455L, 1048576L,  1482910L,  2097152L,  2965820L,
    4194304L, 5931641L, 8388608L, 11863283L, 16777216L, 23726566L,

      33554432L,   47453132L,   67108864L,   94906265L,
     134217728L,  189812531L,  268435456L,  379625062L,
     536870912L,  759250125L, 1073741824L, 1518500250L,
    2147483647L
  };

#else

  /* documentation is in ftcalc.h */

  FT_EXPORT_DEF( FT_Int32 )  FT_Sqrt32( FT_Int32  x )
  {
    FT_ULong  val, root, newroot, mask;


    root = 0;
    mask = 0x40000000L;
    val  = (FT_ULong)x;

    do
    {
      newroot = root + mask;
      if ( newroot <= val )
      {
        val -= newroot;
        root = newroot + mask;
      }

      root >>= 1;
      mask >>= 2;

    } while ( mask != 0 );

    return root;
  }

#endif /* FT_CONFIG_OPTION_OLD_CALCS */


#ifdef FT_LONG64

  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Long )  FT_MulDiv( FT_Long  a,
                                       FT_Long  b,
                                       FT_Long  c )
  {
    FT_Int s;


    s = 1;
    if ( a < 0 ) { a = -a; s = -s; }
    if ( b < 0 ) { b = -b; s = -s; }
    if ( c < 0 ) { c = -c; s = -s; }

    return s * ( c > 0 ? ( (FT_Int64)a * b + ( c >> 1 ) ) / c
                       : 0x7FFFFFFFL );
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Long )  FT_MulFix( FT_Long  a,
                                       FT_Long  b )
  {
    FT_Int  s;


    s = 1;
    if ( a < 0 ) { a = -a; s = -s; }
    if ( b < 0 ) { b = -b; s = -s; }

    return s * (FT_Long)( ( (FT_Int64)a * b + 0x8000 ) >> 16 );
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Long )  FT_DivFix( FT_Long  a,
                                       FT_Long  b )
  {
    FT_Int32   s;
    FT_UInt32  q;


    s  = a; a = ABS(a);
    s ^= b; b = ABS(b);

    if ( b == 0 )
      /* check for division by 0 */
      q = 0x7FFFFFFFL;
    else
      /* compute result directly */
      q = ( (FT_Int64)a << 16 ) / b;

    return (FT_Int32)( s < 0 ? -q : q );
  }


#ifdef FT_CONFIG_OPTION_OLD_CALCS

  /* a helper function for FT_Sqrt64() */

  static
  int  ft_order64( FT_Int64  z )
  {
    int  j = 0;


    while ( z )
    {
      z = (unsigned FT_INT64)z >> 1;
      j++;
    }
    return j - 1;
  }


  /* documentation is in ftcalc.h */

  FT_EXPORT_DEF( FT_Int32 )  FT_Sqrt64( FT_Int64  l )
  {
    FT_Int64  r, s;


    if ( l <= 0 ) return 0;
    if ( l == 1 ) return 1;

    r = ft_square_roots[ft_order64( l )];

    do
    {
      s = r;
      r = ( r + l / r ) >> 1;

    } while ( r > s || r * r > l );

    return r;
  }


  FT_EXPORT( FT_Int32 )   FT_SqrtFixed( FT_Int32  x )
  {

    FT_Int64  z;


    z = (FT_Int64)(x) << 16;
    return FT_Sqrt64( z );
  }

#endif /* FT_CONFIG_OPTION_OLD_CALCS */


#else /* FT_LONG64 */


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Long )  FT_MulDiv( FT_Long  a,
                                       FT_Long  b,
                                       FT_Long  c )
  {
    long   s;


    if ( a == 0 || b == c )
      return a;

    s  = a; a = ABS( a );
    s ^= b; b = ABS( b );
    s ^= c; c = ABS( c );

    if ( a <= 46340 && b <= 46340 && c <= 176095L && c > 0 )
    {
      a = ( a * b + ( c >> 1 ) ) / c;
    }
    else if ( c > 0 )
    {
      FT_Int64  temp, temp2;


      FT_MulTo64( a, b, &temp );
      temp2.hi = (FT_Int32)( c >> 31 );
      temp2.lo = (FT_UInt32)( c / 2 );
      FT_Add64( &temp, &temp2, &temp );
      a = FT_Div64by32( &temp, c );
    }
    else
      a = 0x7FFFFFFFL;

    return ( s < 0 ? -a : a );
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Long )  FT_MulFix( FT_Long  a,
                                       FT_Long  b )
  {
    FT_Long   s;
    FT_ULong  ua, ub;


    if ( a == 0 || b == 0x10000L )
      return a;

    s  = a; a = ABS(a);
    s ^= b; b = ABS(b);

    ua = (FT_ULong)a;
    ub = (FT_ULong)b;

    if ( ua <= 2048 && ub <= 1048576L )
    {
      ua = ( ua * ub + 0x8000 ) >> 16;
    }
    else
    {
      FT_ULong  al = ua & 0xFFFF;


      ua = ( ua >> 16 ) * ub +
           al * ( ub >> 16 ) +
           ( al * ( ub & 0xFFFF ) >> 16 );
    }

    return ( s < 0 ? -(FT_Long)ua : ua );
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Long )  FT_DivFix( FT_Long  a,
                                       FT_Long  b )
  {
    FT_Int32   s;
    FT_UInt32  q;


    s  = a; a = ABS(a);
    s ^= b; b = ABS(b);

    if ( b == 0 )
    {
      /* check for division by 0 */
      q = 0x7FFFFFFFL;
    }
    else if ( ( a >> 16 ) == 0 )
    {
      /* compute result directly */
      q = (FT_UInt32)( a << 16 ) / (FT_UInt32)b;
    }
    else
    {
      /* we need more bits; we have to do it by hand */
      FT_Int64  temp, temp2;

      temp.hi  = (FT_Int32) (a >> 16);
      temp.lo  = (FT_UInt32)(a << 16);
      temp2.hi = (FT_Int32)( b >> 31 );
      temp2.lo = (FT_UInt32)( b / 2 );
      FT_Add64( &temp, &temp2, &temp );
      q = FT_Div64by32( &temp, b );
    }

    return ( s < 0 ? -(FT_Int32)q : (FT_Int32)q );
  }


  /* documentation is in ftcalc.h */

  FT_EXPORT_DEF( void )  FT_Add64( FT_Int64*  x,
                                   FT_Int64*  y,
                                   FT_Int64  *z )
  {
    register FT_UInt32  lo, hi;


    lo = x->lo + y->lo;
    hi = x->hi + y->hi + ( lo < x->lo );

    z->lo = lo;
    z->hi = hi;
  }


  /* documentation is in ftcalc.h */

  FT_EXPORT_DEF( void )  FT_MulTo64( FT_Int32   x,
                                     FT_Int32   y,
                                     FT_Int64  *z )
  {
    FT_Int32   s;


    s  = x; x = ABS( x );
    s ^= y; y = ABS( y );

    {
      FT_UInt32  lo1, hi1, lo2, hi2, lo, hi, i1, i2;


      lo1 = x & 0x0000FFFF;  hi1 = x >> 16;
      lo2 = y & 0x0000FFFF;  hi2 = y >> 16;

      lo = lo1 * lo2;
      i1 = lo1 * hi2;
      i2 = lo2 * hi1;
      hi = hi1 * hi2;

      /* Check carry overflow of i1 + i2 */
      i1 += i2;
      if ( i1 < i2 )
        hi += 1L << 16;

      hi += i1 >> 16;
      i1  = i1 << 16;

      /* Check carry overflow of i1 + lo */
      lo += i1;
      hi += ( lo < i1 );

      z->lo = lo;
      z->hi = hi;
    }

    if ( s < 0 )
    {
      z->lo = (FT_UInt32)-(FT_Int32)z->lo;
      z->hi = ~z->hi + !( z->lo );
    }
  }


  /* documentation is in ftcalc.h */

  FT_EXPORT_DEF( FT_Int32 )  FT_Div64by32( FT_Int64*  x,
                                           FT_Int32   y )
  {
    FT_Int32   s;
    FT_UInt32  q, r, i, lo;


    s  = x->hi;
    if ( s < 0 )
    {
      x->lo = (FT_UInt32)-(FT_Int32)x->lo;
      x->hi = ~x->hi + !( x->lo );
    }
    s ^= y;  y = ABS( y );

    /* Shortcut */
    if ( x->hi == 0 )
    {
      if ( y > 0 )
        q = x->lo / y;
      else
        q = 0x7FFFFFFFL;

      return ( s < 0 ? -(FT_Int32)q : (FT_Int32)q );
    }

    r  = x->hi;
    lo = x->lo;

    if ( r >= (FT_UInt32)y ) /* we know y is to be treated as unsigned here */
      return ( s < 0 ? 0x80000001UL : 0x7FFFFFFFUL );
                             /* Return Max/Min Int32 if division overflow.  */
                             /* This includes division by zero!             */
    q = 0;
    for ( i = 0; i < 32; i++ )
    {
      r <<= 1;
      q <<= 1;
      r  |= lo >> 31;

      if ( r >= (FT_UInt32)y )
      {
        r -= y;
        q |= 1;
      }
      lo <<= 1;
    }

    return ( s < 0 ? -(FT_Int32)q : (FT_Int32)q );
  }


#ifdef FT_CONFIG_OPTION_OLD_CALCS


  /* two helper functions for FT_Sqrt64() */

  static
  void  FT_Sub64( FT_Int64*  x,
                  FT_Int64*  y,
                  FT_Int64*  z )
  {
    register FT_UInt32  lo, hi;


    lo = x->lo - y->lo;
    hi = x->hi - y->hi - ( (FT_Int32)lo < 0 );

    z->lo = lo;
    z->hi = hi;
  }


  static
  int  ft_order64( FT_Int64*  z )
  {
    FT_UInt32  i;
    int        j;


    i = z->lo;
    j = 0;
    if ( z->hi )
    {
      i = z->hi;
      j = 32;
    }

    while ( i > 0 )
    {
      i >>= 1;
      j++;
    }
    return j - 1;
  }


  /* documentation is in ftcalc.h */

  FT_EXPORT_DEF( FT_Int32 )  FT_Sqrt64( FT_Int64*  l )
  {
    FT_Int64  l2;
    FT_Int32  r, s;


    if ( (FT_Int32)l->hi < 0          ||
         ( l->hi == 0 && l->lo == 0 ) )
      return 0;

    s = ft_order64( l );
    if ( s == 0 )
      return 1;

    r = ft_square_roots[s];
    do
    {
      s = r;
      r = ( r + FT_Div64by32( l, r ) ) >> 1;
      FT_MulTo64( r, r,   &l2 );
      FT_Sub64  ( l, &l2, &l2 );

    } while ( r > s || (FT_Int32)l2.hi < 0 );

    return r;
  }


  FT_EXPORT( FT_Int32 )  FT_SqrtFixed( FT_Int32  x )
  {
    FT_Int64  z;


    z.hi = (FT_UInt32)((FT_Int32)(x) >> 16);
    z.lo = (FT_UInt32)( x << 16 );
    return FT_Sqrt64( &z );
  }


#endif /* FT_CONFIG_OPTION_OLD_CALCS */

#endif /* FT_LONG64 */


/* END */
