#include <unistd.h>
#include "../timestamp.h"
#include <sys/time.h>
namespace LightSpeed {

	TimeStamp TimeStamp::now() {
		
		struct timeval tv;
		gettimeofday(&tv,NULL);
		return TimeStamp::fromUnix(tv.tv_sec,tv.tv_usec/1000);
	}

}
