/*
 * numtext.cpp
 *
 *  Created on: 12.10.2013
 *      Author: ondra
 */




#include "toString.tcc"

namespace LightSpeed {

#ifdef LIGHTSPEED_PLATFORM_WINDOWS
#undef snprintf
#define snprintf sprintf_s
#endif

ToString<float, char>::ToString(float val, integer precision, bool sci)
{
	char buff[256];
	if (precision < 0) {
		snprintf(buff,255,"%1.*g",int(-precision),double(val));
	} else if (sci) {
		snprintf(buff,255,"%1.*E",int(precision),double(val));
	} else {
		snprintf(buff,255,"%1.*f",int(precision),double(val));
	}
	char *c = buff;
	while (*c) this->buffer.write(*c++);
	commitBuffer();

}

ToString<float, char>::ToString(float val)
{
	char buff[256];
	snprintf(buff,255,"%g",double(val));
	char *c = buff;
	while (*c) this->buffer.write(*c++);
	commitBuffer();

}

ToString<double, char>::ToString(double val, integer precision, bool sci)
{
	char buff[256];
	if (precision < 0) {
		snprintf(buff,255,"%1.*g",int(-precision),double(val));
	} else if (sci) {
		snprintf(buff,255,"%1.*E",int(precision),double(val));
	} else {
		snprintf(buff,255,"%1.*f",int(precision),double(val));
	}
	char *c = buff;
	while (*c) this->buffer.write(*c++);
	commitBuffer();
}

ToString<double, char>::ToString(double val)
{
	char buff[256];
	snprintf(buff,255,"%g",double(val));
	char *c = buff;
	while (*c) this->buffer.write(*c++);
	commitBuffer();

}


}
