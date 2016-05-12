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

Builder::Array Builder::array(PNode nd) const {
	return Array(factory,nd);
}
Builder::Object Builder::object(PNode nd) const {
	return Object(factory,nd);
}

Builder::Array Builder::operator [](Empty_t ) const {
	return Array(factory,factory->newArray());
}

Builder::Array LightSpeed::JSON::Builder::array() const {
	return Array(factory,factory->newArray());
}

Builder::Object LightSpeed::JSON::Builder::object() const {
	return Object(factory,factory->newObject());
}

PNode Builder::operator ()(NullType) const {
	return factory->newNullNode();
}

Builder::Array Builder::array(ConstValue nd, natural depth, bool mt_share) const {
	return Array(factory,nd->copy(factory,depth,mt_share));
}
Builder::Object Builder::object(ConstValue nd, natural depth, bool mt_share) const {
	return Object(factory,nd->copy(factory,depth,mt_share));
}


}
}

