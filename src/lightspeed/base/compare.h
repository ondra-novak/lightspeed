#ifndef LIGHTSPEED_COMPARE_H_
#define LIGHTSPEED_COMPARE_H_

#include "invokable.h"
#include "../base/types.h"

namespace LightSpeed {

    enum CompareResult {
        cmpResultLess = -1,
        cmpResultEqual = 0,
        cmpResultGreater = 1,
        cmpResultNotEqual = 2
    };

    static inline CompareResult swapCompareResult(CompareResult other) {
    	switch (other) {
    	case cmpResultLess: return cmpResultGreater;
    	case cmpResultEqual: return other;
    	case cmpResultGreater: return cmpResultLess;
    	case cmpResultNotEqual: return other;
    	default: return cmpResultNotEqual;
    	}

    }
    
    template<class Master>
    class Comparable: public Invokable<Master> {
        
    public:
        
        
        
        bool operator==(const Comparable<Master> &other) const {
            return this->_invoke().compare(other._invoke()) == cmpResultEqual;
        }
        
        bool operator!=(const Comparable<Master> &other) const {
            return this->_invoke().compare(other._invoke()) != cmpResultEqual;
        }

        bool operator>(const Comparable<Master> &other) const {
            return this->_invoke().compare(other._invoke()) == cmpResultGreater;
        }

        bool operator<(const Comparable<Master> &other) const {
            return this->_invoke().compare(other._invoke()) == cmpResultLess;
        }

        bool operator>=(const Comparable<Master> &other) const {
            return this->_invoke().compare(other._invoke()) != cmpResultLess;
        }

        bool operator<=(const Comparable<Master> &other) const {
            return this->_invoke().compare(other._invoke()) != cmpResultGreater;
        }
    };
   


    template<class Master>
    class ComparableLess: public Invokable<Master> {
    public:
        bool operator==(const ComparableLess<Master> &other) const { 
            return (*this >= other) && (other >= *this);     
        }
        bool operator!=(const ComparableLess<Master> &other) const { 
            return (*this < other) || (other < *this);     
        }
        bool operator>(const ComparableLess<Master> &other) const { 
            return other < *this;      
        }
        bool operator<(const ComparableLess<Master> &other) const { 
            return this->_invoke().lessThan(other._invoke());
        }
        bool operator>=(const ComparableLess<Master> &other) const { 
            return !this->_invoke().lessThan(other._invoke());
        }
        bool operator<=(const ComparableLess<Master> &other) const { 
            return other >= *this;                 
        }        
        bool operator==(const NullType &/*x*/) const {
            return this->_invoke().isNil();     
        }
        bool operator!=(const NullType &/*x*/) const {
            return !this->_invoke().isNil();     
        }
    };

    template<class Master>
    class ComparableLessAndEqual: public ComparableLess<Master> {
    public:
        bool operator==(const ComparableLessAndEqual<Master> &other) const { 
            return this->_invoke().equalTo(other._invoke());
        }
        bool operator!=(const ComparableLessAndEqual<Master> &other) const { 
            return !this->_invoke().equalTo(other._invoke());
        }
        bool operator==(const NullType &) const {
            return this->_invoke().isNil();     
        }
        bool operator!=(const NullType &) const {
            return !this->_invoke().isNil();     
        }
    };
    
    template<class Master>
    class ComparableEqual: public Invokable<Master> {
    public:
        bool operator==(const ComparableEqual<Master> &other) const { 
            return this->_invoke().equalTo(other._invoke());
        }
        bool operator!=(const ComparableEqual<Master> &other) const { 
            return !this->_invoke().equalTo(other._invoke());
        }
        bool operator==(const NullType &) const {
            return this->_invoke().isNil();     
        }
        bool operator!=(const NullType &) const {
            return !this->_invoke().isNil();     
        }
    };

    template<typename T>
    class DefaultCompare {
    public:
        static bool less(const T &a, const T &b) {
            return a < b;
        }
        static bool equal(const T &a, const T &b) {
            return a == b;
        }

        static CompareResult compare(const T &a, const T &b) {
        	if (a == b) return cmpResultEqual;
        	if (a > b) return cmpResultGreater;
        	if (a < b) return cmpResultLess;
        	return cmpResultNotEqual;
        }
    };

    template<typename T>
    class NoneCompare {
    public:
        static bool less(const T &, const T &) {
            return false;
        }
        static bool equal(const T &, const T &) {
            return false;
        }

        static CompareResult compare(const T &, const T &) {
        	return cmpResultNotEqual;
        }

    };

    template<typename T, typename Cmp, CompareResult res>
    class TestCompare {
    public:
    	Cmp cmp;
    	TestCompare () {}
    	TestCompare (const Cmp &cmp):cmp(cmp) {}

    	bool operator()(const T &a, const T &b) const {
    		return cmp(a,b) == res;
    	}
    };


};

#endif /*COMPARE_H_*/
