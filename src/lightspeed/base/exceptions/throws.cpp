/*
 * throws.cpp
 *
 *  Created on: 6.10.2010
 *      Author: ondra
 */

#include "iterator.h"
#include "outofmemory.h"
#include "pointerException.h"
#include "throws.tcc"


namespace LightSpeed {

void throwNullPointerException(const ProgramLocation &loc) {
    throw NullPointerException(loc);
}


void throwIteratorNoMoreItems(const ProgramLocation &loc,
                                    const std::type_info &typeInfo)
{
    throw IteratorNoMoreItems(loc,typeInfo);
}


void throwWriteIteratorNoSpace(const ProgramLocation &loc,
                                   const std::type_info &subject) {
    throw WriteIteratorNoSpace(loc,subject);
}


void throwAllocatorLimitException(const ProgramLocation &loc,
                                   natural reqCount,
                                   natural availableCount,
                                   const std::type_info &allocatorType) {
    throw AllocatorLimitException(loc,reqCount,availableCount,allocatorType);

}

void exceptionRethrow(const ProgramLocation &loc) {
	Exception::rethrow(loc);
}

void throwOutOfMemoryException(const ProgramLocation &loc,natural reqCount) {
	throw OutOfMemoryException(loc,reqCount);
}

}
