#pragma once

#ifndef LIGHTSPEED_BASE_TYPES
#define LIGHTSPEED_BASE_TYPES

#include "platform.h"

#include <cstdio>



namespace LightSpeed
{

    using std::size_t;

    ///generic int declaration depends on current platform
    /**
     * On most 64-bit platforms, int stays on 32-bit and long is 64-bit
     * On most 32-bit platforms, int and long are equal
     * LightSpeed will use integer type as generic integer that respects
     * current preferred bit width
     */
    typedef intptr_t integer;

    ///natural type, that holds not negative values
    /**
     * uses CPU preferred bit width
     */
    typedef uintptr_t natural;
    
    ///defines smallest unit on the platform
    /**
     * in most of cases, this type can hold one byte ... unsigned 
     * */
    typedef unsigned char byte;
    
    ///Defines the largest integer type
    /** for example disk offsets, memory address, etc
     */
    typedef int64_t linteger;

    ///Defines the largest natural type
    /** for example disk offsets, memory address, etc
     */
    typedef uint64_t lnatural;

    ///Defines type for the Ascii equivalent
    /** This type is used to representing text characters while they
     * are converted from or to another user type. Any object of user
     * type which can act as text character have to be convertible into
     * the ascii equivalent.
     *
     * @see TextControlCharacter
     */
#ifndef LIGHTSPEED_USE_CHAR_AS_ASCII
    typedef wchar_t Ascii;
#else
    typedef char Ascii;
#endif


    
    static const natural naturalNull = (natural)-1;
    static const lnatural lnaturalNull = (lnatural)-1;
    static const integer integerNull = integer(~(naturalNull>>1));
    static const linteger lintegerNull = linteger(~(lnaturalNull>>1));
    
    ///declaration nil typu
    enum Null_t {
        nil = 0,
        null = 0,
    };

    typedef Null_t NullType;

    ///Empty type
    /** Type is used wherever is need to express that argument is not used.
     * Using empty type must be supported by callee. For example operator[] requires
     * one argument. To create empty JSON array you can use empty as argument.
     *
     * There is difference between nil and empty. Value nil (aka Null_t) is sometimes used to
     * specify, that variable has no value. Value empty is used to specify, that there
     * is no value at all. Callee can detect nil as value and store it (for example
     * an array contains single nil value). In contrast, empty should not be never used
     * as an value
     */
    enum Empty_t {
    	empty//!< empty
    };
    
    
    ///Enable/disable enumeration
    enum EnabledState {
    	disabled = 0,
    	enabled = 1
    };
    
    ///Defines 9-valued logic state
    enum LogicState {    	
    	LS_Undefined = -1,	///<undefined state, it has no current value now
    	LS_Low = 0,			///<It has logical zero (low)
    	LS_High = 1,		///<It has logical one (high)
    	LS_Invalid = 2,		///<Specifies invalid or undefined state (as result of calculation)
    	LS_Inactive = 3,	///<State will not affect any calculation
    	LS_WeakLow = 4,		///<Weak low
    	LS_WeakHigh = 5,	///<Weak high
    	LS_WeakInvalid = 6, ///<Specifies invalid or undefined state (as result of calculation on weak level)
    	LS_DontCare = 7		///<Similar to Inactive, but it should not appear as result... only deactivates logic input.
    };

    

///contains enumeration specifies general directions (for iterators for example)
	namespace Direction {
	
		enum Type {
			
		    current, ///<current direction - defined by iterator (meta)
		    reversed, ///<reversed direction - move iterator to other direction (meta)
			forward, ///<forward iterators 
			backward, ///<backward iterators
			up,  ///<if you want to iterate from bottom to top
			down, ///<if you want to iterate from top to bottom
			left, ///<if you want to iterate from right to left
			right, ///<if you want to iterate from left to right
			outside, ///<if you want to iterate from inside to outside
			inside, ///<if you want to iterate from outside to inside
			clockwise, ///<if you want to iterate in clockwise order
			counterclockwise, ///<if you want to iterate in counterclockwise order
			pingpong_fw, ///<pingpong, first direction will be forward
            pingpong_bk, ///<pingpong, first direction will be backward
			ascent, ///<processing items in ascent order (from lower to highter)
			descent, ///<processing items in descent order (from highter to lower)
			undefined, ///<undefined direction
			nowhere, ///<no direction, stay on place
			absolute, ///<no direction, specified absolute position

		};
		
		inline Type reverse(Type x) {
	    	switch (x) {
				case current: return reversed;
				case reversed: return current;
				case forward: return backward;
				case backward: return forward;
				case up: return down;
				case down: return up;
				case left: return right;
				case right: return left;
				case outside: return inside;
				case clockwise: return counterclockwise;
				case counterclockwise: return clockwise;
				case pingpong_fw: return pingpong_bk;
				case pingpong_bk: return pingpong_fw;
				case ascent: return descent;
				case descent: return ascent;
				default: return x;
	    	}
	    }

	}
	
	namespace Bin {

	    ///natural number 8 bit long
    	typedef uint8_t natural8;
        ///natural number 16 bit long
    	typedef uint16_t natural16;
        ///natural number 32 bit long
        typedef uint32_t natural32;
        ///natural number 64 bit long
        typedef uint64_t natural64;
        ///integer number 8 bit long
        typedef int8_t integer8;
        ///integer number 16 bit long
        typedef int16_t integer16;
        ///integer number 32 bit long
        typedef int32_t integer32;
        ///integer number 64 bit long
        typedef int64_t integer64;
        
        typedef float real32;
        
        typedef double real64;
        
        typedef long double real80;
	}	



}	
	
#endif

