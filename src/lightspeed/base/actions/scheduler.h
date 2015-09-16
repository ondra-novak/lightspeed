/*
 * scheduler.h
 *
 *  Created on: 16. 9. 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_ACTIONS_SCHEDULER_H_
#define LIGHTSPEED_BASE_ACTIONS_SCHEDULER_H_

#include "schedulerOld.h"


namespace LightSpeed {

#ifndef LIGHTSPEED_MT_SCHEDULER_H_
typedef OldScheduler Scheduler;
#endif

}

#endif /* LIGHTSPEED_BASE_ACTIONS_SCHEDULER_H_ */
