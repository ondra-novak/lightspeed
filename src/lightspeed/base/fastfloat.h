#ifndef BREDYLIBS_LIBS_COMMON_FASTFLOAT_H_
#define BREDYLIBS_LIBS_COMMON_FASTFLOAT_H_
#include <limits.h>
#include <math.h>
//#include "debug.h"

namespace LightSpeed
{

//! Returns logarithm of value rounded down
inline int log2Floor(int value) {
  if (value <= 0) return INT_MIN;
  int counter = -1;
  while (value > 0) {
    counter++;
    value >>= 1;
  }
  return counter;
}

//! Returns logarithm of value rounded up
inline int log2Ceil(int value) {
  int result = log2Floor(value);
  if (result >= 0) {
    int b = 1 << result;
    return (b == value) ? result : result + 1;
  }
  else {
    return result;
  }
}

static const float Inv2=0.5;
static const float Snapper=3<<22;

#if _MSC_VER && !_MANAGED


inline float fastRound( float x )
{
	// x must be: -1<<21 < x < 1<<21
	volatile float retVal;
	retVal=x+Snapper;
	retVal-=Snapper;
	return retVal;
}

// fast convert float to int - take care to have rounding mode set

inline int toLargeInt( float f )
{
	int retVal;
	_asm
	{
		fld f;
		fistp retVal;
	}
	return retVal;
}

inline __int64 to64bInt( float f )
{
	__int64 retVal;
	_asm
	{
		fld f;
		fistp retVal;
	}
	return retVal;
}

#else

// portable (non-optimized) version

inline float fastRound( float x ) {return floor(x+0.5f);}
inline int toLargeInt( float x ){return (int)floor(x+0.5f);}
inline int64_t to64bInt( float x ){return (int64_t)floor(x+0.5f);}

#endif

// by Vlad Kaipetsky <vlad@TEQUE.CO.UK>

inline int toInt( float fval )
{

//	assertDebugDesc( fabs(fval)<=0x003fffff,"Range for toInt function is limited");
	fval += Snapper;
	return ( (*(int *)&fval)&0x007fffff ) - 0x00400000;
}
#endif

inline int toInt( double f ){return toInt(float(f));}

inline int toInt(int i) {return i;}

#define toIntFloor(x) toInt((x)-Inv2)
#define toIntCeil(x) toInt((x)+Inv2)

#define fastCeil(x) fastRound((x)+Inv2)
#define fastFloor(x) fastRound((x)-Inv2)

/// floating point modulo - fast implementation
/** result is positive even for negative x */

inline float fastFmod( float x, const float n )
{ // n is often constant expression
	x*=1/n;
	x-=toIntFloor(x); // nearest int
	return x*n;
}

#if _MSC_VER>1300
// some VS .NET 2003 specific overload to handle int to float propagation
__forceinline int abs(unsigned x)
{
	return abs(int(x));
}

__forceinline float fabs(int x) {return fabs(float(x));}
__forceinline float log(int x) {return log(float(x));}
__forceinline float sqrt(int x) {return sqrt(float(x));}
__forceinline float pow(int x, float y) {return pow(float(x),y);}
//__forceinline float pow(float x, int y) {return pow(x,float(y));}

#endif

#define FP_BITS(fp) (*(DWORD *)&(fp))
#define FP_ABS_BITS(fp) (FP_BITS(fp)&0x7FFFFFFF)
#define FP_SIGN_BIT(fp) (FP_BITS(fp)&0x80000000)
#define FP_ONE_BITS 0x3F800000

// float tricks (by NVIDIA?)

inline float FastInv( float p )
{
	// about 6b precision?
	int _i = 2 * FP_ONE_BITS - *(int *)&(p);
	float r = *(float *)&_i;
	return r * (2.0f - (p) * r);
}

// some tricks to speed-up class handling

}
