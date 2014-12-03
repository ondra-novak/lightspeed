/*
 * textOut.h
 *
 *  Created on: 28.12.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_TEXT_TEXTOUT_H_
#define LIGHTSPEED_TEXT_TEXTOUT_H_

#include "textFormat.h"
#include "../iter/iteratorChain.h"
#pragma once

namespace LightSpeed {

	namespace _intr {
		template<typename X>
		class TextOutSink: public SharedResource {
		public:

			template<typename T>
			TextOutSink &operator <<(const T &a) {ref.fmt.operator<<(a);return *this;}

			TextOutSink(X &ref):ref(ref) {}
			~TextOutSink() {if (!isShared()) flush();}
		protected:
			X &ref;
			void flush();
		};


		template<typename Impl>
		class TextOutBase: public Invokable<Impl> {
		public:

			TextOutSink<Impl> operator()(ConstStringT<char> pattern);
			TextOutSink<Impl> operator()(const char *pattern);
		};


	}

	template<typename Iter, typename Alloc = StdAlloc>
	class TextOut: public _intr::TextOutBase<TextOut<Iter,Alloc> >{
		typedef typename OriginT<Iter>::T::ItemT ItemT;
		typedef  ItemT T;
	public:
		typedef Iter IterT;

		typedef _intr::TextOutSink<TextOut<Iter,Alloc> > Sink;

		TextOut(Iter iter):iter(iter) {}
		TextOut(Iter iter, const Alloc &alloc):iter(iter),fmt(alloc) {}



		TextOut &setBase(natural b);
		TextOut &setPrecision(natural p);
		TextOut &setSci(natural p);
		TextOut &setNumberFormat(INumberFormatter *fmt);

		using _intr::TextOutBase<TextOut<Iter,Alloc> >::operator ();
		Sink operator()(ConstStringT<T> pattern);
		Sink operator()(const T *pattern);
		Sink operator()();

		TextOut &setNL(const ConstStringT<T> &nl) {fmt.setNL(nl);return *this;}

		typename OriginT<Iter>::T &nxChain() {return iter;}
		const typename OriginT<Iter>::T &nxChain() const {return iter;}
	protected:
		Iter iter;
		TextFormat<ItemT,Alloc> fmt;

		friend class _intr::TextOutSink<TextOut<Iter,Alloc> >;
		friend class _intr::TextOutBase<TextOut<Iter,Alloc> >;
	};


	


}





  // namespace LightSpeed

#endif /* LIGHTSPEED_TEXT_TEXTOUT_H_ */
