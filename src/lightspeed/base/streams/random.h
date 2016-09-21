#ifndef LIGHTSPEED_STREAMS_RANDOM_H_
#define LIGHTSPEED_STREAMS_RANDOM_H_

#include "../iter/iterator.h"

namespace LightSpeed {


	class Rand: public IteratorBase<uint32_t, Rand > {
	public:
		Rand(uint32_t x):x(x),crry(362436069) {
			getNext();
		}
		Rand():x(1234567),crry(362436069) {
			getNext();
		}
		
		void setSeed(uint32_t x) {
			this->x = x;
			crry = 362436069;
		}


		bool hasItems() const {return true;}
		natural getRemain() const {return naturalNull;}

		const uint32_t &getNext() {
			current = x;
			fetchNext();
			return current;
		}
		const uint32_t &peek()const  {
			return x;
		}
		
	protected:
		uint32_t x;
		uint32_t crry;		
		mutable uint32_t current;
		
		void fetchNext() {
			uint64_t multt = x * (uint64_t)2083801278;
			multt+=crry;
			crry = multt>>32;
			x = multt & ((((uint64_t)1)<<32)-1);
		}
	};


	template<typename T>
	class Random;

	template<>
	class Random<integer>: public IteratorBase<integer, Random<integer> > {
	public:
		Random(integer minimum, integer maximum):offset(minimum),
			mult(maximum-minimum) {}
		Random(integer minimum, integer maximum, uint32_t seed):offset(minimum),
			mult(maximum-minimum),data(seed) {}
		bool hasItems() const {return true;}
		const integer &peek() const {
			return transform(data.peek());
		}
		const integer &getNext() {
			const integer &x = transform(data.getNext());
			return x;
		}

	protected:
		integer offset;
		integer mult;
		mutable integer result;
		Rand data;

		const integer &transform(uint32_t rnd) const {
			int64_t mm = (int64_t)rnd * mult;
			return (result = (integer)(mm >> 32));
		}
	};

	template<>
	class Random<natural>: public IteratorBase<natural, Random<natural> > {
	public:
		Random(natural minimum, natural maximum):offset(minimum),
			mult(maximum-minimum) {}
		Random(natural minimum, natural maximum, int32_t seed):offset(minimum),
			mult(maximum-minimum),data(seed) {}
		bool hasItems() const {return true;}
		const natural &peek() const {
			return transform(data.peek());
		}
		const natural &getNext() {
			const natural &x = transform(data.getNext());
			return x;
		}

	protected:
		natural offset;
		natural mult;
		mutable natural result;
		Rand data;

		const natural &transform(uint32_t rnd) const {
			uint64_t mm = (uint64_t)rnd * mult;
			return (result = (natural)(mm >> 32) + offset);
		}
	};

	template<>
	class Random<double>: public IteratorBase<double, Random<double> > {
	public:
		Random(double minimum, double maximum):offset(minimum),
			mult(maximum-minimum) {}
		Random(double minimum, double maximum, int32_t seed):offset(minimum),
			mult(maximum-minimum),data(seed) {}

		bool hasItems() const {return true;}
		const double &peek() const {
			return transform(data.peek());
		}
		const double &getNext()  {
			return transform(data.getNext());
		}

	protected:
		double offset;
		double mult;
		mutable double result;
		Rand data;

		const double &transform(uint32_t rnd) const {
			double randmax = 4294967296.0;
			double mm = rnd / randmax;
			result = mm * mult + offset;
			return result;
		}
	};


}

#endif /*RANDOM_H_*/

