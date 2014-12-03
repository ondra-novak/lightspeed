#pragma once

#include <set>
#include "../containers/list.h"
#include "../containers/arraySet.h"
#include "../memory/ownedPointer.h"
#include "pointers.h"
#include "../typeinfo.h"
#include "sectionId.h"


namespace LightSpeed {



template<typename Serializer, typename BaseT, typename ClassId = StdSectionName>
class ClassOperations: public RefCntObj {
public:
	virtual TypeInfo getType() const = 0;
	virtual const ClassId &getClassId() const = 0;
	virtual BaseT *load(Serializer &arch) const = 0;
	virtual void store(Serializer &arch, const BaseT *ptr) const = 0;
	virtual void destroy(BaseT *src) const = 0;
	virtual ~ClassOperations() {}
};



template<typename Serializ, typename BaseT,
			typename ClassId = StdSectionName,
			typename AllocFactory = StdAlloc >
class ClassTable: public IPointerSerializer<typename Serializ::ThisSerializer, BaseT> {

public:
	typedef typename Serializ::ThisSerializer Serializer;
	typedef ClassOperations<Serializer,BaseT> ClassOpers;
private:

	typedef RefCntPtr<ClassOpers> PClassOperations;

	

	typedef Map<ClassId, PClassOperations, std::less<ClassId>, AllocFactory> TypesByName;
	typedef Map<TypeInfo, PClassOperations, std::less<TypeInfo>, AllocFactory> TypesById;

public:

	ClassTable();
	ClassTable(const AllocFactory &fact);

	template<typename T>
	ClassTable &add(ClassId name, const T &src) {
		return add(name,src,StdFactory());
	}

	template<typename T, typename Fact>
	ClassTable &add(ClassId name, const T &src, const Fact &fact);

	bool isKnown(ClassId name) const;

	bool isKnown(const BaseT *ptr) const;

	void remove(ClassId name);

	void remove(const BaseT *ptr);

	virtual BaseT *load(Serializer &arch) const;

	virtual void store(Serializer &arch, const BaseT *instance) const;

	virtual void destroy(BaseT *instance) const;

	virtual ~ClassTable() {}


protected:


	TypesByName typesByName;
	TypesById typesById;

};
}




