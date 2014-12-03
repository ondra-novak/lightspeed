/*
 * ivararg.tcc
 *
 *  Created on: 12.9.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_ACTIONS_IVARARG_TCC_
#define LIGHTSPEED_BASE_ACTIONS_IVARARG_TCC_

#include "ivararg.h"
#include "vararg.h"

namespace LightSpeed {

template<typename V, typename N, bool flat>
class VarArgImpl: public IVarArg, public MIf<flat,
		typename VarArg<V,N>::Flat,
		typename VarArg<V,N>::Ref>::T {
public:


	typedef typename MIf<flat,
		typename VarArg<V,N>::Flat,
		typename VarArg<V,N>::Ref>::T Super;

	template<typename A,typename B>
	VarArgImpl(const VarArg<A,B> &v):Super(v) {}

	virtual IVarArg *clone( IRuntimeAlloc &alloc = StdAlloc()) const {
		IRuntimeAlloc::AllocLevel l = alloc.getAllocLevel();
		return new(alloc) ICloneable::ClonedObject<VarArgImpl<V,N,false> >(*this);
	}

	virtual IVarArg *copy(IRuntimeAlloc &alloc = StdAlloc()) const {
		IRuntimeAlloc::AllocLevel l = alloc.getAllocLevel();
		return new(alloc) ICloneable::ClonedObject<VarArgImpl<V,N,true> >(*this);
	}

protected:

	class ForEachFunctor {
	public:
		ForEachFunctor(const IVarArgEnum &fn):fn(fn) {}
		template<typename T>
		bool operator()(const T &v) const {
			return fn(ArgInfo(&v,sizeof(T),typeid(T)));
		}
	protected:
		const IVarArgEnum &fn;
	};

	class GetArgFunctor {
	public:
		GetArgFunctor(ArgInfo &nfo,natural &idx):nfo(nfo),idx(idx) {}
		template<typename T>
		bool operator()(const T &v) const {
			if (idx == 0) {
				nfo = (ArgInfo(&v,sizeof(T),typeid(T)));
				return true;
			}
			idx --;
			return false;
		}

	protected:
		ArgInfo &nfo;
		natural &idx;

	};

	class ThrowArgFunctor {
	public:
		ThrowArgFunctor(natural &idx):idx(idx) {}
		template<typename T>
		bool operator()(const T &v) const {
			if (idx == 0) {
				throw &v;
			}
			idx --;
			return false;
		}

	protected:
		natural &idx;

	};
public:
	virtual ArgInfo getArg(natural index) const {
		ArgInfo arg(0,0,TypeInfo());
		if (static_cast<const Super *>(this)->forEach(GetArgFunctor(arg,index))) {
			return arg;
		} else {
			throwRangeException_To(THISLOCATION,VarArg<V,N>::count-1,index);
			return arg;
		}
	}

	virtual void throwArg(natural index) const {
		Super::forEach(ThrowArgFunctor(index));
		throwRangeException_To(THISLOCATION,VarArg<V,N>::count-1,index);
	}

	virtual bool forEach(const IVarArgEnum &fn) const {
		return Super::forEach(ForEachFunctor(fn));
	}


	virtual natural length() const {
		return VarArg<V,N>::count;
	}

};

template<typename V, typename N>
VarArgImpl<V,N> VarArg<V,N>::ivararg() const {return VarArgImpl<V,N>(*this);}


inline VarArgImpl<void,void> VarArg<void,void>::ivararg() const {return VarArgImpl<void,void>(*this);}




}  // namespace LightSpeed

#endif /* LIGHTSPEED_BASE_ACTIONS_IVARARG_TCC_ */
