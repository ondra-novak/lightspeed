#pragma once

#include "impl/md5.h"
#include "../base/containers/constStr.h"
#include "../base/types.h"
#include "../base/countof.h"
#include "../base/iter/nullIterator.h"
#include "../base/iter/iterator.h"

namespace LightSpeed {


	template<typename T, typename NextIter = NullWriteIterator<T,true> >
	class HashMD5: public WriteIteratorBase<T,HashMD5<T,NextIter> > {
	public:

		HashMD5() {
			md5_init(&st);
		}

		HashMD5(const NextIter &nextIter):nextIter(nextIter) {
			md5_init(&st);
		}

		void write(const T &x) {
			md5_append(&st,reinterpret_cast<const md5_byte_t *>(&x),sizeof(x));
			nextIter.write(x);
		}

		template<class Traits>
		natural blockWrite(const FlatArray<typename ConstObject<T>::Remove,Traits> &buffer, bool writeAll) {
			natural k = nextIter.blockWrite(buffer,writeAll);
			md5_append(&st,reinterpret_cast<const md5_byte_t *>(buffer.data()),k*sizeof(T));
			return k;
		}
		template<class Traits>
		natural blockWrite(const FlatArray<typename ConstObject<T>::Add,Traits> &buffer, bool writeAll) {
			natural k = nextIter.blockWrite(buffer,writeAll);
			md5_append(&st,reinterpret_cast<const md5_byte_t *>(buffer.data()),k*sizeof(T));
			return k;
		}

		void finish() {
			md5_finish(&st,_digest);
			_hexdigest[0] = 0;
		}

		void flush() {
			finish();
		}

		bool hasItems() const {return true;}

		ConstStringT<byte> digest() const {return ConstStringT<byte>(_digest,16);}
		ConstStringT<char> hexdigest() const {		
			if (_hexdigest[0] == 0) {
				for (natural i = 0; i < 16; i++) {
					_hexdigest[i*2] = _digest[i] >> 4;
					_hexdigest[i*2+1] = _digest[i] & 0xF;
				}
				for (natural k = 0; k < 32; k++) {
					byte z = _hexdigest[k];
					if ( z<=9) _hexdigest[k] = z+48;
					else _hexdigest[k] = z + 'a' - 10;
				}
			}
			return ConstStringT<char>(_hexdigest,32);
		}

	protected:
		NextIter nextIter;
		md5_byte_t _digest[16];
		md5_state_t st;
		mutable char _hexdigest[32];

	};

}

