/*
 * classTable.tcc
 *
 *  Created on: 17.11.2010
 *      Author: ondra
 */

#ifndef _LIGHTSPEED_SERIALIZE_CLASSTABLE_TCC_
#define _LIGHTSPEED_SERIALIZE_CLASSTABLE_TCC_

#include "classTable.h"
#include "../exceptions/badcast.h"

namespace LightSpeed {

template<typename Serializer, typename BaseT, typename ClassId, typename AllocFactory>
ClassTable<Serializer,BaseT,ClassId,AllocFactory>::ClassTable() {}

template<typename Serializer, typename BaseT, typename ClassId, typename AllocFactory>
ClassTable<Serializer,BaseT,ClassId,AllocFactory>::ClassTable(const AllocFactory &fact)
	:typesByName(std::less<ClassId>(),fact)
	,typesById(std::less<TypeInfo>(),fact) {}
template<typename Serializer, typename BaseT, typename ClassId, typename AllocFactory>
BaseT *ClassTable<Serializer,BaseT,ClassId,AllocFactory>::load(Serializer & arch) const
{
	ClassId clsId;
	if (arch.getFormatter().openTaggedSection(clsId)) {
		arch.getFormatter().closeTaggedSection(clsId);
		return 0;
	}
	const PClassOperations *regs = typesByName.find(clsId);
	if (regs == 0) throw UnknownObjectIdentifierException(THISLOCATION,clsId.toString());
	arch.getFormatter().openTaggedSection(clsId);
	BaseT *res = (*regs)->load(arch);
	arch.getFormatter().closeTaggedSection(clsId);
	return res;
}

template<typename Serializer, typename BaseT, typename ClassId, typename AllocFactory>
void ClassTable<Serializer,BaseT,ClassId,AllocFactory>::store(Serializer & arch, const BaseT *val) const {

	if (val == 0) {
		arch.getFormatter().openTaggedSection(ClassId());
		arch.getFormatter().closeTaggedSection(ClassId());
	} else {
		TypeInfo nfo = typeid(*val);
		const PClassOperations *regs = typesById.find(nfo);
		if (regs == 0) throw ClassNotRegisteredException(THISLOCATION,nfo);

		arch.getFormatter().openTaggedSection((*regs)->getClassId());
		(*regs)->store(arch,val);
		arch.getFormatter().closeTaggedSection((*regs)->getClassId());
	}
}

template<typename Serializer, typename BaseT, typename ClassId, typename AllocFactory>
void ClassTable<Serializer,BaseT,ClassId,AllocFactory>::destroy(BaseT *val) const {

	if (val != 0) {
		TypeInfo nfo = typeid(*val);
		const PClassOperations *regs = typesById.find(nfo);
		if (regs == 0) throw ClassNotRegisteredException(THISLOCATION,nfo);

		(*regs)->destroy(val);
	}
}

template<typename Serializer, typename BaseT, typename T,
		typename ClassId, typename TFactory>
class ClassOperationsImpl: public ClassOperations<Serializer,BaseT,ClassId>
{
public:

	ClassOperationsImpl(const T &init, const ClassId &id, const TFactory &fact)
		:init(init),id(id),fact(fact) {}
	virtual TypeInfo getType() const {return typeid(T);}
	virtual const ClassId &getClassId() const {return id;}
	virtual BaseT *load(Serializer &arch) const {
		T *x = fact.createInstance(init);
		arch(*x);
		return x;
	}
	virtual void store(Serializer &arch, const BaseT *ptr) const {
		const T *x = dynamic_cast<const T *>(ptr);
		if (x == 0) throw BadCastException(THISLOCATION,typeid(*ptr),typeid(T));
		arch << *x;
	}
	virtual void destroy(BaseT *src) const {
		T *x = dynamic_cast<T *>(src);
		if (x == 0) throw BadCastException(THISLOCATION,typeid(*x),typeid(T));
		fact.destroyInstance(x);
	}

protected:
	T init;
	ClassId id;
	TFactory fact;
};


template<typename Serializer, typename BaseT, typename ClassId, typename AllocFactory>
template<typename T, typename Fact>
ClassTable<Serializer,BaseT, ClassId,AllocFactory> &
	ClassTable<Serializer,BaseT, ClassId,AllocFactory>::add(
				ClassId name, const T &src, const Fact &fact)
{
	typedef typename Fact::template Factory<T> ItemFactory;
	typedef ClassOperationsImpl<Serializer, BaseT, T, ClassId, ItemFactory> Ops;
	typedef PolyAlloc<AllocFactory, Ops> PolyAllocOps;
	AllocFactory factory(typesById.getFactory());
	PClassOperations ops(new(factory) PolyAllocOps(src,name,ItemFactory(fact)));
	typesById.insert(ops->getType(),ops);
	typesByName.insert(ops->getClassId(),ops);
	return *this;

}

template<typename Serializer, typename BaseT, typename ClassId, typename AllocFactory>
bool ClassTable<Serializer,BaseT, ClassId,AllocFactory>::isKnown(ClassId name) const {
	const PClassOperations *regs = typesByName.find(name);
	return regs != 0;

}

template<typename Serializer, typename BaseT, typename ClassId, typename AllocFactory>
bool ClassTable<Serializer,BaseT, ClassId,AllocFactory>::isKnown(const BaseT *ptr) const {
	const PClassOperations *regs = typesById.find(typeid(*ptr));
	return regs != 0;
}

template<typename Serializer, typename BaseT, typename ClassId, typename AllocFactory>
void ClassTable<Serializer,BaseT, ClassId,AllocFactory>::remove(ClassId name) {
	const PClassOperations *regs = typesByName.find(name);
	if (regs == 0) return;
	PClassOperations ops = *regs;
	typesByName.erase(name);
	typesById.erase(ops->getType());
}

template<typename Serializer, typename BaseT, typename ClassId, typename AllocFactory>
void ClassTable<Serializer,BaseT, ClassId,AllocFactory>::remove(const BaseT *ptr) {
	const PClassOperations *regs = typesById.find(typeid(*ptr));
	if (regs == 0) return;
	PClassOperations ops = *regs;
	typesByName.erase(ops->getClassId());
	typesById.erase(ops->getType());
}


}  // namespace LightSpeed

#endif /* _LIGHTSPEED_SERIALIZE_CLASSTABLE_TCC_ */
