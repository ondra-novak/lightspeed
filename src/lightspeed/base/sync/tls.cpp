/*
 * tls.cpp
 *
 *  Created on: 22.10.2010
 *      Author: ondra
 */

#include "tls.h"
#include "threadVar.h"
#include "../memory/singleton.h"
#include "nulllock.h"
#include "../iter/sortFilter.tcc"
#include "../../mt/spinlock.h"
#include "tlsalloc.h"

namespace LightSpeed {

	ITLSTable &_stGetTLS() {
		return Singleton<TLSTable<> >::getInstance();
	}

	ITLSAllocator &_stGetTLSAllocator() {
		return Singleton<TLSAllocator >::getInstance();
	}

	static ITLSTable::fn_GetTLS ITLSTable_getInstance = &_stGetTLS;
	static ITLSAllocator::fn_GetTLSAllocator ITLSAllocator_getInstance = &_stGetTLSAllocator;


	ITLSAllocator & ITLSAllocator::getInstance()
	{
		return ITLSAllocator_getInstance();
	}

	void ITLSAllocator::setTLSFunction( fn_GetTLSAllocator fn )
	{
		ITLSAllocator_getInstance = fn;
	}

	ITLSAllocator::fn_GetTLSAllocator ITLSAllocator::getTLSFunction()
	{
		return ITLSAllocator_getInstance;
	}

	ITLSTable & ITLSTable::getInstance()
	{
		return ITLSTable_getInstance();
	}

	void ITLSTable::setTLSFunction( fn_GetTLS fn )
	{
		ITLSTable_getInstance = fn;
	}

	ITLSTable::fn_GetTLS ITLSTable::getTLSFunction()
	{
		return ITLSTable_getInstance;
	}
}
