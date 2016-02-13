#include "../../base/text/textIn.tcc"
#include "../../base/streams/utf.h"
#include "../../base/containers/constStr.h"
#include "../../base/memory/smallAlloc.h"
#include "../../base/containers/map.h"
#include "../../base/text/textFormat.tcc"
#include "json.h"
#include "jsonexception.h"
#include "../../base/memory/clusterAllocFactory.h"
#include "../../base/streams/fileio.h"
#include "../../base/text/textstream.h"
#include "../../base/iter/limiter.h"
#include "../../base/text/textOut.tcc"
#include "../../mt/atomic.h"
#include "jsondefs.h"
#include "../../base/containers/stringparam.h"
#include "jsonimpl.h"
#include "jsonfast.tcc"
#include "../../base/text/textParser.tcc"
#include "../../base/exceptions/throws.h"
#include "../../base/exceptions/invalidParamException.h"

namespace LightSpeed {

namespace JSON {



INode *Object::getVariable(ConstStrA var) const {
	const PNode *x = fields.find(var);
	if (x) return *x; else return 0;
}

class StrKeyA2: public IKey {
public:
	StrKeyA2() {}
	StrKeyA2(ConstStrA w):str(w) {}


	virtual Type getType() const {return string;}
	virtual ConstStrA getString() const {return str;}
	virtual natural getIndex() const {
		natural res = 0;
		parseUnsignedNumber(str.getFwIter(),res,10);
		return res;
	}


protected:
	ConstStrA str;
};

bool Object::enumEntries(const IEntryEnum &fn) const {
	StrKeyA2 kk;
	for (FieldMap_t::Iterator iter = fields.getFwIter();iter.hasItems();) {
		const FieldMap_t::Entity &e = iter.getNext();
		kk = StrKeyA2(e.key);
		if (fn(*e.value.get(),kk)) return true;
	}
	return false;
}

void Object::insertField(const StringA &name, PNode nd) {
	if (isMTAccessEnabled()) fields.insert(name,nd.getMT());
	else fields.insert(name,nd);
}

INode *Object::add(PNode nd) {
	if (nd == nil) nd = getNullNode();
	if (nd == this) throw InvalidParamException(THISLOCATION,0,"JSON: Cycle detected");
	natural l = fields.length();
	TextFormatBuff<char,StaticAlloc<50> > fmt;
	fmt("unnamed%l") << l;
	return add(StringA(fmt.write()),nd);
}

INode *Object::add(ConstStrA name, PNode nd) {
	if (nd == nil) nd = getNullNode();
	if (nd == this) throw InvalidParamException(THISLOCATION,0,"JSON: Cycle detected");
	insertField(name,nd);
	return this;
}

INode *Object::clone(PFactory factory) const {
	PNode r = factory->newClass();
	for (FieldMap_t::Iterator iter = fields.getFwIter();iter.hasItems();) {
		const FieldMap_t::Entity &e = iter.getNext();
		r->add(e.key,e.value->clone(factory));
	}
	return r.detach();
}

bool Object::operator==(const INode &other) const {
	const Object *k = dynamic_cast<const Object *>(&other);
	if (k == 0) return false;
	if (fields.length() != k->fields.length()) return false;
	for (FieldMap_t::Iterator iter = fields.getFwIter(),iter2 =k->fields.getFwIter();
		iter.hasItems();) {
		const FieldMap_t::Entity &e1 = iter.getNext();
		const FieldMap_t::Entity &e2 = iter2.getNext();
		if (e1.key != e2.key || (*e1.value) != (*e2.value)) return false;
	}
	return true;
}

INode *Object::replace(ConstStrA name, Value newValue, Value *prevValue ) {
	bool found;
	FieldMap_t::Iterator iter = fields.seek(name,&found);
	Value prev;
	if (!found) {
		add(name,newValue);
	} else {
		const FieldMap_t::Entity &e = iter.peek();
		prev = e.value;
		e.value = newValue;
	}
	if (prevValue) *prevValue = prev;
	return this;
}
INode *Object::clear() {fields.clear();return this;}


INode *Object::enableMTAccess()
{
	RefCntObj::enableMTAccess();
	for (FieldMap_t::Iterator iter = fields.getFwIter(); iter.hasItems();) {
		const FieldMap_t::Entity &e = iter.getNext();
		e.key.getMT();
		e.value->enableMTAccess();
	}
	return this;
}

bool Array::enumEntries(const IEntryEnum &fn) const {
	natural p = 0;
	for (FieldList_t::Iterator iter = list.getFwIter();iter.hasItems();) {
		const PNode &nd = iter.getNext();
		if (fn(*nd.get(),NumKey(p))) return true;
		++p;
	}
	return false;
}

INode *Array::add(PNode nd) {
	if (nd == nil) nd = getNullNode();
	if (nd == this) throw InvalidParamException(THISLOCATION,0,"JSON: Cycle detected");
	if (isMTAccessEnabled()) list.add(nd.getMT());
	else list.add(nd);
	return this;
}

INode *Array::add(ConstStrA , PNode nd) {
	return add(nd);
}

INode *Array::erase(natural index) {
	list.erase(index);
	return this;
}

void Array::internalAdd(PNode nd) {list.add(nd);}

INode *Array::clone(PFactory factory) const {
	PNode r = factory->newArray();
	for (FieldList_t::Iterator iter = list.getFwIter();iter.hasItems();) {
		const PNode &e = iter.getNext();
		r->add(e->clone(factory));
	}
	return r.detach();
}

bool Array::operator==(const INode &other) const {
	const Array *k = dynamic_cast<const Array *>(&other);
	if (k == 0) return false;
	if (list.length() != k->list.length()) return false;
	for (FieldList_t::Iterator iter = list.getFwIter(),iter2 =k->list.getFwIter();
		iter.hasItems();) {
			const PNode &e1 = iter.getNext();
			const PNode &e2 = iter2.getNext();
			if (*e1 != *e2) return false;
	}
	return true;
}



INode * Array::enableMTAccess()
{
	RefCntObj::enableMTAccess();
	for (FieldList_t::Iterator iter = list.getFwIter(); iter.hasItems();) {
		const PNode &e = iter.getNext();
		e.getMT();
		e->enableMTAccess();
	}
	return this;
}

INode *Array::replace(natural index, Value newValue, Value *prevValue) {
	Value prev = list[index];
	list(index) = newValue;
	if (prevValue) *prevValue = prev;
	return this;
}
INode *Array::clear() {
	list.clear();
	return this;
}


integer TextField::getInt() const {
	integer res;
	if (parseSignedNumber(x.getFwIter(),res,10)) return res;
	else return integerNull;
}

linteger TextField::getLongInt() const {
	linteger res;
	if (parseSignedNumber(x.getFwIter(),res,10)) return res;
	else return integerNull;
}

ConstStrA TextField::getStringUtf8() const {
	if (utf!=0) return *utf;
	StringA *k = new StringA(x.getUtf8());
	k->getMT();
	if (lockCompareExchangePtr<StringA>(&utf,0,k) != 0) {
		delete k;
	}
	return *utf;
}

INode * TextField::enableMTAccess() {
	RefCntObj::enableMTAccess();
	x = x.getMT();
	return this;
}

INode *TextField::clone(PFactory factory) const {
	return factory->newValue(x).detach();
}

bool TextField::operator==(const INode &other) const {
	if (other.getType() == ndString) return other.getString() == getString();
	else return false;
}

TextField::~TextField() {delete utf;}


double TextField::getFloat() const {
	TextParser<wchar_t,StaticAlloc<100> > parser;
	if (parser(L"%f1",x)) return parser[1];
	return 0;
}




integer TextFieldA::getInt() const {
	integer res;
	if (parseSignedNumber(x.getFwIter(),res,10)) return res;
	else return integerNull;
}

linteger TextFieldA::getLongInt() const {
	linteger res;
	if (parseSignedNumber(x.getFwIter(),res,10)) return res;
	else return integerNull;
}

ConstStrW AbstractTextFieldA::getString() const {
	if (unicode!=0) return *unicode;
	String *k = new String(getStringUtf8());
	k->getMT();
	if (lockCompareExchangePtr<String>(&unicode,0,k) != 0) {
		delete k;
	}
	return *unicode;
}

INode * TextFieldA::enableMTAccess() {
	RefCntObj::enableMTAccess();
	x = x.getMT();
	return this;
}

INode *TextFieldA::clone(PFactory factory) const {
	return factory->newValue(x).detach();
}

bool TextFieldA::operator==(const INode &other) const {
	if (other.getType() == ndString) return other.getStringUtf8() == getStringUtf8();
	else return false;
}

AbstractTextFieldA::~AbstractTextFieldA() { delete unicode; }


double TextFieldA::getFloat() const {
	TextParser<char,StaticAlloc<100> > parser;
	if (parser("%f1",x)) return parser[1];
	return 0;
}

ConstStrA IntField::getStringUtf8() const {
	if (strx.empty()) {
		TextFormatBuff<char,StaticAlloc<200> > fmt;
		fmt("%1") << x;
		strx = fmt.write();
	}
	return strx;
}

ConstStrA IntField64::getStringUtf8() const {
	if (strx.empty()) {
		TextFormatBuff<char,StaticAlloc<200> > fmt;
		fmt("%1") << x;
		strx = fmt.write();
	}
	return strx;
}

INode *IntField::clone(PFactory factory) const {
	return factory->newValue(x).detach();
}

INode *IntField64::clone(PFactory factory) const {
	return factory->newValue(x).detach();
}

bool IntField::operator==(const INode &other) const {
	const IntField *t = dynamic_cast<const IntField *>(&other);
	if (t == 0) return false;
	else return x == t->x;
}

bool IntField64::operator==(const INode &other) const {
	const IntField64 *t = dynamic_cast<const IntField64 *>(&other);
	if (t == 0) return false;
	else return x == t->x;
}

ConstStrA FloatField::getStringUtf8() const {
	if (strx.empty()) {
		TextFormatBuff<char,StaticAlloc<200> > fmt;
		fmt("%1") << x;
		strx = fmt.write();
	}
	return strx;
}

INode *FloatField::clone(PFactory factory) const {
	return factory->newValue(x).detach();
}

bool FloatField::operator==(const INode &other) const {
	const FloatField *t = dynamic_cast<const FloatField *>(&other);
	if (t == 0) return false;
	else return x == t->x;
}

INode *Null::clone(PFactory ) const {
	return getNullNode().detach();
}

bool Null::operator==(const INode &other) const {
	const Null *t = dynamic_cast<const Null *>(&other);
	if (t == 0) return false;
	else return true;
}

INode *Delete::clone(PFactory factory) const {
	return factory->newDeleteNode().detach();
}

bool Delete::operator==(const INode &other) const {
	const Delete *t = dynamic_cast<const Delete *>(&other);
	if (t == 0) return false;
	else return true;
}

ErrorMessageException Delete::accessError(const ProgramLocation &loc) const {
	return ErrorMessageException(loc,"Member is deleted");
}

INode *Bool::clone(PFactory factory) const {
	return factory->newValue(b).detach();
}

bool Bool::operator==(const INode &other) const {
	const Bool *t = dynamic_cast<const Bool *>(&other);
	if (t == 0) return false;
	else return b == t->b;
}


void ParseError_t::message( ExceptionMsg &msg ) const
{
	msg("JSON parser error at: %1") << nearStr;
}


void RequiredFieldException::message( ExceptionMsg &msg ) const
{
	msg("Required field: %1") << fieldName;
}

INode & AbstractNode_t::operator[]( ConstStrA v ) const
{
	INode *k = getVariable(v);
	if (k == 0) throw RequiredFieldException(THISLOCATION,v);
	return *k;
}

INode & AbstractNode_t::operator[]( natural index ) const
{
	INode *k = getEntry(index);
	if (k == 0) throw RequiredFieldException(THISLOCATION,"Element is not array");
	return *k;
}



ConstStrA AbstractNode_t::getStringUtf8() const
{
	return ConstStrA();
}

ConstStrW AbstractNode_t::getString() const
{
	return ConstStrW();
}

/*
INode * AbstractNode_t::add( ConstStrA name, PNode newNode )
{
	return add(StringA(name),newNode);
}*/




PNode getNullNode()
{
	static PNode nd = new Null;
	return nd.getMT();
}

PNode getDeleteNode()
{
	static PNode nd = new Delete;
	return nd.getMT();
}

PNode Factory::newClass() {return new Object;}
PNode Factory::newArray() {return new Array;}
PNode Factory::newValue(natural v) {return new IntField(v);}
PNode Factory::newValue(integer v) {return new IntField(v);}
#ifdef LIGHTSPEED_HAS_LONG_TYPES
PNode Factory::newValue(lnatural v) {return new IntField64(v);}
PNode Factory::newValue(linteger v) {return new IntField64(v);}
#endif
PNode Factory::newValue(float v) {return new FloatField(v);}
PNode Factory::newValue(double v) {return new FloatField(v);}
PNode Factory::newValue(bool v) {return new Bool(v);}
PNode Factory::newValue(ConstStrW v) {return new TextField(v);}
PNode Factory::newValue(ConstStrA v) {return new TextFieldA(v);}




class FastFactory: public FactoryAlloc<> {
public:

	ClusterAlloc alloc;

	FastFactory():FactoryAlloc<>(alloc) {}
	virtual IFactory *clone() {return new FastFactory;}
};



PFactory create()
{
	return new Factory;	
}


PFactory create(IRuntimeAlloc &alloc)
{
	return new FactoryAlloc<>(alloc);
}

PFactory createFast()
{
		return new FastFactory();
}

ConstStrA Factory::toString(const INode &nd )
{
	strRes.clear();
	serialize(&nd,strRes,this->escapeUTF);
	return strRes.getArray();
}

void FactoryCommon::toStream(const INode &nd, SeqFileOutput &stream) {
	JSON::toStream(&nd,stream,this->escapeUTF);
}





LightSpeed::JSON::PNode Factory::fromString( ConstStrA text )
{
	ConstStrA::Iterator iter = text.getFwIter();
	return parseFast(iter, *getAllocator());
}


LightSpeed::JSON::PNode IFactory::fromStream( SeqFileInput &stream )
{
	SeqTextInA in(stream);
	return parseFast(in,*getAllocator());
			
}
String decodeString( ConstStrW jsontext ) 
{
	AutoArrayStream<wchar_t> buffer;
	ConstStrW::Iterator rd(jsontext.getFwIter());
	buffer.reserve(jsontext.length());
	while (rd.hasItems()) {
		wchar_t x = rd.getNext();
		if (x == '\\') {
			wchar_t y = rd.getNext();
			switch (y) {
				case 'n': buffer.write('\n');break;
				case 'r': buffer.write('\r');break;
				case 'b': buffer.write('\b');break;
				case 't': buffer.write('\t');break;
				case '\\':buffer.write('\\');break;
				case '/': buffer.write('/');break;
				case 'u': {
					natural hexChar = 0;
					IteratorLimiter<ConstStrW::Iterator &> lmrd(rd,4);
					parseUnsignedNumber(lmrd,hexChar,16);
					buffer.write((wchar_t)hexChar);
							}break;
				default: buffer.write(x);
							buffer.write(y);
							break;
			}
		} else {
			buffer.write(x);
		}
	}
	return String(buffer.getArray());
}

/* creates new JSON class, merges subclasses and
   skips items presented in changes JSON class, because
   it expects that they will be added by MergeClassAddNew_t
   */
class MergeClassDropOld_t: public IEntryEnum {
public:
	virtual bool operator()(const INode &nd, const IKey &name) const {
		const INode *chgnd = changes->getVariable(name.getString());
		if (chgnd) {
			if (chgnd->getType() == ndObject && nd.getType() == ndObject) {
				newcls->add(name.getString(),fact->mergeClasses(
					static_cast<const Object *>(chgnd),
					static_cast<const Object *>(&nd)));
			}
		} else {
			newcls->add(name.getString(),nd.clone(fact));
		}
		return false;
	}

	MergeClassDropOld_t(Object *newcls, const Object *changes, FactoryCommon *fact)
		:newcls(newcls),changes(changes),fact(fact) {}
protected:
	Object *newcls;
	const Object *changes;
	FactoryCommon *fact;
};

/* Adds new items into class, not removing already existing */
class MergeClassAddNew_t: public IEntryEnum {
public:
	virtual bool operator()(const INode &nd, const IKey &name) const {
		NodeType ndt = nd.getType();
		if (newcls->getVariable(name.getString()) == 0 && ndt != ndDelete) {
			newcls->add(name.getString(),nd.clone(fact));
		}
		return false;
	}
	MergeClassAddNew_t(Object *newcls, IFactory *fact)
		:newcls(newcls),fact(fact) {}
protected:
	Object *newcls;
	IFactory *fact;
};

LightSpeed::JSON::PNode FactoryCommon::merge( const INode &base, const INode &change )
{

	const Object *clsbase = dynamic_cast<const Object *>(&base);
	const Object *clschange = dynamic_cast<const Object *>(&change);
	if (NULL != clsbase && NULL != clschange) {
		return mergeClasses(clschange, clsbase);
	} else {
		return change.clone(this);
	}
	
}

LightSpeed::JSON::PNode FactoryCommon::mergeClasses( const Object * clschange, const Object * clsbase )
{
	PNode nwnode = this->newClass();
	Object *newcls = static_cast<Object *>(nwnode.get());
	MergeClassDropOld_t mgd(newcls,clschange, this);
	MergeClassAddNew_t mga(newcls,this);
	clsbase->enumEntries(mgd);
	clschange->enumEntries(mga);
	return nwnode;
}



Iterator::~Iterator() {
	if (iter) iter->~IIntIter();
}

Iterator::Iterator(const IIntIter* internalIter) {
	AllocInBuffer b(buffer,sizeof(buffer));
	iter = internalIter->clone(b);
	next = iter->hasItems();
}

const NodeInfo& Iterator::getNext() {
	next = iter->getNext(tmp);
	return tmp;
}

const NodeInfo& Iterator::peek() const {
	next = iter->peek(tmp);
	return tmp;
}

Iterator::Iterator(const Iterator& other)
	:iter(0),next(false)
{
	if (other.iter) {
		AllocInBuffer b(buffer,sizeof(buffer));
		iter = other.iter->clone(b);
		next = iter->hasItems();
	}

}

bool Iterator::hasItems() const {
	return next;
}

Iterator Object::getFwIter() const {

	class Iter: public Iterator::IIntIter, public DynObject {
	public:
		Iter(const FieldMap_t &mp):iter(mp.getFwIter()) {}

		virtual bool hasItems() const {return iter.hasItems();}
		virtual bool getNext(NodeInfo &nfo) {
			const FieldMap_t::Entity &e = iter.getNext();
			k = e.key;
			nfo.key = &k;
			nfo.node = e.value;
			return iter.hasItems();
		}
		virtual bool peek(NodeInfo &nfo) const {
			const FieldMap_t::Entity &e = iter.peek();
			k = e.key;
			nfo.key = &k;
			nfo.node = e.value;
			return iter.hasItems();
		}
		virtual IIntIter *clone(IRuntimeAlloc &alloc) const {
			return new(alloc) Iter(*this);
		}

	protected:
		FieldMap_t::Iterator iter;
		mutable StrKeyA2 k;
	};

	Iter x(fields);
	return Iterator(&x);

}

Iterator Array::getFwIter() const {

	class Iter: public Iterator::IIntIter, public DynObject {
	public:
		Iter(const FieldList_t &mp):iter(mp.getFwIter()),index(0) {}

		virtual bool hasItems() const {return iter.hasItems();}
		virtual bool getNext(NodeInfo &nfo) {
			k = index++;
			PNode nd = iter.getNext();
			nfo.key = &k;
			nfo.node = nd;
			return iter.hasItems();
		}
		virtual bool peek(NodeInfo &nfo) const {
			k = index;
			PNode nd = iter.peek();
			nfo.key = &k;
			nfo.node = nd;
			return iter.hasItems();
		}
		virtual IIntIter *clone(IRuntimeAlloc &alloc) const {
			return new(alloc) Iter(*this);
		}

	protected:
		FieldList_t::Iterator iter;
		natural index;
		mutable NumKey k;
	};

	Iter x(list);
	return Iterator(&x);

}


const INode& Iterator::getNextKC(ConstStrA fieldName) {
	if (isNextKey(fieldName)) {
		return *getNext().node;
	} else {
		throw RequiredFieldException(THISLOCATION,fieldName);
	}
}

bool Iterator::isNextKey(ConstStrA fieldName) const {
	return hasItems() && peek().key->getString() == fieldName;
}

PNode PFactory::array()
{
	return (*this)->array();
}


PNode  PFactory::object()
{
	return (*this)->object();
}



LightSpeed::JSON::PNode PNode::operator[]( ConstStrA name ) const
{
	PNode ret = (*this)->getVariable(name);
	if (ret == nil) throw JSON::RequiredFieldException(THISLOCATION, name);
	return ret;
}


LightSpeed::JSON::PNode PNode::operator[]( const char *name ) const
{
	PNode ret = (*this)->getVariable(name);
	if (ret == nil) throw JSON::RequiredFieldException(THISLOCATION, name);
	return ret;
}

LightSpeed::JSON::PNode PNode::operator[]( int pos ) const
{
	if (pos < 0 || pos >= (int)(*this)->getEntryCount()) {
		throwRangeException_To<natural>(THISLOCATION,(*this)->getEntryCount(),pos);
	}
	PNode ret = (*this)->getEntry(pos);
	return ret;
}

LightSpeed::JSON::PNode PNode::operator[]( natural pos ) const
{
	if (pos >= (*this)->getEntryCount()) {
		throwRangeException_To(THISLOCATION,(*this)->getEntryCount(),pos);
	}
	PNode ret = (*this)->getEntry(pos);
	return ret;
}


} 







}

