/*
 * argument.h
 *
 *  Created on: 21.8.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_ARGUMENT_H_
#define LIGHTSPEED_BASE_ARGUMENT_H_

namespace LightSpeed {

	template<typename X> struct Argument {typedef const X &T;};
	template<typename X> struct Argument<const X &> {typedef const X &T;};
	template<typename X> struct Argument<X &> {typedef X &T;};

}

#endif /* LIGHTSPEED_BASE_ARGUMENT_H_ */
