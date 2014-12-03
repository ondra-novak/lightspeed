/*
 * generator.tcc
 *
 *  Created on: 8.10.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_ITER_GENERATOR_TCC_
#define LIGHTSPEED_ITER_GENERATOR_TCC_

#include "generator.h"
#include "../exceptions/badcast.h"
#include "../actions/message.h"

namespace LightSpeed {


template<typename T> GeneratorIterator<T>::GeneratorIterator(
			const IFiberFunction &fn, natural stackSize)
	:curStream(0)
{
	this->start(fn,stackSize);
}

template<typename T>
class GeneratoIterator_Run: public IFiberFunction {
public:
	typedef GeneratoIterator_Run ICloneableBase;
	typedef typename GeneratorBackend<T>::GeneratorFunction GeneratorFunction;
	LIGHTSPEED_CLONEABLECLASS;

	void operator()() const {
		GeneratorWriter<T> wr(bk);
		fn(wr);
	}

	GeneratoIterator_Run(GeneratorBackend<T> &bk,const GeneratorFunction &fn):bk(bk),fn(fn) {}

protected:
	GeneratorBackend<T> &bk;
	const GeneratorFunction &fn;
};


template<typename T> GeneratorIterator<T>::GeneratorIterator(
			const GeneratorFunction &fn, natural stackSize)
	:curStream(0)
{

	this->start(GeneratoIterator_Run<T>(*this,fn),stackSize);
}

template<typename T> GeneratorIterator<T>::~GeneratorIterator() {
	this->needItems = false;
	while (this->checkState()) this->wakeUp();
}


template<typename T> bool GeneratorIterator<T>::hasItems() const
{
	return (curStream && curStream->hasItems()) ||
			const_cast<GeneratorIterator<T> *>(this)->prepareNext();
}



template<typename T> const T & GeneratorIterator<T>::getNext()
{
	if (curStream == 0 || !curStream->hasItems() || !prepareNext())
		throwIteratorNoMoreItems(THISLOCATION,typeid(T));
	const T &res = curStream->getNext();
	return res;
}


template<typename T> const T & GeneratorIterator<T>::peek()
{
	if (curStream == 0 || !curStream->hasItems() || !prepareNext())
		throwIteratorNoMoreItems(THISLOCATION,typeid(T));
	const T &res = curStream->peek();
	return res;
}


template<typename T> bool GeneratorIterator<T>::prepareNext()  {
	if (!this->checkState()) return false;
	curStream = 0;
	this->wakeUp();
	return curStream != 0;
}

template<typename T> GeneratorWriter<T>::GeneratorWriter():otherSide(findOtherSide()) {

}
template<typename T> GeneratorBackend<T> &GeneratorWriter<T>::findOtherSide() {
	Fiber &cur = Fiber::current();
	GeneratorBackend<T> *iter = dynamic_cast<GeneratorBackend<T> *>(&cur);
	if (iter == 0)
			throw BadCastException(THISLOCATION, typeid(cur),typeid(GeneratorBackend<T>));
	return *iter;
}

template<typename T> bool GeneratorWriter<T>::hasItems() const {
	return otherSide.needItems;
}

template<typename T> void GeneratorWriter<T>::write(const T &val) {
	ConstStringT<T> stream(&val,1);
	typename ConstStringT<T>::Iterator iter = stream.getFwIter();
	otherSide.pushValue(iter);
}

template<typename T> void GeneratorIterator<T>::pushValue(ValueIterator &val) {
	if (!this->needItems) throwWriteIteratorNoSpace(THISLOCATION,typeid(T));
	curStream = &val;
	this->sleep();
}
template<typename T>
template<class Traits>
natural GeneratorWriter<T>::blockWrite(const FlatArray<typename ConstObject<T>::Remove,Traits> &buffer, bool ) {
	ConstStringT<T> stream(buffer.data(),buffer.length());
	typename ConstStringT<T>::Iterator iter = stream.getFwIter();
	otherSide.pushValue(iter);
	natural processed = iter.getRemain() - buffer.length();
	return processed;
}

template<typename T>
template<class Traits>
natural GeneratorWriter<T>::blockWrite(const FlatArray<typename ConstObject<T>::Add,Traits> &buffer, bool ) {
	ConstStringT<T> stream(buffer.data(),buffer.length());
	typename ConstStringT<T>::Iterator iter = stream.getFwIter();
	otherSide.pushValue(iter);
	natural processed = iter.getRemain() - buffer.length();
	return processed;
}


}  // namespace LightSpeed

#endif /* LIGHTSPEED_ITER_GENERATOR_TCC_ */
