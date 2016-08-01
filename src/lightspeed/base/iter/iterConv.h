/*
 * iterConv.h
 *
 *  Created on: 12. 7. 2016
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_ITER_ITERCONV_H_
#define LIGHTSPEED_BASE_ITER_ITERCONV_H_
#include "../containers/optional.h"
#include "../exceptions/throws.h"
#include "../qualifier.h"
#include "../types.h"
#include "iterator.h"


namespace LightSpeed {


template<typename From, typename To, typename Impl>
class IConverter {
public:

	typedef To ToType;
	typedef From FromType;
	typedef Impl Implementator;

	Impl &getImpl() {return *static_cast<Impl *>(this);}
	const Impl &getImpl() const {return *static_cast<const Impl *>(this);}

	const To &getNext();
	const To &peek() const;
	void write(const From &item);

	template<typename FlatArray, typename Iter>
	natural blockWrite(const FlatArray &array, Iter &target, bool writeAll = true);

	template<typename FlatArray, typename Iter>
	natural blockRead(Iter &source, FlatArray &array, bool readAll = true);

	template<typename Iter>
	void writeToIter(const From &item, Iter &target);

	template<typename Iter>
	const To &getNextFromIter(Iter &source);

	template<typename Iter>
	const To &peekFromIter(const Iter &source) const;

	template<typename Iter>
	bool hasItemsFromIter(const Iter &source) const;


	void flush();

	template<typename Iter>
	void flushToIter(Iter &iter);


	bool hasItems;
	bool needItems;
	bool eolb;

	IConverter();
};


template<typename From, typename To, typename Impl>
class ConverterBase: public IConverter<From,To,Impl> {
public:

	typedef IConverter<From,To,Impl> Super;

	const To &getNext();
	const To &peek() const;
	void write(const From &item);

	template<typename FlatArray, typename Iter>
	natural blockWrite(const FlatArray &array, Iter &target, bool writeAll = true);

	template<typename FlatArray, typename Iter>
	natural blockRead(Iter &source, FlatArray &array, bool readAll = true);

	template<typename Iter>
	void writeToIter(const From &item, Iter &target);

	template<typename Iter>
	const To &getNextFromIter(Iter &source);

	template<typename Iter>
	const To &peekFromIter(const Iter &source) const;

	template<typename Iter>
	void flushToIter(Iter &iter);

	template<typename Iter>
	bool hasItemsFromIter(const Iter &source) const;


	void flush();
};

template<typename Converter, typename SrcIter, bool implicitFlush = true>
class ConvertReadIter: public IteratorBase<typename OriginT<Converter>::T::ToType, ConvertReadIter<Converter, SrcIter> > {
public:
	typedef typename OriginT<Converter>::T::ToType To;
	typedef SrcIter BaseIter;

	ConvertReadIter(typename FastParam<SrcIter>::T srcIter):srcIter(srcIter) {}
	ConvertReadIter(const Converter &init, typename FastParam<SrcIter>::T srcIter):conv(init),srcIter(srcIter) {}
	bool hasItems() const {
		if (conv.hasItemsFromIter(srcIter)) return true;
		//if not, call flush to finish conversion (we need to const cast here, because there is no other place how to handle this)
		if (implicitFlush) const_cast<ConvertReadIter *>(this)->conv.flush();
		//if there are items after flush, continue returning true, otherwise return false
		return conv.hasItems;
	}
	const To &getNext() {
		return conv.getNextFromIter(srcIter);
	}
	const To &peek() const {
		return conv.peekFromIter(srcIter);
	}
    template<class Traits>
    natural blockRead(FlatArrayRef<To, Traits> buffer, bool readAll = true) {
    	return conv.blockRead(srcIter, buffer, readAll);
    }

    typename AddReference<BaseIter>::T getSource() const {return srcIter;}
    Converter &getConverter() {return conv;}
    const Converter &getConverter() const {return conv;}


protected:
    Converter conv;
    SrcIter srcIter;
};

template<typename Converter, typename TrgIter, bool implicitFlush = true>
class ConvertWriteIter: public WriteIteratorBase<typename OriginT<Converter>::T::FromType, ConvertWriteIter<Converter, TrgIter> > {
public:
	typedef typename OriginT<Converter>::T::ToType To;
	typedef typename OriginT<Converter>::T::FromType From;
	typedef TrgIter BaseIter;

	ConvertWriteIter(typename FastParam<TrgIter>::T trgIter):trgIter(trgIter) {}
	ConvertWriteIter(const Converter &init, typename FastParam<TrgIter>::T trgIter):conv(init),trgIter(trgIter) {}
	~ConvertWriteIter() {
		if (implicitFlush) conv.flushToIter(trgIter);
	}
	bool hasItems() const {
		return trgIter.hasItems();
	}
	void write(const From &from) {
		return conv.writeToIter(from, trgIter);
	}

	typename AddReference<BaseIter>::T getTarget() const {return trgIter;}

    Converter &getConverter() {return conv;}
    const Converter &getConverter() const {return conv;}

    void flush() {
    	conv.flushToIter(trgIter);
    	trgIter.flush();
    }

    void finish() {
    	conv.flush();
    }

    template<class Traits>
    natural blockWrite(const FlatArray<typename ConstObject<From>::Add,Traits> &buffer, bool writeAll = true) {
    	return conv.blockWrite(buffer,trgIter,writeAll);
    }

    template<class Traits>
    natural blockWrite(const FlatArray<typename ConstObject<From>::Remove,Traits> &buffer, bool writeAll = true) {
    	return conv.blockWrite(buffer,trgIter,writeAll);
    }

	protected:
		Converter conv;
		TrgIter trgIter;
};

template<typename Conv1, typename Conv2>
class ConverterChain: public ConverterBase<typename OriginT<Conv1>::T::FromType,typename OriginT<Conv2>::T::FromType, ConverterChain<Conv1,Conv2> > {
public:

	typedef typename OriginT<Conv1>::T::FromType From;
	typedef typename OriginT<Conv2>::T::ToType To;

	ConverterChain() {}
	ConverterChain(typename FastParam<Conv1>::T conv1, Null_t  = null):conv1(conv1) {}
	ConverterChain(Null_t,typename FastParam<Conv2>::T conv2):conv2(conv2) {}
	ConverterChain(typename FastParam<Conv1>::T conv1, typename FastParam<Conv2>::T conv2):conv1(conv1),conv2(conv2) {}

	const To &getNext() {
		const To &out = conv2.getNext();
		if (!conv2.hasItems) {
			tempStorage = out;
			while (!conv2.hasItems && conv1.hasItems) {
				conv2.write(conv1.getNext());
			}
			this->hasItems = conv2.hasItems;
			this->needItems = conv1.needItems;
			this->eolb = conv1.eolb || conv2.eolb;
			return tempStorage;
		} else {
			this->hasItems = conv2.hasItems;
			this->needItems = conv1.needItems;
			this->eolb = conv1.eolb || conv2.eolb;
			return out;
		}

	}
	const To &peek() const {
		return conv2.peek();
	}
	void write(const From &item) {
		conv1.write(item);
		while (conv1.hasItems && !conv2.hasItems) {
			conv2.write(conv1.getNext());
		}
		this->hasItems = conv2.hasItems;
		this->needItems = conv1.needItems;
	}

	template<typename FlatArray, typename Iter>
	natural blockWrite(const FlatArray &array, Iter &target, bool writeAll = true) {
		ConvertWriteIter<Conv2 &, Iter &, false> outIter(conv2, target);
		return conv1.blockWrite(array, outIter, writeAll);
	}

	template<typename FlatArray, typename Iter>
	natural blockRead(Iter &source, FlatArray &array, bool readAll = true) {
		ConvertReadIter<Conv1 &, Iter &, false> inIter(conv1, source);
		return conv2.blockRead(inIter, array, readAll);
	}

	template<typename Iter>
	void writeToIter(const From &item, Iter &target) {
		ConvertWriteIter<Conv2 &, Iter &, false> outIter(conv2, target);
		conv1.writeToIter(item,outIter);
	}

	template<typename Iter>
	const To &getNextFromIter(Iter &source) {
		ConvertReadIter<Conv1 &, Iter &, false> inIter(conv1, source);
		return conv2.getNextFromIter(inIter);
	}

	template<typename Iter>
	const To &peekFromIter(const Iter &source) const {
		ConvertReadIter<Conv1 &, Iter &, false> inIter(conv1, source);
		return conv2.peek(inIter);
	}

	void flush() {
		conv1.flush();
		while (conv1.hasItems && !conv2.hasItems) {
			conv2.write(conv1.getNext());
		}
		conv2.flush();
		this->hasItems = conv2.hasItems;
		this->needItems = conv1.needItems;
	}

	template<typename Iter>
	void flushToIter(Iter &iter) {
		ConvertWriteIter<Conv2 &, Iter &, true> outIter(conv2, iter);
		conv1.flushToIter(outIter);
	}

protected:
	Conv1 conv1;
	Conv2 conv2;
	Optional<To> tempStorage;

};

template<typename Converter, typename SrcIter, bool implicitFlush = true>
class ConvertReadChain: public ConvertReadIter<Converter, typename OriginT<SrcIter>::T &> {
public:

	typedef typename OriginT<SrcIter>::T::BaseIter BaseIter;

	ConvertReadChain(typename FastParam<BaseIter>::T iter):ConvertReadIter<Converter, typename OriginT<SrcIter>::T &>(curiter),curiter(iter) {}
	ConvertReadChain(const ConvertReadChain &other):ConvertReadIter<Converter, typename OriginT<SrcIter>::T &>(curiter),curiter(other.curiter) {}

protected:
	SrcIter curiter;

};

template<typename Converter, typename TrgIter, bool implicitFlush=true>
class ConvertWriteChain: public ConvertWriteIter<Converter, typename OriginT<TrgIter>::T &, implicitFlush> {
public:

	typedef typename OriginT<TrgIter>::T::BaseIter BaseIter;

	ConvertWriteChain(typename FastParam<BaseIter>::T iter):ConvertWriteIter<Converter,  typename OriginT<TrgIter>::T &>(curiter),curiter(iter) {}
	ConvertWriteChain(const ConvertWriteChain &other):ConvertWriteIter<Converter,  typename OriginT<TrgIter>::T &>(curiter),curiter(other.curiter) {}

protected:
	TrgIter curiter;

};


template<typename From, typename To, typename Impl>
inline const To& IConverter<From, To, Impl>::getNext() {
	return static_cast<Impl &>(*this).getNext();
}

template<typename From, typename To, typename Impl>
inline const To& IConverter<From, To, Impl>::peek() const {
	return static_cast<const Impl &>(*this).peek();
}

template<typename From, typename To, typename Impl>
inline void IConverter<From, To, Impl>::write(const From& item) {
	return static_cast<Impl &>(*this).write(item);
}

template<typename From, typename To, typename Impl>
template<typename FlatArray, typename Iter>
inline natural IConverter<From, To, Impl>::blockWrite(const FlatArray& array, Iter& target, bool writeAll) {
	return static_cast<Impl &>(*this).blockWrite(array,target,writeAll);
}

template<typename From, typename To, typename Impl>
template<typename FlatArray, typename Iter>
inline natural IConverter<From, To, Impl>::blockRead(Iter& source,FlatArray& array, bool readAll) {
	return static_cast<Impl &>(*this).blockWrite(source,array,readAll);
}

template<typename From, typename To, typename Impl>
template<typename Iter>
inline void IConverter<From, To, Impl>::writeToIter(const From& item,Iter& target) {
	return static_cast<Impl &>(*this).writeToIter(item,target);
}

template<typename From, typename To, typename Impl>
template<typename Iter>
inline const To& IConverter<From, To, Impl>::getNextFromIter(Iter& source) {
	return static_cast<Impl &>(*this).getNextFromIter(source);
}

template<typename From, typename To, typename Impl>
template<typename Iter>
inline bool IConverter<From, To, Impl>::hasItemsFromIter(const Iter& source) const {
	return static_cast<Impl &>(*this).hasItemsFromIter(source);
}

template<typename From, typename To, typename Impl>
template<typename Iter>
inline const To& IConverter<From, To, Impl>::peekFromIter(const Iter& source) const {
	return static_cast<Impl &>(*this).peekFromIter(source);
}

template<typename From, typename To, typename Impl>
inline void IConverter<From, To, Impl>::flush() {
	return static_cast<Impl &>(*this).flush();
}

template<typename From, typename To, typename Impl>
template<typename Iter>
inline void IConverter<From, To, Impl>::flushToIter(Iter &iter) {
	return static_cast<Impl &>(*this).flushToIter(iter);
}

template<typename From, typename To, typename Impl>
inline IConverter<From, To, Impl>::IConverter()
	:hasItems(false),needItems(true),eolb(false)
{
}


template<typename From, typename To, typename Impl>
template<typename FlatArray, typename Iter>
inline natural ConverterBase<From, To, Impl>::blockWrite(
								const FlatArray& array, Iter& target, bool ) {

	while (this->hasItems) {
		target.write(Super::getNext());
		if (!target.hasItems()) return 0;
	}

	const From *src = array.data();
	natural len = array.length();
	for (natural i = 0; i <len; i++) {
		if (this->eolb) return i;
		Super::write(src[i]);
		while (this->hasItems) target.write(Super::getNext());
		if (!target.hasItems()) return i+1;
	}
	return len;

}

template<typename From, typename To, typename Impl>
template<typename FlatArray, typename Iter>
inline natural ConverterBase<From, To, Impl>::blockRead(Iter& source, FlatArray& array, bool ) {
	To *target = array.data();
	natural avail = array.length();
	for (natural i = 0; i < avail; i++) {
		while (!this->hasItems) {
			if (this->eolb) return i;
			Super::write(source.getNext());
			if (!source.hasItems()) {
				Super::flush();
				while (i < avail && this->hasItems) {
					target[i] = Super::getNext();
					i++;
				}
				return i;
			}
		}
		target[i] = Super::getNext();
	}
	return avail;
}

template<typename From, typename To, typename Impl>
template<typename Iter>
inline void ConverterBase<From, To, Impl>::writeToIter(const From& item, Iter& target) {
	while (this->hasItems) {
		target.write(Super::getNext());
	}
	Super::write(item);
	while (this->hasItems) {
		target.write(Super::getNext());
	}
}

template<typename From, typename To, typename Impl>
template<typename Iter>
inline const To& ConverterBase<From, To, Impl>::getNextFromIter(Iter& source) {
	if (this->hasItems) return Super::getNext();
	while (!this->hasItems) {
		if (this->eolb) throwIteratorNoMoreItems(THISLOCATION, typeid(To));
		Super::write(source.getNext());
	}
	return Super::getNext();
}

template<typename From, typename To, typename Impl>
template<typename Iter>
inline const To& ConverterBase<From, To, Impl>::peekFromIter(const Iter& source) const{
	if (this->hasItems) return Super::peek();
	while (!this->hasItems) {
		if (this->eolb) throwIteratorNoMoreItems(THISLOCATION, typeid(To));
		Super::write(source.getNext());
	}
	return Super::peek();
}

template<typename From, typename To, typename Impl>
template<typename Iter>
inline bool ConverterBase<From, To, Impl>::hasItemsFromIter(const Iter& source) const{
	//if convertor has items, return true
	if (this->hasItems) return true;
	//if convertor reports end of logical block, return false;
	if (this->eolb) return false;
	//if source stream has items
	while (source.hasItems()) {
		//we don't know until we will feed convertor with some items
		Impl &impl = const_cast<Impl &>(this->getImpl());
		Iter &iter = const_cast<Iter &>(source);

		impl.write(iter.getNext());
		//if convertor has items, return true
		if (this->hasItems) return true;
		//if convertor reports end of logical block, return false;
		if (this->eolb) return false;
	}
	return false;

}


template<typename From, typename To, typename Impl>
inline void ConverterBase<From, To, Impl>::flush() {
}

template<typename From, typename To, typename Impl>
template<typename Iter>
inline void ConverterBase<From, To, Impl>::flushToIter(Iter &iter) {
	Super::flush();
	while (this->hasItems) {
		iter.write(Super::getNext());
	}
}




}


#endif /* LIGHTSPEED_BASE_ITER_ITERCONV_H_ */

