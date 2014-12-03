/*
 * textOut.tcc
 *
 *  Created on: 28.12.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_TEXT_TEXTOUT_TCC_
#define LIGHTSPEED_TEXT_TEXTOUT_TCC_

#pragma once

#include "textOut.h"
#include "textFormat.tcc"

namespace LightSpeed {

template<typename Impl>
_intr::TextOutSink<Impl> _intr::TextOutBase<Impl>::operator ()(ConstStringT<char> pattern)
{
	this->_invoke().fmt.operator()(pattern);return this->_invoke();
}



template<typename Impl>
_intr::TextOutSink<Impl>  _intr::TextOutBase<Impl>::operator ()(const char *pattern)
{
	this->_invoke().fmt.operator()(pattern);return this->_invoke();
}

template<typename X>
void _intr::TextOutSink<X>::flush() {
	if (!std::uncaught_exception()) {
		ref.fmt.output(ref.iter);
	}
}


template<typename Iter, typename Alloc> TextOut<Iter,Alloc> & TextOut<Iter,Alloc>::setBase(natural b)
{
	fmt.setBase(b); return *this;
}



template<typename Iter, typename Alloc> TextOut<Iter,Alloc> & TextOut<Iter,Alloc>::setPrecision(natural p)
{
	fmt.setPrecision(p);return *this;
}



template<typename Iter, typename Alloc> TextOut<Iter,Alloc> & TextOut<Iter,Alloc>::setSci(natural p)
{
	fmt.setSci(p);return *this;
}







template<typename Iter, typename Alloc> TextOut<Iter,Alloc> & TextOut<Iter,Alloc>::setNumberFormat(INumberFormatter *fmt)
{
	this->fmt.setNumberFormat(fmt);return *this;
}



template<typename Iter, typename Alloc>
typename TextOut<Iter,Alloc>::Sink TextOut<Iter,Alloc>::operator ()(ConstStringT<T> pattern)
{
	this->fmt.operator()(pattern);return Sink(*this);
}



template<typename Iter, typename Alloc>
typename TextOut<Iter,Alloc>::Sink TextOut<Iter,Alloc>::operator ()(const T *pattern)
{
	this->fmt.operator ()(pattern);return Sink(*this);
}



template<typename Iter, typename Alloc>
typename TextOut<Iter,Alloc>::Sink TextOut<Iter,Alloc>::operator ()()
{
	this->fmt.operator ()();
	return Sink(*this);
}




}  // namespace LightSpeed

#endif /* LIGHTSPEED_TEXT_TEXTOUT_TCC_ */
