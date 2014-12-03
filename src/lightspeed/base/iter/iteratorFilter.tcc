/*
 * iteratorFilter.h
 *
 *  Created on: 24.7.2009
 *      Author: ondra
 */


#ifndef LIGHTSPEED_ITERATORFILTER_TCC_WRITEFN_
#define LIGHTSPEED_ITERATORFILTER_TCC_WRITEFN_

#include "iteratorFilter.h"
#include "../exceptions/iterator.h"

namespace LightSpeed {
    template<typename WrIterator, typename Filter>
    void FilterWrite<WrIterator, Filter>::write(const InputT &item)  {
        try {
            //if filter cannot accept the items ... try first flush the output buffer
            if (!flt.needItems()) flushCycle();
            //then try insert the item ... filter should handle any invalid operation by exception
            flt.input(item);
            //if filter cannot accept items now, ... try to flush result
            //... this prevent delaying data in the internal buffer
            //... and also allows to 1:1 filter optimize resource management:
            //... it can for example keep pointer, because it knows, that inserted item will immediately written
            if (!flt.needItems()) flushCycle();

        } catch (WriteIteratorNoSpace &e) {
            throw WriteIteratorNoSpace(THISLOCATION,typeid(InputT)) << e;
        }
    }

    template<typename RdIterator, typename Filter>
    FilterRead<RdIterator,Filter>::FilterRead(RdIterator rd)
        :Super(rd),fetched(0) {}

    template<typename RdIterator, typename Filter>
    FilterRead<RdIterator,Filter>::FilterRead(RdIterator rd, Filter flt)
        :Super(rd),flt(flt),fetched(0) {}

    template<typename RdIterator, typename Filter>
    FilterRead<RdIterator,Filter>::~FilterRead() {
    	freeBuffer();
    }


    template<typename RdIterator, typename Filter>
    bool FilterRead<RdIterator,Filter>::hasItems() const {
        return fetched == 1|| flt.hasItems() || (this->iter.hasItems() && flt.needItems());
    }

    template<typename RdIterator, typename Filter>
    const typename FilterRead<RdIterator,Filter>::OutputT &
                FilterRead<RdIterator,Filter>::peek() const {
        return getBuffer();
    }
    template<typename RdIterator, typename Filter>
    const typename FilterRead<RdIterator,Filter>::OutputT &
                FilterRead<RdIterator,Filter>::getNext()  {
        const OutputT &res = getBuffer();
        prefreeBuffer();
        return res;
    }

    template<typename RdIterator, typename Filter>
    const typename FilterRead<RdIterator,Filter>::OutputT &
            FilterRead<RdIterator,Filter>::getBuffer() const {
        if (fetched != 1) {
            freeBuffer();
            if (!flt.hasItems())
            	flt.blockFetch(const_cast<FilterRead *>(this)->iter);
            if (!flt.hasItems())
                throwIteratorNoMoreItems(THISLOCATION,typeid(OutputT));
            //store result into the buffer
            new(reinterpret_cast<void *>(buffer)) OutputT(flt.output());
            fetched = 1;
        }
        void *k = buffer;
        return *reinterpret_cast<OutputT *>(k);
    }
    template<typename RdIterator, typename Filter>
    void FilterRead<RdIterator,Filter>::freeBuffer() const {
        if (fetched != 0) {
        	void *k = buffer;
            reinterpret_cast<OutputT *>(k)->~OutputT();
            fetched = 0;
        }
    }
    template<typename RdIterator, typename Filter>
    void FilterRead<RdIterator,Filter>::prefreeBuffer() {
        fetched = 2;
    }

    template<typename WrIterator, typename Filter>
    FilterWrite<WrIterator, Filter>::FilterWrite(WrIterator wr)
		:Super(wr) {}

    template<typename WrIterator, typename Filter>
    FilterWrite<WrIterator, Filter>::FilterWrite(WrIterator wr, Filter flt)
		:Super(wr),flt(flt) {}
    template<typename WrIterator, typename Filter>
    FilterWrite<WrIterator, Filter>::~FilterWrite() 
    	try {
    		//try flush
    		flush();
    	} catch (...) {
    		//if there were exception
    		//throw only, if is safe to throw exception
    		if (std::uncaught_exception()) return;
    	}
    


    template<typename WrIterator, typename Filter>
    bool FilterWrite<WrIterator, Filter>::hasItems() const {
    	return this->iter.hasItems() && flt.needItems();
    }


    template<typename WrIterator, typename Filter>
    void FilterWrite<WrIterator, Filter>::flushCycle() {
    	flt.blockFeed(this->iter);
    }

    template<typename WrIterator, typename Filter>
    void FilterWrite<WrIterator, Filter>::flush() {
    	flt.flush();
    	flushCycle();
    	this->iter.flush();
    }

    template<typename RdIterator, typename Filter>
    template<class Traits>
    natural FilterRead<RdIterator,Filter>::blockRead(FlatArrayRef<OutputT,Traits> buffer, bool readAll) {
    	if (buffer.length() == 0) return 0;
    	natural pos = 0;
    	if (fetched == 1) {
    		buffer[fetched] = getNext();
    		pos+=1;
    	}
    	do {
    		if (!flt.hasItems()) {
    			flt.blockFetch(this->iter);
    		}
    		natural c = flt.blockOutput(buffer.offset(pos).ref());
    		pos+=c;

    	} while (!readAll && pos < buffer.length());
    	return pos;
    }

    template<typename WrIterator, typename Filter>
    template<class Traits>
    natural FilterWrite<WrIterator,Filter>::blockWrite(const FlatArray<typename ConstObject<InputT>::Remove,Traits> &buffer, bool writeAll) {
    	if (buffer.length() == 0) return 0;
    	natural pos = 0;
    	do {
    		if (!flt.needItems()) {
    			flt.blockFeed(this->iter);
    		}
    		natural c = flt.blockInput(buffer.offset(pos));
    		pos+=c;

    	} while (!writeAll && pos < buffer.length());
    	//this is need to handle 1:1 filters correctly
        if (!flt.needItems()) flushCycle();
    	return pos;
    }
    template<typename WrIterator, typename Filter>
    template<class Traits>
    natural FilterWrite<WrIterator,Filter>::blockWrite(const FlatArray<typename ConstObject<InputT>::Add,Traits> &buffer, bool writeAll ) {
    	if (buffer.length() == 0) return 0;
    	natural pos = 0;
    	do {
    		if (!flt.needItems()) {
    			flt.blockFeed(this->iter);
    		}
    		natural c = flt.blockInput(buffer.offset(pos));
    		pos+=c;

    	} while (writeAll && pos < buffer.length());
    	//this is need to handle 1:1 filters correctly
        if (!flt.needItems()) flushCycle();
    	return pos;
    }





}

#endif














