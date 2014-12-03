#ifndef __LIGHTSPEED_POINTER
#define __LIGHTSPEED_POINTER

#include "types.h"
#include "compare.h"
#include "memory/releaseRules.h"
#include "memory/scopePtr.h"


namespace LightSpeed {




    template<class T, template<class> class ReleaseRules = ReleaseDeleteRule>
    class CopyPtr: public ScopePtr<T,ReleaseRules> {
    public:
        CopyPtr() {}
        CopyPtr(NullType x):ScopePtr<T,ReleaseRules>(x) {}
        CopyPtr(T *x):ScopePtr<T,ReleaseRules>(x) {}

        CopyPtr(const CopyPtr &other) 
            :ScopePtr<T,ReleaseRules>(new T(*other.ptr)) {
        }
        CopyPtr &operator=(const CopyPtr &other) {
            ScopePtr<T,ReleaseRules>::operator=(new T(*other.ptr));
            return *this;
        }
    };

    template<class T, template<class> class ReleaseRules = ReleaseDeleteRule> 
    class ClonePtr: public ScopePtr<T,ReleaseRules> {

        class ICloneFuncts {
        public:

            virtual T *clone(T *) const = 0;
            virtual void destroy(T *) const = 0;
        };

    public:

        ClonePtr():cloneFunct(nilFuncts()) {}
        ClonePtr(NullType x):ScopePtr<T,ReleaseRules>(x),cloneFunct(nilFuncts()) {}
        
        template<class X>
        ClonePtr(X *x):ScopePtr<T,ReleaseRules>(x),cloneFunct(getTFuncts(x)) {}

        ClonePtr(const ClonePtr<T,ReleaseRules> &other)
            :ScopePtr<T,ReleaseRules>(other.cloneFunct->clone(other.ptr))
            ,cloneFunct(other.cloneFunct) {}

        ClonePtr &operator= (const ClonePtr<T,ReleaseRules> &other) {
            releaseThis();
            this->ptr = other.cloneFunct->clone(other.ptr);
            cloneFunct = other.cloneFunct;
            return *this;
        }

        ~ClonePtr() {
            releaseThis();
        }

    protected:

        ICloneFuncts *cloneFunct;

        static ICloneFuncts *nilFuncts() {

            class Functs: public ICloneFuncts {
            public:
                virtual T *clone(T *) const {return 0;}
                virtual void destroy(T *) const {};
            };
            static Functs functs;
            return &functs;

        }

        template<class X>
        static ICloneFuncts *getTFuncts(X *x) {

            class Functs: public ICloneFuncts {
            public:
                virtual T *clone(T *t) const {return new X(*static_cast<X *>(t));}
                virtual void destroy(T *t) const {
                    ReleaseRules<X> release;
                    release(static_cast<X *>(t));
                };
            };

            static Functs functs;
            return &functs;
        }

        void releaseThis() {
            cloneFunct->destroy(this->ptr);
            this->ptr = 0;
        }

    };
    
    
}

#endif
