/*
 * resourcePool.h
 *
 *  Created on: 13.5.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_CONTAINERS_RESOURCEPOOL_H__
#define LIGHTSPEED_BASE_CONTAINERS_RESOURCEPOOL_H__
#include "../memory/sharedResource.h"
#include "../../mt/slist.h"
#include "../../mt/timeout.h"
#include "../../mt/atomic_type.h"
#include "../../mt/syncPt.h"






namespace LightSpeed {

///Abstract resource object
/** This represents any resource which is allocated for threads
 *
 * Resources are allocated from pool (AbstractResourcePool). Pool is limited and when exhausted
 * other threads has to wait to release resource.
 *
 * Example: Database connection can be resource while there can be limited count of database connections
 * available for workers
 *
 */
class AbstractResource {
public:
	AbstractResource();
	virtual ~AbstractResource();

	///Returns true, when this resource already expired
	/**
	 * @retval true expired
	 * @retval false not expired yet
	 *
	 * Function can be also used to determine resource state. If resource is not in good
	 * condition, it should return true and pool manager removes the resource from the pool.
	 */
	virtual bool expired() const;
	///Sets new expiration time
	/** Expiration time is represented by timeout objec
	 * @param expiration new expiration
	 */
	void setTimeout(Timeout expiration);


protected:
	AbstractResource *next;
	Timeout expiration;
	friend class AbstractResourcePool;

};


class AbstractResourcePtr;

///Abstract resource pool
/**
 * To use pool, you have to implement abstract method createResource().
 *
 * Pool can define limit of resource count, how long resource can be cached until expired (i.e. due
 * idle-timeout) and how long thread can wait for resource until timeout is reported
 */
class AbstractResourcePool: private SyncPt {
public:
	///Install resource pool
	/**
	 * @param limit limit to maximal count of resources
	 * @param resTimeout how long resource can be cached until expired}in milliseconds]
	 * 		Setting zero causes, that resource will be destroyed if there is no waiting threads
	 * @param waitTimeout
	 * 		How long thread can wait in milliseconds
	 *
	 *
	 * To retrieve resource, create AbstractResourcePtr object (or ResourcePtr<> object)
	 */
	AbstractResourcePool(natural limit, natural resTimeout, natural waitTimeout);
	///destroyes pool and removes all idle resources
	/** Ensure, that every resource has been released. Destructor doesn't perform any checking
	 *
	 */
	virtual ~AbstractResourcePool();


	///Changes limit
	void setLimit(natural newlimit);
	natural getLimit() const;
	natural getCurLimit() const;
public:

	AbstractResource *pool;

	atomic curLimit;
	natural resTimeout;
	natural waitTimeout;
	natural limit;

	virtual AbstractResource *createResource() = 0;
	virtual const char *getResourceName() const = 0;

	friend class AbstractResourcePtr;

	void release(AbstractResource *a);
	AbstractResource *acquire();


};

///Contains resource
/** this is base class for template class ResourcePtr
 *
 * @note AbstractResourcePtr is SharedResource. You can share pointer
 * on many places and you still have one instance of the resource. When
 * last pointer is released, resource is returned back to the pool
 *
 * SharedResource object has been designed to share pointer for limited
 * count of instances. Actually, there is no limit in count, but every
 * shared instance can degrade performance especially when releasing
 * it (shared instances are kept in linked-list)
 */
class AbstractResourcePtr: public SharedResource{
public:

	///Construct
	/** Construct pointer and acquire resource from the pool
	 *
	 * @param pool reference to the pool. This reference must remain valid,
	 * until all resources is released
	 */
	explicit AbstractResourcePtr(AbstractResourcePool &pool);
	///Destroy pointer
	/** Destroy pointer and release resource if it is necesery
	 *
	 */
	~AbstractResourcePtr();

	///retrieve link to the resource
	AbstractResource *get() const {return ptr;}

protected:
	AbstractResource *ptr;
	AbstractResourcePool &pool;
private:
	AbstractResourcePtr &operator=(const AbstractResource &);


	friend class AbstractResourcePool;
};



///Pointer to specified resource
/**
 * @tparam T type of the resource. Resource must inherit AbstractResource class
 */
template<typename T>
class ResourcePtr: public AbstractResourcePtr {
public:

	ResourcePtr(const AbstractResourcePtr &other):AbstractResourcePtr(other) {}
	ResourcePtr(AbstractResourcePool &other):AbstractResourcePtr(other) {}

    ///Member access
    /**
     * @return pointer to object
     * @exception NullPointerException if used with null. This feature can
     * be disabled defining preprocessor variable LIGHTSPEED_NOCHECKPTR
     */
    T *operator->() const {
    	return static_cast<T *>(this->ptr);
    }
    ///Dereference
    /**
     * @return reference  to object
     * @exception NullPointerException if used with null. This feature can
     * be disabled defining preprocessor variable LIGHTSPEED_NOCHECKPTR
     */
    T &operator*() const {
    	return *static_cast<T *>(this->ptr);
    }
    ///conversion to pointer
    /**
     * @return content of instance
     */
    operator T *() const {
    	return static_cast<T *>(this->ptr);
    }

    ///current pointer
    /**
     * @return current pointer value
     */
    T *get() const {
    	return static_cast<T *>(this->ptr);
    }


    typedef T ItemT;

};

} /* namespace jsonsrv */
#endif /* LIGHTSPEED_BASE_CONTAINERS_RESOURCEPOOL_H__ */
