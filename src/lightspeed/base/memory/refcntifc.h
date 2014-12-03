/*
 * refcntifc.h
 *
 *  Created on: 11.4.2014
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MEMORY_REFCNTIFC_H_
#define LIGHTSPEED_MEMORY_REFCNTIFC_H_
#include "refCntPtr.h"
#include "../interface.h"

namespace LightSpeed {


///Combines RefCntObj and IInterface to reduce count of virtual inheritances on some objects
class IRefCntInterface: public IInterface, public RefCntObj {

};


}

#endif /* LIGHTSPEED_MEMORY_REFCNTIFC_H_ */
