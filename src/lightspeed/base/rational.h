/*
 * ratio.h
 *
 *  Created on: 18.3.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_RATIO_H_
#define LIGHTSPEED_RATIO_H_

#pragma once

#include <math.h>
#include "types.h"
#include "compare.h"
#include "str.h"

namespace LightSpeed {


	///Rational numbers.
	/**
	 * Number is stored as a fraction of two integers. First
	 * one is numerator and second one is denominator. All calculations
	 * are done using integer operations, so there are no rounding errors
	 * like while using floats.
	 *
	 * numerator is integer and carries also sign. denominator is
	 * always positive, it is declared a natural.
	 *
	 * By default, after every calculation, numbers are normalized
	 * to the minimal form using gcd (Greatest Common Divider)
	 *
	 * To construct rational number you can use single integer number
	 * both numerator and denominator or standard float number. Note
	 * because floats are rounded, you cannot always convert rational
	 * to float and back without small precision lost.
	 *
	 * You can use rational numbers in financial calculations, where rounding
	 * errors can be multiplied after each row is calculated. Because
	 * rational numbers are rounding errors free, these errors will not appear
	 * in these types of calculations.
	 *
	 */
	class rational: public ComparableLess<rational> {
	public:

		///numerator
		integer n;
		///denominator
		natural d;


	public:

		///keywoard to force normalization
		enum Normalized {normalize};

		///constructs rational number
		/**
		 * @param n numerator
		 * @param d denominator
		 *
		 * Constructor expects normalized numbers, but will not check
		 * it. You can use this constructor to construct not-normalized
		 * numbers
		 */
		rational(integer n, natural d):n(n),d(d) {}

		///constructs normalized rational number
		/**
		 * @param n numerator
		 * @param d denominator
		 * @param norm specify keyword normalize to perform normalization
		 *
		 * Will always check, whether number is normalized and perform
		 * normalization if it is necessary
		 */
		rational(integer n, natural d, Normalized /*norm*/)
					:n(n),d(d) {_normalize();}

		///constructs rational number from integer
		/**
		 * @param n integer number
		 *
		 * Result is number, where numerator is equal to integer
		 * and denominator is 1
		 */
		rational(integer n):n(n),d(1) {}

		///constructs from copy and enforces normalization
		/**
		 * By default copy constructor will not make normalization because
		 *   expects, that numbers are already normalized. This constructor
		 *   enforces normalization
		 * @param other source rational number
		 * @param norm specify keyword normalize to perform normalization
		 */
		rational(const rational &other, Normalized /*norm*/)
					:n(other.n),d(other.d) {_normalize();}

		///constructs zero
		rational():n(0),d(1) {}

		///constructs rational number from float number
		/**
		 * @param n float number
		 * constructor will repately multiply number by 2 until it
		 * reaches number without any decimal digits. Then it converts
		 * this number into integer as numerator and stores multiplicator
		 * as denominator
		 *
		 */
		rational(double n) {
			loadFloatNumber(n);
		}

		///Constructs nil rational.
		/**
		 * Nil rational is number, which has denominator equal to zero
		 */
		rational(NullType /*x*/):n(0),d(0) {}


		///calculates gcd of two integers
		static natural gcd(natural u, natural v)  {
			int shift;

			 /* GCD(0,x) := x */
			 if (u == 0 || v == 0)
			   return u | v;

			 /* Let shift := lg K, where K is the greatest power of 2
				dividing both u and v. */
			 for (shift = 0; ((u | v) & 1) == 0; ++shift) {
				 u >>= 1;
				 v >>= 1;
			 }

			 while ((u & 1) == 0)
			   u >>= 1;

			 /* From here on, u is always odd. */
			 do {
				 while ((v & 1) == 0)  /* Loop X */
				   v >>= 1;

				 /* Now u and v are both odd, so diff(u, v) is even.
					Let u = min(u, v), v = diff(u, v)/2. */
				 if (u < v) {
					 v -= u;
				 } else {
					 natural diff = u - v;
					 u = v;
					 v = diff;
				 }
				 v >>= 1;
			 } while (v != 0);

			 return u << shift;

		}

		///returns signature of number
		/**
		 * @retval 1 positive value
		 * @retval 0 zero
		 * @retval -1 negative value
		 */
		integer sign() const {
			if (n < 0) return -1;
			if (n > 0) return 1;
			return 0;
		}

		///returns absolute value
		rational abs() const {
			if (n < 0) return rational(-n,d);
			else return rational(n,d);
		}

		///swaps numerator and denominator resulting inverted number 1/x
		rational inv() const {
			integer tmpn = n,tmpd = d;
			if (tmpn < 0) return rational(-tmpd,-tmpn);
			else return rational(tmpd,tmpn);
		}

		///returns sum of two rational numbers
		/**
		 * @param other right side or operator
		 * @return result of sum (normalized)
		 */
		rational operator+(const rational &other) const {
			return rational(n*other.d + other.n * d, d * other.d, normalize);
		}
		///returns subtraction of two rational numbers
		/**
		 * @param other right side or operator
		 * @return result of subtraction (normalized)
		 */
		rational operator-(const rational &other) const {
			return rational(n*other.d - other.n * d, d * other.d, normalize);
		}

		///returns multiplication of two rational numbers
		/**
		 * @param other right side or operator
		 * @return result of multiplication (normalized)
		 */
		rational operator*(const rational &other) const {
			return rational(n*other.n, d * other.d, normalize);
		}
		///returns division of two rational numbers
		/**
		 * @param other right side or operator
		 * @return result of division (normalized)
		 */
		rational operator/(const rational &other) const {
			return (*this) * other.inv();
		}

		///Returns remainder
		/** It is calculated using subtraction original result by
		 * divided, rounded and multiplied back number
		 *
		 * @param other denominator
		 * @result remainder of division
		 */
		rational operator%(const rational &other) const {
			integer subRes = ((*this) / other).floor();
			if (subRes < 0) subRes++;
			rational mult = rational(subRes) * other;
			return (*this) - mult;

		}

		rational &operator+=(const rational &other)  {
			(*this) = (*this) + other; return *this;
		}
		rational &operator-=(const rational &other)  {
			(*this) = (*this) - other; return *this;
		}
		rational &operator*=(const rational &other)  {
			(*this) = (*this) * other; return *this;
		}
		rational &operator/=(const rational &other)  {
			(*this) = (*this) / other; return *this;
		}

		///Implements comparsion to allows to use compare operations
		/**
		 * @param other other side
		 * @retval this is less than other
		 * @retval this is not less than other
		 */
		bool lessThan(const rational &other) const {
			return (n*(integer)other.d - other.n * (integer)d) < 0;
		}


		///Tests, whether number is nil
		bool isNil() const {
			return d == 0;
		}

		///converts into float
		/**
		 * @return number as float
		 */
		float asFloat() const {return (float)n / (float)d;}
		///converts into double
		/**
		 * @return number as double
		 */
		double asDouble() const {return (double)n / (double)d;}

		///converts into float
		/**
		 * @return number as float
		 */
		operator float() const {return asFloat();}
		///converts into double
		/**
		 * @return number as double
		 */
		operator double() const {return asDouble();}

		///calculates largest integer number less or equal to current rational number
		/**
		 * @return largest integer that is less or equal to current rational number
		 *
		 * example: rational(11,4).floor()  = 2
		 */
		integer floor() const {
			integer intr = n / d;
			integer frac = n % d;
			if (frac < 0) return intr-1;
			else return intr;
		}

		///calculates smallest integer number greater or equal to current rational number
		/**
		 * @return smallest integer number greater or equal to current rational number
		 *
		 * example: rational(11,4).ceil()  = 3
		 */
		integer ceil() const {
			integer intr = n / d;
			integer frac = n % d;
			if (frac > 0) return intr+1;
			else return intr;
		}

		///calculates nearest integer
		/**
		 * @return nearest integer
		 *
		 * example: rational(11,4).round()  = 3
		 * 			rational(9,4).round()  = 2
		 */
		integer round() const {
			return ((*this) + rational(1,2)).floor();
		}

		///calculates power of rational number
		/**
		 * @param expo exponent. If
		 * @return
		 */
		rational power(integer expo) const  {
			if (expo == 0) return rational((integer)1);
			if (expo <  0) return inv().power(-expo);
			return rational(powerNumber(n,expo),powerNumber(d,expo),normalize);
		}

		template<typename Arch>
		void serialize(Arch &arch) {
			arch(n);
			arch(d);
		}

	protected:
		template<typename T>
		static T powerNumber(T n, natural expo) {
			T acc = 1;
			T cur = n;
			while (expo) {
				if (expo & 1) acc *= cur;
				cur *= cur;
				expo >>= 1;
			}
			return cur;
		}


		void loadFloatNumber(double  f) {
			integer sig = 1;
			if (f < 0 ) {
				sig = -1;
				f = -f;
			}

			natural multiplier = 1;
			double mf = f;
			while (::floor(mf) < mf) {
				multiplier *= 2;
				mf = f * multiplier;
			}
			n = (natural)::floor(f * multiplier) * sig;
			d = multiplier;
			_normalize();
		}

		///normalizes rational number to have gcd() == 1
		void _normalize() {
			natural curgcd = _gcd();
			while (curgcd > 1) {
				n /= curgcd;
				d /= curgcd;
				curgcd = _gcd();
			}
		}
		///calculates gcd of numbers;
		natural _gcd() const {
			if (n < 0) return gcd((natural)(-n),d);
			else return gcd((natural)n,d);
		}
	};



}


#endif /* RATIO_H_ */
