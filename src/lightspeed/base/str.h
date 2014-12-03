/*
 * str.h
 *
 *  Created on: 22.3.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_STR_H_
#define LIGHTSPEED_STR_H_

#include "containers/constStr.h"


namespace LightSpeed {

template<typename T>
class ToString;


template<typename T>
ToString<T> str(const T &obj) {
	return ToString<T>(obj);
}

}

#endif /* STR_H_ */
