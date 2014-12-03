#ifndef ONEITEMARRAY_H_
#define ONEITEMARRAY_H_


#include "flatArray.h"

namespace LightSpeed {


	///Implements single value as array
	/** Represents single value as one item array
	 */
	template<class T>
	class OneItemArray: public FlatArray<T, OneItemArray<T> > {
		typedef FlatArray<T, OneItemArray<T> > Super;
	
	public:
		OneItemArray() {}
		OneItemArray(const T &val):item(val) {}
		OneItemArray(const OneItemArray &other):item(other.item) {}
		
		
		
		natural length() const {return 1;}

        const T *data() const {
            return &item;
        }
        T *data()  {
            return &item;
        }
		
		
		
	protected:
		mutable T item;


	};


}


#endif /*ONEITEMARRAY_H_*/
