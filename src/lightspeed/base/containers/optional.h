/*
 * oneitemstorage.h
 *
 *  Created on: 19.8.2009
 *      Author: ondra
 */

#ifndef LIGHTSPEED_CONTAINERS_ONEITEMSTORAGE_H_
#define LIGHTSPEED_CONTAINERS_ONEITEMSTORAGE_H_

#include "autoArray.h"

namespace LightSpeed {


    ///Container that can contain one or zero items
    /**
     * You can use this container as static variable, which can be
     * constructed later on demand.
     */
    template<typename T>
    class Optional: public FlatArray<T, Optional<T>  >{
        typedef FlatArray<T, Optional<T>  > Super;
    public:

        Optional();
        Optional(const T &val);
        Optional(const Optional &val);
        Optional &operator=(const Optional &val);
        ~Optional();

        natural __size() const;

        const T *data() const;

        T *data();

        void set(const T &val);
        void init();
		template<typename X1>
		void init(const X1 &x1);
		template<typename X1,typename X2>
		void init(const X1 &x1,const X2 &x2);
        void unset();
        const T &get() const;
        T &get();
        bool isSet() const;

        T *operator->() {return data();}

        const T *operator->() const {return data();}
        const T &operator *() const {return get();}
        T &operator *() {return get();}


    protected:
        static const natural needSpace = sizeof(T);
        static const natural needAlignItems = (needSpace + sizeof(natural)) / sizeof(natural);
        natural buff[needAlignItems];

        bool &getUsageFlag() {
            bool *ptr = reinterpret_cast<bool *>(buff);
            return ptr[needSpace];
        }
        const bool &getUsageFlg() const {
            const bool *ptr = reinterpret_cast<const bool *>(buff);
            return ptr[needSpace];
        }
        void *getBuff() {
            return buff;
        }
        const void *getBuff() const {
            return buff;
        }
    };


    //--------------------------------------------

    template<typename T>
    Optional<T>::Optional() {
        getUsageFlag() = false;
    }

    template<typename T>
    Optional<T>::Optional(const T &val) {
        getUsageFlag() = false;
        set(val);
    }

    template<typename T>
    Optional<T>::Optional(const Optional &val) {
        getUsageFlag() = false;
        if (val.isSet()) set(val.get());
    }


    template<typename T>
    Optional<T> ::~Optional() {
        unset();
    }

    template<typename T>
    Optional<T> &Optional<T>::operator=(const Optional &val) {
        if (this != &val) {
            unset();
            if (val.isSet()) set(val.get());
        }
        return *this;
    }
    template<typename T>
    natural Optional<T>::__size() const {
        return getUsageFlg()?1:0;
    }

    template<typename T>
    const T *Optional<T>::data() const {
        return reinterpret_cast<const T *>(getBuff());
    }

    template<typename T>
    T *Optional<T>::data() {
        return reinterpret_cast<T *>(getBuff());
    }

    template<typename T>
    void Optional<T>::set(const T &val) {
		unset();
        new(getBuff()) T(val);
        getUsageFlag() = true;
    }

    template<typename T>
    void Optional<T>::init() {
		unset();
        new(getBuff()) T();
        getUsageFlag() = true;
    }

	template<typename T>
	template<typename X1>
	void Optional<T>::init(const X1 &x1) {
		unset();
		new(getBuff()) T(x1);
		getUsageFlag() = true;
	}

	template<typename T>
	template<typename X1, typename X2>
	void Optional<T>::init(const X1 &x1,const X2 &x2) {
		unset();
		new(getBuff()) T(x1,x2);
		getUsageFlag() = true;
	}

    template<typename T>
    void Optional<T>::unset() {
        if (isSet()) {
            getUsageFlag() = false;
            this->data()->~T();

        }
    }
    template<typename T>
    const T &Optional<T>::get() const {
        return *this->data();
    }
    template<typename T>
    T &Optional<T>::get() {
        return *this->data();
    }
    template<typename T>
    bool Optional<T>::isSet() const {
        return getUsageFlg();
    }
}



#endif /* ONEITEMSTORAGE_H_ */
