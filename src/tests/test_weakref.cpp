/*
 * test_base64.cpp
 *
 *  Created on: 17. 7. 2016
 *      Author: ondra
 */
#include "../lightspeed/base/containers/constStr.h"
#include "../lightspeed/base/framework/testapp.h"
#include "../lightspeed/base/containers/convertString.h"
#include "../lightspeed/base/memory/weakref.h"
#include "../lightspeed/mt/thread.h"

namespace LightSpeed {

defineTest test_weakRefBasic("weakRef.basic","10 1 ",[](PrintTextA &out) {

	int x = 10;
	WeakRefTarget<int> target(&x);
	WeakRef<int> ref1(target);
	WeakRef<int> ref2(ref1);
	{
		WeakRefPtr<int> ptr = ref2.lock();
		out("%1 ") << *ptr;
	}
	target.setNull();
	{
		WeakRefPtr<int> ptr = ref1.lock();
		out("%1 ") << (ptr==null);
	}

});


defineTest test_weakRefThreaded("weakRef.threaded","1 30 1 ",[](PrintTextA &out) {

	int x = 10;
	WeakRefTarget<int> target(&x);
	WeakRef<int> ref(target);
	Thread thr;

	thr >> [ref,&out](){
		{
			WeakRefPtr<int> ptr = ref.lock();
			*ptr = 20;
			Thread::deepSleep(200);
			*ptr += 10;
		}
		{
			WeakRefPtr<int> ptr = ref.lock();
			out("%1 ") << (ptr==null);
		}
	};

	Thread::deepSleep(100);
	target.setNull();
	thr.join();
	{
		WeakRefPtr<int> ptr = ref.lock();
		out("%1 %2 ") << x << (ptr==null);
	}

});

}



