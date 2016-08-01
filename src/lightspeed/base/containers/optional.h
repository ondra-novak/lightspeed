/*
 * oneitemstorage.h
 *
 *  Created on: 19.8.2009
 *      Author: ondra
 */

#ifndef LIGHTSPEED_CONTAINERS_ONEITEMSTORAGE_H_
#define LIGHTSPEED_CONTAINERS_ONEITEMSTORAGE_H_

#include <utility>
#include "autoArray.h"
#include "../constructor.h"

namespace LightSpeed {


	///Optional variable can be either contain value or can be null
	/**
	 * Optional variable is by default constructed containing null, but you can specify object or delegated construction to
	 * construct instance.
	 *
	 * In compare to ordinary variable, optional variable doesn't need to have the assignment operator  defined, because assigning
	 * to the Optional causes destroying of the previous instance and constructing the new one. That is because Optional is
	 * great to be used with object that don't have assignment operators. Neither copy constructor is required, because you can
	 * use delegated construction to initialize content (through assignment operator)
	 *
	 * Class has minimal methods, everything is done through operators. To test, whether variable is engaged, simply compare its value
	 * with nil, this comparison returns true, when variable contains nothing.
	 */
	template<typename T>
	class Optional {
	public:

		Optional():_x(0) {}
		Optional(const T &val) {
			construct(val);
			arm();
		}
		template<typename Impl>
		Optional(const Constructor<T,Impl> &c) {
			construct(c);
			arm();
		}
		Optional(NullType):_x(0) {}
		Optional(const Optional &other) {
			if (other._x) {
				construct(other.get());
				arm();
			} else {
				disarm();
			}
		}

		~Optional() {
			if (_x) {
				destruct();
			}
		}


		Optional &operator=(NullType) {
			if (_x) {
				disarm();
				destruct();
			}
			return *this;
		}

		Optional &operator=(const T &val) {
			if (&val != &get()) {
				if (_x) {
					disarm();
					destruct();
				}
				construct(val);
				arm();

			}
			return *this;
		}

		template<typename Impl>
		Optional &operator=(const Constructor<T,Impl> &val) {
			if (_x) {
				disarm();
				destruct();
			}
			construct(val);
			arm();

			return *this;
		}

		Optional &operator=(const Optional &other) {
			if (this != &other) {
				if (_x) {
					disarm();
					destruct();
				}
				if (other._x) {
					construct(other.get());
					arm();
				}
			}
			return *this;
		}


#ifdef LIGHTSPEED_ENABLE_CPP11
		Optional &operator=(T &&val) {
			if (&val != &get()) {
				if (_x) {
					disarm();
					destruct();
				}
				construct(std::move(val));
				arm();

			}
			return *this;
		}
		Optional(Optional &&other) {
			if (other._x) {
				construct(std::move(other.get()));
				arm();
			} else{
				disarm();
			}
			other = nil;
		}
		Optional(T &&val) {
			construct(std::move(val));
			arm();
		}
		Optional &operator=(Optional &&other) {
			if (this != &other) {
				if (_x) {
					disarm();
					destruct();
				}
				if (other._x) {
					constuct(std::move(other.get()));
					arm();
					other = nil;
				}
			}
			return *this;
		}

#endif

		operator const T &() const {
			if (!_x) throwNullPointerException(THISLOCATION);
			return get();
		}

		operator T &() {
			if (!_x) throwNullPointerException(THISLOCATION);
			return get();
		}

		bool operator==(NullType) const {
			return !_x;
		}

		bool operator!=(NullType) const {
			return _x != 0;
		}

		const T *operator->() const {
			if (!_x) throwNullPointerException(THISLOCATION);
			return &get();
		}

		T *operator->() {
			if (!_x) throwNullPointerException(THISLOCATION);
			return &get();
		}

		const T &value() const {
			if (!_x) throwNullPointerException(THISLOCATION);
			return get();
		}
		T &value() {
			if (!_x) throwNullPointerException(THISLOCATION);
			return get();
		}


	protected:
		byte buffer[sizeof(T)];
		T *_x;

		inline void arm() {
			_x = &get();
		}
		inline void disarm() {
			_x = 0;
		}

		inline T &get() {
			void *buff = buffer; //need to break strict aliasing - compiler cannot optimize here
			return *reinterpret_cast<T *>(buff);
		}
		inline const T &get() const {
			const void *buff = buffer; //need to break strict aliasing - compiler cannot optimize here
			return *reinterpret_cast<const T *>(buff);
		}

		inline void destruct() {
			get().~T();
		}

		template<typename Impl>
		void construct(const Constructor<T,Impl> &c) {
			c.construct(buffer);
		}
		void construct(const T &val) {
			new(reinterpret_cast<void *>(buffer)) T(val);
		}
#ifdef LIGHTSPEED_ENABLE_CPP11
		void construct(T &&val) {
			new(reinterpret_cast<void *>(buffer)) T(std::move(val));
		}
#endif
	};

#if 0
    ///Container that can contain one or zero items
    /**
     * You can use this container as static variable, which can be
     * constructed later on demand.
     *
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

        natural length() const;

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
        bool empty() const {return !isSet();}


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
    natural Optional<T>::length() const {
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

#endif
}



#endif /* ONEITEMSTORAGE_H_ */
