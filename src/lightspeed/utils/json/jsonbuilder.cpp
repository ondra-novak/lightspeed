#include "../../base/memory/refCntPtr.h"
#include "../../base/containers/constStr.h"

#include "jsonbuilder.h"
namespace LightSpeed {namespace JSON {

Builder::Builder() {
	factory = JSON::create();
}

Builder::Builder(PFactory factory) {
	this->factory = factory;
}

Builder::Builder(IRuntimeAlloc& alloc) {
	this->factory = JSON::create(alloc);
}

Builder::Array Builder::array(const Value &nd) const {
	return Array(factory,nd);
}
Builder::Object Builder::object(const Value &nd) const {
	return Object(factory,nd);
}

Builder::CArray Builder::array(const Container &nd) const {
	return CArray(factory,nd);
}
Builder::CObject Builder::object(const Container &nd) const {
	return CObject(factory,nd);
}

Builder::Array Builder::operator [](Empty_t ) const {
	return Array(factory,factory->newArray());
}

Builder::Array Builder::array() const {
	return Array(factory,factory->newArray());
}

Builder::Object Builder::object() const {
	return Object(factory,factory->newObject());
}

PNode Builder::operator ()(NullType) const {
	return factory->newNullNode();
}

Builder::CArray Builder::array(const ConstValue &nd) const {
	Container c(factory->newArray());
	c.load(nd);
	return CArray(factory,c);
}
Builder::CObject Builder::object(const ConstValue &nd) const {
	Container c(factory->newObject());
	c.load(nd);
	return CObject(factory,c);
}



Container Builder::operator ()(const ConstStringT<Container>& val) const {
	return factory->newValue(val);
}

Container Builder::operator ()(const ConstStringT<ConstValue>& val) const {
	return factory->newValue(val);
}

Builder::Object Builder::Object::operator/(ConstStrA pathItem) {
	Value v = (*this)[pathItem];
	if (v == null) {
		v = factory->object();
		(*this)(pathItem,v);
	}
	return Object(factory,v);
}


Builder::CObject Builder::operator ()(ConstStrA name,const ConstValue& value) const {
	CObject obj(factory);
	return obj(name,value);
}

Builder::CObject Builder::operator ()(ConstStrA name,const Container& value) const {
	CObject obj(factory);
	return obj(name,value);
}

Builder::CArray Builder::operator <<(const Container& x) const {
	CArray obj(factory);
	return obj << x;

}

Builder::CArray Builder::operator <<(const ConstValue& x) const {
	CArray obj(factory);
	return obj << x;
}

JSON::Builder::CObject JSON::Builder::container(const ConstValue& value) const {
	return CObject(factory, value->copy(factory,1,true));
}


}

}

