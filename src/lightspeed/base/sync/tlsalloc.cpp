/*
 * tlsalloc.cpp
 *
 *  Created on: 25.8.2011
 *      Author: ondra
 */

#include "tlsalloc.h"
#include "../iter/sortFilter.tcc"
namespace LightSpeed {


natural TLSAllocator::allocIndex() {
    if (!freeList.hasItems())
        return nextIndex++;
    else {
        natural idx = freeList.output();
        return idx;
    }

}


void TLSAllocator::freeIndex(natural index) {
    if (index + 1 == nextIndex) {
        --nextIndex;
    }
    else
        if (index < nextIndex)
            freeList.input(index);
}

natural TLSAllocator::getMaxIndex() const {
	return nextIndex - 1;
}


}
