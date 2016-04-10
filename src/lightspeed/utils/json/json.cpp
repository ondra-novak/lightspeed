#include "../../base/containers/string.h"
#include "../../base/containers/map.tcc"
#include "../../base/exceptions/invalidParamException.h"
#include "../../base/text/textFormat.tcc"
#include "../../mt/atomic.h"
#include "../../base/iter/vtiterator.h"
#include "json.h"
#include "jsonimpl.h"
#include "jsonexception.h"
#include "jsonfast.tcc"
#include "jsonparser.tcc"
#include "jsonserializer.tcc"


namespace LightSpeed {

namespace JSON {

const char *strTrue = "true";
const char *strFalse = "false";
const char *strNull = "null";
const char *strDelete = "deleted";

static Value sharedNull, sharedTrue, sharedFalse, sharedZero, sharedDelete, sharedEmptyStr;


INode *Object::getVariable(ConstStrA var) const {

	FieldNode f((Field(var, null) ));

	const FieldNode *x = fields.find(f);
	if (x) return x->data.value; else return 0;
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
	for (FieldMap::Iterator iter = fields.getFwIter();iter.hasItems();) {
		const FieldNode *e = iter.getNext();
		kk = StrKeyA2(e->data.key);
		if (fn(*e->data.value,kk)) return true;
	}
	return false;
}

void Object::insertField(ConstStrA name, PNode nd) {
	Value v = isMTAccessEnabled()?nd.getMT():nd;

	ConstStrA out;
	FieldNode *item = new(name, out) FieldNodeNew(Field(out, nd));
	fields.insert(item);
}

INode *Object::add(PNode nd) {
	if (nd == nil) throwNullPointerException(THISLOCATION);
	if (nd == this) throw InvalidParamException(THISLOCATION,0,"JSON: Cycle detected");
	natural l = fields.size();
	TextFormatBuff<char,StaticAlloc<50> > fmt;
	fmt("unnamed%l") << l;
	return add(StringA(fmt.write()),nd);
}

INode *Object::add(ConstStrA name, PNode nd) {
	if (nd == nil) throwNullPointerException(THISLOCATION);
	if (nd == this) throw InvalidParamException(THISLOCATION,0,"JSON: Cycle detected");
	insertField(name,nd);
	return this;
}

INode *Object::clone(PFactory factory) const {
	PNode r = factory->newClass();
	for (FieldMap::Iterator iter = fields.getFwIter();iter.hasItems();) {
		const FieldNode *e = iter.getNext();
		r->add(e->data.key,e->data.value->clone(factory));
	}
	return r.detach();
}

bool Object::operator==(const INode &other) const {
	const Object *k = dynamic_cast<const Object *>(&other);
	if (k == 0) return false;
	if (fields.size() != k->fields.size()) return false;
	for (FieldMap::Iterator iter = fields.getFwIter(),iter2 =k->fields.getFwIter();
		iter.hasItems();) {
		const FieldNode *e1 = iter.getNext();
		const FieldNode *e2 = iter2.getNext();
		if (e1->data.key != e2->data.key || (e1->data.value) != (e2->data.value)) return false;
	}
	return true;
}

INode *Object::replace(ConstStrA name, Value newValue, Value *prevValue ) {
	bool found;
	FieldNode search(Field(name, null));
	FieldMap::Iterator iter = fields.seek(search,Direction::forward,&found);
	Value prev;
	if (!found) {
		add(name,newValue);
	} else {
		const FieldNode *e = iter.peek();
		prev = e->data.value;
		e->data.value = newValue;
	}
	if (prevValue) *prevValue = prev;
	return this;
}

void Object::releaseNode(FieldNode *x) {
	delete static_cast<FieldNodeNew *>(x);
}

INode *Object::clear() {
	fields.clear(releaseNode);
	return this;
}




INode *Object::enableMTAccess()
{
	RefCntObj::enableMTAccess();
	for (FieldMap::Iterator iter = fields.getFwIter(); iter.hasItems();) {
		const FieldNode *e = iter.getNext();
		e->data.value->enableMTAccess();
	}
	return this;
}

Object::Object() {

}

Object::~Object() {
	clear();
}

void *Object::FieldNodeNew::operator new( size_t objSize, ConstStrA str, ConstStrA &stored) {
	size_t total = objSize + str.length();
	void *r = ::operator new(total);
	char *c = reinterpret_cast<char *>(r) + objSize;
	memcpy(c, str.data(), str.length());
	stored = ConstStrA(c,str.length());
	return r;
}
void Object::FieldNodeNew::operator delete(void *ptr, ConstStrA , ConstStrA &) {
	::operator delete(ptr);
}
void Object::FieldNodeNew::operator delete(void *ptr, size_t ) {
	::operator delete(ptr);
}

bool Object::CompareItems::operator ()(const FieldNode *a, const FieldNode *b) const {
	return a->data.key < b->data.key;
}

INode * Object::erase(ConstStrA name) {
	FieldNode search(Field(name,null));
	FieldNode *out = fields.remove(search);
	if (out != 0) {
		releaseNode(out);
	}
	return this;
}


Array::Array() {}
Array::Array(ConstStringT<Value> v) {
	list.append(v);
}
Array::Array(ConstStringT<INode *> v) {
	list.append(v);
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
	if (nd == nil) throw InvalidParamException(THISLOCATION,0,"Argument has no value");
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
	if (parseSignedNumber(value.getFwIter(),res,10)) return res;
	else return integerNull;
}

linteger TextField::getLongInt() const {
	linteger res;
	if (parseSignedNumber(value.getFwIter(),res,10)) return res;
	else return integerNull;
}

ConstStrA TextField::getStringUtf8() const {
	if (utf!=0) return *utf;
	StringA *k = new StringA(String::getUtf8(value));
	k->getMT();
	if (lockCompareExchangePtr<StringA>(&utf,0,k) != 0) {
		delete k;
	}
	return *utf;
}

ConstStrW TextFieldA::getString() const {
	if (wide!=0) return *wide;
	String *k = new String(ConstStrA(value));
	if (lockCompareExchangePtr<String>(&wide,0,k) != 0) {
		delete k;
	}
	return *wide;
}

INode * TextField::enableMTAccess() {
	RefCntObj::enableMTAccess();
	return this;
}

INode *TextField::clone(PFactory factory) const {
	return factory->newValue(value).detach();
}

bool TextField::operator==(const INode &other) const {
	if (other.getType() == ndString) return other.getString() == getString();
	else return false;
}

TextField::~TextField() {delete utf;}
TextFieldA::~TextFieldA() {delete wide;}


double TextField::getFloat() const {
	TextParser<wchar_t,StaticAlloc<100> > parser;
	if (parser(L"%f1",value)) return parser[1];
	return 0;
}




integer TextFieldA::getInt() const {
	integer res;
	if (parseSignedNumber(value.getFwIter(),res,10)) return res;
	else return integerNull;
}

linteger TextFieldA::getLongInt() const {
	linteger res;
	if (parseSignedNumber(value.getFwIter(),res,10)) return res;
	else return integerNull;
}


INode * TextFieldA::enableMTAccess() {
	RefCntObj::enableMTAccess();
	return this;
}

INode *TextFieldA::clone(PFactory factory) const {
	return factory->newValue(value).detach();
}

bool TextFieldA::operator==(const INode &other) const {
	if (other.getType() == ndString) return other.getStringUtf8() == getStringUtf8();
	else return false;
}

TextFieldA &LeafNodeConvToStr::getTextNode() const {
	if (txtBackend == 0) {
		TextFieldA *z = createTextNode();
		TextFieldA *r = lockCompareExchangePtr<TextFieldA>(&txtBackend,0,z);
		if (r) delete z;
	}
	return *txtBackend;
}

LeafNodeConvToStr::~LeafNodeConvToStr() {
	if (txtBackend != 0) delete txtBackend;
}

double TextFieldA::getFloat() const {
	TextParser<char,StaticAlloc<100> > parser;
	if (parser("%f1",value)) return parser[1];
	return 0;
}

TextFieldA *IntField::createTextNode() const {
	TextFormatBuff<char,StaticAlloc<200> > fmt;
	fmt("%1") << x;
	return new TextFieldA(fmt.write());
}

TextFieldA *IntField64::createTextNode() const {
	TextFormatBuff<char,StaticAlloc<200> > fmt;
	fmt("%1") << x;
	return new TextFieldA(fmt.write());
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

TextFieldA *FloatField::createTextNode() const {
	TextFormatBuff<char,StaticAlloc<200> > fmt;
	fmt("%1") << x;
	return new TextFieldA(fmt.write());
}

INode *FloatField::clone(PFactory factory) const {
	return factory->newValue(x).detach();
}

bool FloatField::operator==(const INode &other) const {
	const FloatField *t = dynamic_cast<const FloatField *>(&other);
	if (t == 0) return false;
	else return x == t->x;
}

INode *Null::clone(PFactory f) const {
	return f->newValue(null);

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



PNode Factory::createObject() {return new Object;}
PNode Factory::createNumber(natural v) {return new IntField(v);}
PNode Factory::createNumber(integer v) {return new IntField(v);}
#ifdef LIGHTSPEED_HAS_LONG_TYPES
PNode Factory::createNumber(lnatural v) {return new IntField64(v);}
PNode Factory::createNumber(linteger v) {return new IntField64(v);}
#endif
PNode Factory::createNumber(float v) {return new FloatField(v);}
PNode Factory::createNumber(double v) {return new FloatField(v);}
PNode Factory::createNumber(bool v) {return new Bool(v);}
PNode Factory::createString(ConstStrW v) {return new TextField(v);}
PNode Factory::createString(ConstStrA v) {return new TextFieldA(v);}
PNode Factory::createArray(ConstStringT<Value> v) {return new Array(v);}
PNode Factory::createArray(ConstStringT<INode *> v) {return new Array(v);}




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
	return parseStream(iter);
}


LightSpeed::JSON::PNode Factory::fromStream( SeqFileInput &stream )
{
	SeqTextInA in(stream);
	return parseStream(in);

}
LightSpeed::JSON::PNode Factory::fromCharStream( IVtIterator<char> &stream )
{
	return parseStream(stream);
			
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
		Iter(const FieldMap &mp):iter(mp.getFwIter()) {}

		virtual bool hasItems() const {return iter.hasItems();}
		virtual bool getNext(NodeInfo &nfo) {
			const FieldNode *e = iter.getNext();
			k = e->data.key;
			nfo.key = &k;
			nfo.node = e->data.value;
			return iter.hasItems();
		}
		virtual bool peek(NodeInfo &nfo) const {
			const FieldNode *e = iter.peek();
			k = e->data.key;
			nfo.key = &k;
			nfo.node = e->data.value;
			return iter.hasItems();
		}
		virtual IIntIter *clone(IRuntimeAlloc &alloc) const {
			return new(alloc) Iter(*this);
		}

	protected:
		FieldMap::Iterator iter;
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


bool PNode::setIfNullDeleteOtherwiseAtomic(INode *nd) {
	nd->enableMTAccess();
	nd->addRef();
	if (lockCompareExchangePtr<INode>(&this->ptr,0,nd) != 0) {
		delete nd;
		return false;
	} else {
		return true;
	}
}


template<typename T>
Value Factory::parseStream( IIterator<char, T> &iter ) {
	Parser<T> parser(iter, this);
	return parser.parse();

}

///Creates JSON object
Value FactoryCommon::newObject() {
	return createObject();
}
///Creates JSON array
Value FactoryCommon::newArray() {
	return createArray(ConstStringT<Value>());

}
///Creates JSON number using unsigned value
Value FactoryCommon::newValue(natural v) {
	if (v == 0) {
		if (sharedZero == nil) sharedZero.setIfNullDeleteOtherwiseAtomic(new ZeroNumber);
		return sharedZero;
	}
	return createNumber(v);
}
///Creates JSON number using signed value
Value FactoryCommon::newValue(integer v) {
	if (v == 0) {
		if (sharedZero == nil) sharedZero.setIfNullDeleteOtherwiseAtomic(new ZeroNumber);
		return sharedZero;
	}
	return createNumber(v);
}
#ifdef LIGHTSPEED_HAS_LONG_TYPES
///Creates JSON number using unsigned value
Value FactoryCommon::newValue(lnatural v) {
	if (v == 0) {
		if (sharedZero == nil) sharedZero.setIfNullDeleteOtherwiseAtomic(new ZeroNumber);
		return sharedZero;
	}
	return createNumber(v);
}
///Creates JSON number using signed value
Value FactoryCommon::newValue(linteger v) {
	if (v == 0) {
		if (sharedZero == nil) sharedZero.setIfNullDeleteOtherwiseAtomic(new ZeroNumber);
		return sharedZero;
	}
	return createNumber(v);
}
#endif
///Creates JSON number using double-float value
Value FactoryCommon::newValue(double v) {
	if (v == 0) {
		if (sharedZero == nil) sharedZero.setIfNullDeleteOtherwiseAtomic(new ZeroNumber);
		return sharedZero;
	}
	return createNumber(v);
}
///Creates JSON bool and stores value
Value FactoryCommon::newValue(bool v) {
	if (v) {
		if (sharedTrue == nil) sharedTrue.setIfNullDeleteOtherwiseAtomic(new Bool(true));
		return sharedTrue;
	} else {
		if (sharedFalse == nil) sharedFalse.setIfNullDeleteOtherwiseAtomic(new Bool(false));
		return sharedFalse;
	}
}
///Creates JSON string
Value FactoryCommon::newValue(ConstStrW v) {
	if (v.empty()) {
		if (sharedEmptyStr == nil) sharedEmptyStr.setIfNullDeleteOtherwiseAtomic(new EmptyString);
		return sharedEmptyStr;
	}
	return createString(v);
}
///Creates JSON string
Value FactoryCommon::newValue(ConstStrA v) {
	if (v.empty()) {
		if (sharedEmptyStr == nil) sharedEmptyStr.setIfNullDeleteOtherwiseAtomic(new EmptyString);
		return sharedEmptyStr;
	}
	return createString(v);

}
///Creates JSON array
Value FactoryCommon::newValue(ConstStringT<JSON::Value> v) {
	return createArray(v);
}
///Creates JSON array
Value FactoryCommon::newValue(ConstStringT<JSON::INode *> v) {
	return createArray(v);
}


Value FactoryCommon::newValue(NullType) {
	if (sharedNull == nil) sharedNull.setIfNullDeleteOtherwiseAtomic(new Null);
	return sharedNull;
}

Value FactoryCommon::newDeleteNode() {
	if (sharedDelete == nil) sharedDelete.setIfNullDeleteOtherwiseAtomic(new Delete);
	return sharedDelete;
}

INode *EmptyString::clone(PFactory f) const {
	return f->newValue(ConstStrA());
}

bool EmptyString::operator==(const INode &other)const {
	return !other.getBool();
}



INode *ZeroNumber::clone(PFactory f)const {
	return f->newValue(natural(0));
}

bool ZeroNumber::operator==(const INode &other)const {
	if (other.getType() == ndInt) return other.getLongInt() == 0;
	else return other.getFloat() == 0;
}


INode *SingleCharacter::clone(PFactory f)const {
	return f->newValue(getStringUtf8());
}

int SingleCharacter::getNum()const {
	if (isdigit(x)) return (x - '0');
	else return 0;
}
bool SingleCharacter::operator==(const INode &other) const {
	if (other.isUtf8()) return ConstStrA(x) == other.getStringUtf8();
	else return ConstStrW(x) == other.getString();
}


} 



}


