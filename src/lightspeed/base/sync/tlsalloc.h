#include "../iter/sortFilter.h"
/*
 * tlsalloc.h
 *
 *  Created on: 25.8.2011
 *      Author: ondra
 */

#pragma once

#include "../iter/sortFilter.h"
#include "tls.h"

namespace LightSpeed {


///TLSAllocator is singleton class that handles global allocations in TLS table
/**
  Using this class, you are able to reserve / allocate a TLS index in the global
  TLS table. The index is used to access variable in the each TLS of each thread.

  TLS indexes are used, because runtime must identify variable, which will
  be different in each thread. The implementation of this is hidden, if you are
  using ThreadVar<> template, which is also recomended to use for working with the
  thread variables. Using TLSAllocator is low level technique, designed to support
  other LightSpeed classes.

*/


class TLSAllocator: public ITLSAllocator {


public:

    TLSAllocator():nextIndex(0) {}

    ///Allocates index in TLS table
    /**
        @return new index in TLS table.
        @note indexes are recycled. So it is possible, that you receive index,
        which has been released by previous calling of function freeIndex.

        @see freeIndex
    */
    natural allocIndex();

    ///Releases the specified index
    /**
      @param index index to release.
      @note Function releases index only. It will not destroy content of variables
      because they are not accessible by this object. You should release indexes only
      when all threads stop using them. If you release index too soon, another
      ThreadVar variable may get the same index, but content of variable still
      can contain an old value.
      */
    void freeIndex(natural index);


    natural getMaxIndex() const;

protected:

    SortFilter<natural> freeList;
    natural nextIndex;
    TLSAllocator& operator=(const TLSAllocator &);
    TLSAllocator(const TLSAllocator &src);
};

}
