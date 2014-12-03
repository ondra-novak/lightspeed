/**@file
 * Classes to help create singletons of other classes on demand
 * @see Singleton class
 */

#ifndef LIGHTSPEED_SINGLETON_H_
#define LIGHTSPEED_SINGLETON_H_



#include <stdlib.h>
#include <stddef.h>
#include <new>
#include "../types.h"
#include "../../mt/atomic_type.h"

namespace LightSpeed {

    ///Contains helper classes to implement Singleton class
    namespace _intr {

        ///Declares singleton control chain
        struct SingletonChunkBase {

            SingletonChunkBase *next;
            void (*Destroy)(SingletonChunkBase *t);
            ///State of singleton initialization
            /**
             * @retval 0 singleton is not initialized yet
             * @retval 1 some thread currently initializes the singleton. Other
             *   threads must wait to finalize initialization
             * @retval 2 singleton is fully initialized
             */
            atomic inited;

            ///Controls singleton chain
            /**
             * @param append contains pointer to be appended at the head of
             *   the chain. Must not be NULL. If NULL used, function detaches
             *   current chain and creates new one. Detached chain is returned
             *   as return value.
             * @return If "append" parameter is non-NULL, function returns
             *   head of chain (which is always "append" instance). If
             *   parameter "append" is NULL, function returns detached chain.
             *
             * @note If function is called first-time, function registers
             *   SingletonAtExit() as atexit handler.
             */
            static SingletonChunkBase *SingletonChunkHead(SingletonChunkBase *append) {

                static SingletonChunkBase *list = 0;
                static bool atexitRegistered = false;

                if (append) {
                    append->next = list;
                    list = append;
                    if (!atexitRegistered) {
                        atexit(&SingletonAtExit);
                        atexitRegistered = true;
                    }
                    return list;
                } else {
                    SingletonChunkBase *res = list;
                    list = 0;
                    return res;
                }

            }

            ///Called as atexit handler.
            /**
             * It detaches singleton control chain and calls Destroy on
             * each singleton. After this is done, function tries to detach
             * chain again (to handle registered instances during destruction)
             * and repeats the action. Function exits, when all classes
             * are destroyed and there is no chain.
             */
            static void SingletonAtExit() {
                SingletonChunkBase *x = SingletonChunkHead(0);
                while (x) {
                    while (x) {
                        SingletonChunkBase *t = x;
                        x = x -> next;
                        (t->Destroy)(t);
                    }
                    x = SingletonChunkHead(0);
                }
            }
        };

    }

    struct Singleton_StartupInstance {
    	int val;
    	Singleton_StartupInstance():val(0) {}
	};

	extern Singleton_StartupInstance &singletonDummyInstance;

	///Controls spinlock installed during singleton initialization - acquire the lock
	/**
	 * @param cnt referece to variable that implements lock
	 * @retval true thread has permition to initialize singleton
	 * @retval false thread cannot initialize singletion because another
	 * thread already did
	 *
	 * @note cnt variable can contain three states. State 0 is used, when
	 * singleton's object is not (yet) initialized. State 1 is used, when
	 * there is thread currently initializing the singleton's object.
	 * Other threads will wait in infinite spin-lock, until thread is inited
	 * State 2 is used for already initialized singleton's object
	 *
	 *
	 */
	bool singletonInitLock( atomic &cnt);

	///Unlocks spin-lock after object is fully initialized
	/**
	 *
	 * @param cnt spinlock counter
	 */
	void singletonInitUnlock( atomic &cnt);


	///Releases singleton's object.
	/**
	 * @param cnt spinlock counter
	 *
	 * this function is called after singleton's object is destroyed, or
	 * when construction of singleton throws an exception. It resets counter to zero.
	 *
	 */
	void singletonRelease( atomic &cnt);

	extern char singletonDefaultInstance;

    ///Singleton
    /**
     * Singleton class is support class to create singletons. You can derive
     * it to create class designed as singleton, or you can use this
     * class directly to create singleton on deamand.
     *
     * Template has two parameters
     * @tparam T specifies name of class, which singleton will be handled
     * @tparam spec specifies class specific ID. This allows you to create
     * more then one "singleton" from one class. For example for private
     * singletons, that will be configured by special way. References to
     * same ID will refer the same singleton instance.
     *
     *
     * @note Singletons are registered on special "atexit" handler. They
     * are destroyed in reversed order of creation (they can be created on
     * first usage). Handler is registered on first usage of any singleton,
     * but early than program reaches the main function.
     *
     * Singletons can be referenced after they are destroyed. In that case,
     * class will recreate the singleton and destroyes it after current
     * chain "atexit" is processed.
     * @tparam T type of singleton
     * @tparam nodestroy Never destroy the singleton. Don't use, if singleton
     *   allocates some resources. Good for singletons with vtables to prevent
	 *   overwriting vtable with _purecall.
	 * @tparam specifies instance of singleton. You can have multiple instances
	 *   To set instance create a dummy variable and use its address as parameter.
	 *   Note that variable must have external linkage
     */
    template<class T, bool nodestroy = false, const char *inst = &singletonDefaultInstance >
    class Singleton {

        struct SingletonData {
            _intr::SingletonChunkBase base;
            char buff[sizeof(T)];

            static void DestroyInst(_intr::SingletonChunkBase *t) {
                SingletonData *self =
                    reinterpret_cast<SingletonData *>(
                            reinterpret_cast<char *>(t) -
                                offsetof(SingletonData,base));
                void *k = self->buff;
                reinterpret_cast<T *>(k)->~T();
                singletonRelease(self->base.inited);
            }
            T *InitInst() {
                if (base.inited  != 2) {
                	if (singletonInitLock(base.inited)) {
                		try {
                			new(buff) T();
                		} catch (...) {
                			singletonRelease(base.inited);
                			throw;
                		}
						if (!nodestroy) base.SingletonChunkHead(&base);
						(void)singletonDummyInstance;
            			singletonInitUnlock(base.inited);
                	}
                }
                return reinterpret_cast<T *>(buff);
            }
            template<typename Param>
            T *InitInst(const Param &p) {
                if (base.inited  != 2) {
                	if (singletonInitLock(base.inited)) {
                		try {
                			new(buff) T(p);
                		} catch (...) {
                			singletonRelease(base.inited);
                			throw;
                		}
						if (!nodestroy) base.SingletonChunkHead(&base);
						(void)singletonDummyInstance;
            			singletonInitUnlock(base.inited);
                	}
                }
                return reinterpret_cast<T *>(buff);
            }
        };

        static SingletonData &getSingletonData() {
            static SingletonData instd = {
                    {0,&SingletonData::DestroyInst,0},{0}
            };
            return instd;
        }

    public:

        ///Retrieves instance of singleton
        /**
         * @return Reference of the singleton. It is better to store
         * result for fast access, but don't use stored value in destructor
         * - instead call this function again in destructor to ensure, that
         * reference is still valid (If class is statically allocated,
         * referred singleton may be already destroyed in destructor, calling
         * getInstance() will recreate it
         */
        static T &getInstance() {

        	SingletonData &instd = getSingletonData();
            return *instd.InitInst();
        }

        ///Retrieves instance of singleton
        /**
         * @param p reference to an object used as parameter of
         *  the constructor. Note that parameter is used only first
         *  time of creation and any usage later will ignore parameter.
         * @return Reference of the singleton. It is better to store
         * result for fast access, but don't use stored value in destructor
         * - instead call this function again in destructor to ensure, that
         * reference is still valid (If class is statically allocated,
         * referred singleton may be already destroyed in destructor, calling
         * getInstance() will recreate it
         */
        template<typename Param>
        static T &getInstance(const Param &p) {

        	SingletonData &instd = getSingletonData();
            return *instd.InitInst(p);
        }

		template<typename Ifc>
		static Ifc &castInstance() {
			return getInstance();
		}

	};

}
#endif /*SINGLETON_H_*/
