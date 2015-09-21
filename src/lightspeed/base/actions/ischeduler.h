#pragma once
#include "promise.h"

namespace LightSpeed {


	class IScheduler {
	public:

		virtual Promise<void> schedule(Timeout tm) = 0;
		

		virtual ~IScheduler() {}

	};

}