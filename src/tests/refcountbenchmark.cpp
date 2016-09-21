/*
 * test.cpp
 *
 *  Created on: 20. 9. 2016
 *      Author: ondra
 */
#include <iostream>
#include <lightspeed/base/types.h>
#include <lightspeed/base/memory/refCounted.h>
#include <lightspeed/base/timestamp.h>
#include <lightspeed/base/memory/refCntPtr.h>


using namespace LightSpeed;

int main(int argc, char **argv) {
	{
	RefCountedPtr<RefCounted> obj = new RefCounted;

	static const natural counts = 1000000;
	obj->interlockedAddRef();

	TimeStamp t1 = TimeStamp::now();
	for (natural i = 0; i < counts; i++) {
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
	}
	TimeStamp t2 = TimeStamp::now();


	natural diff = (t2-t1).getMilis();
	std::cout << "Interlocked + local cache: " << diff << std::endl;

	t1 = TimeStamp::now();
	for (natural i = 0; i < counts; i++) {
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedRelease();
		obj->interlockedAddRef();
		obj->interlockedRelease();
		obj->interlockedAddRef();
		obj->interlockedRelease();
		obj->interlockedAddRef();
		obj->interlockedRelease();
		obj->interlockedRelease();
		obj->interlockedRelease();
		obj->interlockedRelease();
		obj->interlockedRelease();
		obj->interlockedRelease();
		obj->interlockedRelease();
		obj->interlockedRelease();
		obj->interlockedRelease();
		obj->interlockedRelease();
		obj->interlockedRelease();
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedAddRef();
		obj->interlockedRelease();
		obj->interlockedAddRef();
		obj->interlockedRelease();
		obj->interlockedAddRef();
		obj->interlockedRelease();
		obj->interlockedAddRef();
		obj->interlockedRelease();
		obj->interlockedRelease();
		obj->interlockedRelease();
		obj->interlockedRelease();
		obj->interlockedRelease();
		obj->interlockedRelease();
		obj->interlockedRelease();
		obj->interlockedRelease();
		obj->interlockedRelease();
		obj->interlockedRelease();
		obj->interlockedRelease();
	}
	t2 = TimeStamp::now();

	diff = (t2-t1).getMilis();
	std::cout << "Interlocked without cache:  " << diff << std::endl;

	RefCounted::onThreadExit();

	t1 = TimeStamp::now();
	for (natural i = 0; i < counts; i++) {
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
	}
	t2 = TimeStamp::now();

	diff = (t2-t1).getMilis();
	std::cout << "Interlocked forced cache miss:  " << diff << std::endl;

	}
	{
	RefCntPtr<RefCntObj> obj = new RefCntObj;

	static const natural counts = 1000000;

	TimeStamp t1 = TimeStamp::now();
	for (natural i = 0; i < counts; i++) {
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
	}
	TimeStamp t2 = TimeStamp::now();


	natural diff = (t2-t1).getMilis();
	std::cout << "RefCntPtr - single-thread mode: " << diff << std::endl;

	obj->enableMTAccess();

	t1 = TimeStamp::now();
	for (natural i = 0; i < counts; i++) {
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->addRef();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
		obj->release();
	}
	t2 = TimeStamp::now();

	diff = (t2-t1).getMilis();
	std::cout << "RefCntPtr - interlocked mode: " << diff << std::endl;
	}
}

