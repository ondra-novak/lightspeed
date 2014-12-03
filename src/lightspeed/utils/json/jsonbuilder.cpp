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

Builder::Object Builder::operator ()(ConstStrA name, PNode nd) {
	PNode nd2 = factory->newObject();
	nd2->add(name,nd);
	return Object(factory,nd2);
}

Builder::Object Builder::operator ()(PNode nd) {
	return Object(factory,nd);
}

Builder::Object Builder::operator ()() {
	return Object(factory,factory->newObject());
}

Builder::Object Builder::operator ()(Empty_t ) {
	return Object(factory,factory->newObject());
}

Builder::Array Builder::operator [](PNode nd) {
	return Array(factory,nd);
}

Builder::Array Builder::operator [](Empty_t ) {
	return Array(factory,factory->newArray());
}

Builder::Object Builder::Object::operator ()(ConstStrA name,	PNode nd) {
	(*this)->add(name,nd);
	return *this;
}

Builder::Array Builder::Array::operator [](PNode nd) {
	(*this)->add(nd);
	return *this;
}

Builder::Array LightSpeed::JSON::Builder::array() {
	return Array(factory,factory->newArray());
}

Builder::Object LightSpeed::JSON::Builder::object() {
	return Object(factory,factory->newObject());
}

PNode Builder::operator ()(NullType) {
	return factory->newNullNode();
}

}
}

