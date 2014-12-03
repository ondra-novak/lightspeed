#ifndef LIGHTSPEED_STREAMS_PROXYITERATOR_H_
#define LIGHTSPEED_STREAMS_PROXYITERATOR_H_

namespace LightSpeed {

	template<class SrcIterator, class OutT = typename DeRefType<SrcIterator>::T::ItemT, class Derived>
	class ProxyIteratorT: public IteratorBase<OutT,class Derived> > {
	public:
		
		
		
	};


} // namespace LightSpeed


#endif /*PROXYITERATOR_H_*/

