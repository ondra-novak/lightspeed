/*
 * singleton.cpp
 *
 *  Created on: 22.10.2010
 *      Author: ondra
 */

#include "singleton.h"
#include "../../mt/atomic.h"

namespace LightSpeed {



Singleton_StartupInstance &singletonDummyInstance = Singleton<Singleton_StartupInstance>::getInstance();
char singletonDefaultInstance = 0;


void singletonRelease(atomic & cnt)
{
	writeRelease(&cnt,0);

}



void singletonInitUnlock(atomic & cnt)
{
	writeRelease(&cnt,2);
}



bool singletonInitLock( atomic & cnt)
{
	natural p = lockCompareExchange(cnt,0,1);
	while (p == 1) {
		p = lockCompareExchange(cnt,0,1);
	};
	if (p == 0) return true; else return false;
}


}
  // namespace LightSpeed
