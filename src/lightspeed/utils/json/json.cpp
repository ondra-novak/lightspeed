#include "../../base/containers/string.h"
#include "../../base/containers/map.tcc"
#include "../../base/exceptions/invalidParamException.h"
#include "../../base/text/textFormat.tcc"
#include "../../mt/atomic.h"
#include "../../base/iter/vtiterator.h"
#include "json.h"
#include "jsonimpl.h"
#include "jsonexception.h"
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


bool Object::enumEntries(const IEntryEnum &fn) const {
	natural index= 0;
	for (FieldMap::Iterator iter = fields.getFwIter();iter.hasItems();) {
		const FieldNode *e = iter.getNext();
		if (fn(e->data.value,e->data.key,index)) return true;
		index++;
	}
	return false;
}

void Object::insertField(ConstStrA name, Value nd) {

	ConstStrA out;
	FieldNode *item = new(name, out) FieldNodeNew(Field(out, nd));
	bool exists;
	fields.insert(item, &exists);
	if (exists) delete item;
}

natural Object::length() const {
	return fields.size();
}


INode *Object::add(Value nd) {
	if (nd == nil) return this;
	if (nd == this) throw InvalidParamException(THISLOCATION,0,"JSON: Cycle detected");
	natural l = fields.size();
	TextFormatBuff<char,StaticAlloc<50> > fmt;
	fmt("unnamed%l") << l;
	return add(StringA(fmt.write()),nd);
}

INode *Object::add(ConstStrA name, Value nd) {
	if (nd == nil) return erase(name);
	if (nd == this) throw InvalidParamException(THISLOCATION,0,"JSON: Cycle detected");
	insertField(name,nd);
	return this;
}

Value Object::clone(PFactory factory) const {
	Value r = factory->newClass();
	for (FieldMap::Iterator iter = fields.getFwIter();iter.hasItems();) {
		const FieldNode *e = iter.getNext();
		r->add(e->data.key,e->data.value->clone(factory));
	}
	return r;
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
		if (newValue == null)
			erase(name);
		else
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




const INode *Object::enableMTAccess() const
{
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
		const Value &nd = iter.getNext();
		if (fn(nd,ConstStrA(),p)) return true;
		++p;
	}
	return false;
}

INode *Array::add(Value nd) {
	if (nd == nil) return this;
	if (nd == this) throw InvalidParamException(THISLOCATION,0,"JSON: Cycle detected");
	list.add(nd);
	return this;
}

INode *Array::add(ConstStrA , Value nd) {
	return add(nd);
}

INode *Array::erase(natural index) {
	list.erase(index);
	return this;
}

void Array::internalAdd(Value nd) {list.add(nd);}

Value Array::clone(PFactory factory) const {
	Value r = factory->newArray();
	for (FieldList_t::Iterator iter = list.getFwIter();iter.hasItems();) {
		const Value &e = iter.getNext();
		r->add(e->clone(factory));
	}
	return r;
}

bool Array::operator==(const INode &other) const {
	const Array *k = dynamic_cast<const Array *>(&other);
	if (k == 0) return false;
	if (list.length() != k->list.length()) return false;
	for (FieldList_t::Iterator iter = list.getFwIter(),iter2 =k->list.getFwIter();
		iter.hasItems();) {
			const Value &e1 = iter.getNext();
			const Value &e2 = iter2.getNext();
			if (*e1 != *e2) return false;
	}
	return true;
}



const INode * Array::enableMTAccess() const
{
	for (FieldList_t::Iterator iter = list.getFwIter(); iter.hasItems();) {
		const Value &e = iter.getNext();
		e.getMT();
		e->enableMTAccess();
	}
	return this;
}

INode *Array::replace(natural index, Value newValue, Value *prevValue) {
	Value prev = list[index];
	if (newValue == null) list.erase(index);
	else list(index) = newValue;
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

const INode * TextField::enableMTAccess() const {
	return this;
}

Value TextField::clone(PFactory factory) const {
	return factory->newValue(value);
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


const INode * TextFieldA::enableMTAccess() const {
	return this;
}

Value TextFieldA::clone(PFactory factory) const {
	return factory->newValue(value);
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

Value IntField::clone(PFactory factory) const {
	return factory->newValue(x);
}

Value IntField64::clone(PFactory factory) const {
	return factory->newValue(x);
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

Value FloatField::clone(PFactory factory) const {
	return factory->newValue(x);
}

bool FloatField::operator==(const INode &other) const {
	const FloatField *t = dynamic_cast<const FloatField *>(&other);
	if (t == 0) return false;
	else return x == t->x;
}

Value Null::clone(PFactory f) const {
	return f->newValue(null);

}

bool Null::operator==(const INode &other) const {
	const Null *t = dynamic_cast<const Null *>(&other);
	if (t == 0) return false;
	else return true;
}


Value Bool::clone(PFactory factory) const {
	return factory->newValue(b);
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

void SerializeError_t::message( ExceptionMsg &msg ) const
{
	msg("JSON serializer error at: %1") << nearStr;
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



Value Factory::createObject() {return new Object;}
Value Factory::createNumber(natural v) {return new IntField(v);}
Value Factory::createNumber(integer v) {return new IntField(v);}
#ifdef LIGHTSPEED_HAS_LONG_TYPES
PNode Factory::createNumber(lnatural v) {return new IntField64(v);}
PNode Factory::createNumber(linteger v) {return new IntField64(v);}
#endif
Value Factory::createNumber(float v) {return new FloatField(v);}
Value Factory::createNumber(double v) {return new FloatField(v);}
Value Factory::createNumber(bool v) {return new Bool(v);}
Value Factory::createString(ConstStrW v) {return new TextField(v);}
Value Factory::createString(ConstStrA v) {return new TextFieldA(v);}
Value Factory::createArray(ConstStringT<Value> v) {return new Array(v);}
Value Factory::createArray(ConstStringT<INode *> v) {return new Array(v);}




class FastFactory: public FactoryAlloc<> {
public:

	ClusterAlloc alloc;

	FastFactory():FactoryAlloc<>(alloc) {}
	virtual IFactory *clone() {return new FastFactory;}
	~FastFactory() {

	}
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

template<typename T> void doSerialize(const INode &nd, IWriteIterator<char, T> &iter) {
	Serializer<T> s(iter,true);
	s.serialize(&nd);

}

void FactoryCommon::toStream(const INode &nd, SeqFileOutput &stream) {
	SeqTextOutA textStream(stream);
	doSerialize(nd,textStream);
}





Value Factory::fromString( ConstStrA text )
{
	ConstStrA::Iterator iter = text.getFwIter();
	return parseStream(iter);
}


Value Factory::fromStream( SeqFileInput &stream )
{
	SeqTextInA in(stream);
	return parseStream(in);

}
Value Factory::fromCharStream( IVtIterator<char> &stream )
{
	return parseStream(stream);
			
}

Value PFactory::array()
{
	return (*this)->array();
}


Value  PFactory::object()
{
	return (*this)->object();
}



Value Value::operator[]( ConstStrA name ) const
{
	return Value(safeGet()->getPtr(name));
}


Value Value::operator[]( const char *name ) const
{
	return Value(safeGet()->getPtr(ConstStrA(name)));
}

Value Value::operator[]( int pos ) const
{
	if (pos < 0 || pos >= (int)(*this)->getEntryCount()) {
		throwRangeException_To<natural>(THISLOCATION,(*this)->getEntryCount(),pos);
	}
	return Value(safeGet()->getPtr(pos));
}

Value Value::operator[]( natural pos ) const
{
	if (pos >= (*this)->getEntryCount()) {
		throwRangeException_To(THISLOCATION,(*this)->getEntryCount(),pos);
	}
	return Value(safeGet()->getPtr(pos));
}


bool ConstValue::setIfNullDeleteOtherwiseAtomic(const INode *nd) {
	nd->addRef();
	if (lockCompareExchangePtr<const INode>(&this->ptr,0,nd) != 0) {
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


Value EmptyString::clone(PFactory f) const {
	return f->newValue(ConstStrA());
}

bool EmptyString::operator==(const INode &other)const {
	return !other.getBool();
}



Value ZeroNumber::clone(PFactory f) const {
	return f->newValue(natural(0));
}

bool ZeroNumber::operator==(const INode &other)const {
	if (other.getType() == ndInt) return other.getLongInt() == 0;
	else return other.getFloat() == 0;
}


Value SingleCharacter::clone(PFactory f) const {
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


ConstValue ConstValue::operator [](ConstStrA name) const {
	return ConstValue(safeGet()->getPtr(name));
}

ConstValue ConstValue::operator [](const char* name) const {
	return ConstValue(safeGet()->getPtr(name));
}

ConstValue ConstValue::operator [](int i) const {
	return ConstValue(safeGet()->getPtr(i));
}

ConstValue ConstValue::operator [](natural i) const {
	return ConstValue(safeGet()->getPtr(i));
}

bool ConstValue::isNull() const {
	return safeGet()->isNull();
}

bool ConstValue::getBool() const {
	return safeGet()->getBool();
}

ConstStrA ConstValue::getStringA() const {
	return safeGet()->getStringUtf8();
}

ConstStrW ConstValue::getString() const {
	return safeGet()->getString();
}

integer ConstValue::getInt() const {
	return safeGet()->getInt();
}

natural ConstValue::getUInt() const {
	return safeGet()->getUInt();
}

linteger ConstValue::getLongInt() const {
	return safeGet()->getLongInt();
}

lnatural ConstValue::getLongUInt() const {
	return safeGet()->getLongUInt();
}

double ConstValue::getNumber() const {
	return safeGet()->getFloat();
}

ConstValue::Type ConstValue::getType() const {
	return safeGet()->getType();
}

bool ConstValue::getOrDefault(bool defVal) const {
	return ptr?ptr->getBool():defVal;
}

ConstStrA ConstValue::getOrDefault(ConstStrA defVal) const {
	return ptr?ptr->getStringUtf8():defVal;
}

ConstStrW ConstValue::getOrDefault(ConstStrW defVal) const {
	return ptr?ptr->getString():defVal;
}

integer ConstValue::getOrDefault(integer defVal) const {
	return ptr?ptr->getInt():defVal;

}

natural ConstValue::getOrDefault(natural defVal) const {
	return ptr?ptr->getUInt():defVal;
}

double ConstValue::getOrDefault(double defVal) const {
	return ptr?ptr->getFloat():defVal;
}

float ConstValue::getOrDefault(float defVal) const {
	return ptr?(float)ptr->getFloat():defVal;
}

ConstValue ConstValue::getOrDefault(const ConstValue& value) const {
	return ptr?ConstValue(ptr):value;

}

natural ConstValue::length() const {
	return safeGet()->length();
}


bool ConstValue::empty() const {
	return safeGet()->empty();
}

static StringCore<ConstKeyValue>enumConst(const ConstValue &object) {
	StringCore<ConstKeyValue> res;
	StringCore<ConstKeyValue>::WriteIterator iter = res.createBufferIter(object.length());
	class Filler: public IEntryEnum {
	public:
		Filler(StringCore<ConstKeyValue>::WriteIterator &iter):iter(iter) {}
		virtual bool operator()(const INode *nd, ConstStrA key, natural index) const {
			iter.write(ConstKeyValue(index,key,ConstValue(nd)));
			return false;
		}
		StringCore<ConstKeyValue>::WriteIterator &iter;
	};
	Filler f(iter);
	object->enumEntries(f);
	return res;
}

static StringCore<KeyValue>enumMutable(const Value &object) {
	StringCore<KeyValue> res;
	StringCore<KeyValue>::WriteIterator iter = res.createBufferIter(object.length());
	class Filler: public IEntryEnum {
	public:
		Filler(StringCore<KeyValue>::WriteIterator &iter):iter(iter) {}
		virtual bool operator()(const INode *nd, ConstStrA key, natural index) const {
			iter.write(KeyValue(index,key,Value(const_cast<INode *>(nd))));
			return false;
		}
		StringCore<KeyValue>::WriteIterator &iter;
	};
	Filler f(iter);
	object->enumEntries(f);
	return res;
}

ConstIterator::ConstIterator(const ConstValue& object):Super(enumConst(object)) {
}

Iterator::Iterator(const Value& object):Super(enumMutable(object)) {
}

Value AbstractNode_t::copy(PFactory factory, natural depth, bool mt_share) const {
	if (getType() == ndObject) {
		Value out = factory->object();
		if (depth) {
			for(ConstIterator iter = getFwConstIter(); iter.hasItems();) {
				const ConstKeyValue &ckv = iter.getNext();
				Value x = ckv->copy(factory,depth-1,mt_share);
				out->add(ckv.getStringKey(),x);
			}
		} else {
			for(ConstIterator iter = getFwConstIter(); iter.hasItems();) {
				const ConstKeyValue &ckv = iter.getNext();
				Value x (const_cast<INode *>((const INode *)ckv));
				out->add(ckv.getStringKey(),x);
			}
		}
		return out;
	} else if (getType() == ndArray) {
		Value out = factory->array();
		if (depth) {
			for(ConstIterator iter = getFwConstIter(); iter.hasItems();) {
				const ConstKeyValue &ckv = iter.getNext();
				Value x = ckv->copy(factory,depth-1,mt_share);
				out->add(x);
			}
		} else {
			for(ConstIterator iter = getFwConstIter(); iter.hasItems();) {
				const ConstKeyValue &ckv = iter.getNext();
				Value x (const_cast<INode *>((const INode *)ckv));
				if (mt_share) x->enableMTAccess();
				out->add(x);
			}
		}
		return out;
	} else  {
		if (mt_share) enableMTAccess();
		return Value(const_cast<INode *>(static_cast<const INode *>(this)));
	}
}


Container& Container::set(ConstStrA name, const ConstValue& value) {
	const_cast<INode *>(safeGet())->replace(name,static_cast<const Value &>(value));
	return *this;
}

Container& Container::add(ConstStrA name, const ConstValue& value) {
	const_cast<INode *>(safeGet())->add(name,static_cast<const Value &>(value));
	return *this;
}

Container& Container::set(natural index, const ConstValue& value) {
	const_cast<INode *>(safeGet())->replace(index,static_cast<const Value &>(value));
	return *this;
}

Container& Container::add(const ConstValue& value) {
	const_cast<INode *>(safeGet())->add(static_cast<const Value &>(value));
	return *this;
}

Container& Container::unset(ConstStrA name) {
	const_cast<INode *>(safeGet())->erase(name);
	return *this;
}

Container& Container::erase(natural index) {
	const_cast<INode *>(safeGet())->erase(index);
	return *this;
}

Container& Container::load(const ConstValue& from) {
	class Loader: public IEntryEnum {
	public:
		Container &c;

		virtual bool operator()(const INode *nd, ConstStrA key, natural) const  {
			c.set(key, ConstValue(nd));
			return false;
		}
		Loader(Container &c):c(c) {}
	};
	class ArrayLoader: public IEntryEnum {
	public:
		Container &c;

		virtual bool operator()(const INode *nd, ConstStrA, natural) const  {
			c.add(ConstValue(nd));
			return false;
		}
		ArrayLoader(Container &c):c(c) {}
	};
	if ((*this)->isObject()) {
		Loader l(*this);
		from->enumEntries(l);
		return *this;
	} else {
		ArrayLoader l(*this);
		from->enumEntries(l);
		return *this;

	}
}

const INode* Container::checkIsolation(const INode* ptr) {
	if (ptr->isShared())
		throw SharedValueException(THISLOCATION);
	return ptr;
}

const char *SharedValueException::msgText = "Cannot make object mutable, it is shared.";
void SharedValueException::message(ExceptionMsg &msg) const {
	msg(msgText);
}

Value& Value::set(ConstStrA name, const Value& value) {
	safeGetMutable()->replace(name,value);return *this;
}

Value& Value::add(ConstStrA name, const Value& value) {
	safeGetMutable()->add(name,value);return *this;
}

Value& Value::set(natural index, const Value& value) {
	safeGetMutable()->replace(index,value);return *this;
}

Value& Value::add(const Value& value) {
	safeGetMutable()->add(value);return *this;
}

Value& Value::unset(ConstStrA name) {
	Container::unset(name);return *this;
}

Value& Value::erase(natural index) {
	Container::erase(index);return *this;
}

Value& Value::load(const ConstValue& from) {
	Container::load(from);return *this;
}

Container& Container::clear() {
	const_cast<INode *>(safeGet())->clear();
	return *this;
}

Value& Value::clear() {
	Container::clear();
	return *this;
}

const Path Path::root(Path::root,"<root>");

ConstValue Path::operator()(const ConstValue& root) const {
	if (isRoot()) return root;
	ConstValue v = parent.operator()(root);
	if (v != null)
		return isIndex()?v[getIndex()]:v[getKey()];
	else
		return null;
}

Value Path::operator()(const Value& root) const {
	if (isRoot()) return root;
	Value v = parent.operator()(root);
	if (v != null)
		return isIndex()?v[getIndex()]:v[getKey()];
	else
		return null;

}

Path *Path::copy() const {
	return Path::copy(StdAlloc::getInstance());
}

Path *Path::copy(IRuntimeAlloc &alloc) const {
	//do not perform copy of root element
	if (isRoot()) return const_cast<Path *>(this);
	//calculate required size - first include pointer to allocator
	natural reqSize = sizeof(IRuntimeAlloc *);
	//calculate length
	natural len=0;
	for (const Path *p = this; p != &root; p = &p->parent) {
		//reserve space for element
		reqSize +=sizeof(Path);
		//if it is key, then reserve space for name
		if (p->isKey()) reqSize+=p->index;
		//increase count of elemenets
		len++;
	}
	IRuntimeAlloc *owner;
	//allocate memory, return owner of memory
	void *buff = alloc.alloc(reqSize,owner);
	IRuntimeAlloc **a = reinterpret_cast<IRuntimeAlloc **>(buff);
	//store allocator
	*a = owner;
	//get pointer to first item
	Path *start = reinterpret_cast<Path *>(a+1);
	//get pointer to string buffer
	char *strbuff = reinterpret_cast<char *>(start+len);
	//perform recursive copy
	return copyRecurse(start,strbuff);
}

Path *Path::copyRecurse(Path * trg, char  *strBuff) const {
	//stop on root
	if (isRoot()) return const_cast<Path *>(this);
	//if it is key
	if (isKey()) {
		//pick first unused byte in string buffer
		char *c = strBuff;
		//copy name to buffer
		memcpy(strBuff, keyName,index);
		//advence pointer in buffer
		strBuff+=index;
		//copy rest of path to retrieve new pointer
		Path *out = copyRecurse(trg+1,strBuff);
		//construct path item at give position, use returned pointer as parent
		//and use text from string buffer as name
		return new((void *)trg) Path(*out, ConstStrA(c, index));
	} else {
		//if it is index, just construct rest of path
		Path *out = copyRecurse(trg+1,strBuff);
		//copy rest of path to retrieve new pointer

		return new((void *)trg) Path(*out, index);
	}
}

void Path::operator delete(void *ptr) {
	//delete operator is used instead of destructor
	//because object is POD type, it has no destructor
	//destructor is called when need to delete object
	//because object is always allocated by copy()
	//we can nout perform cleanup of whole path

	//do nothing with root
	if (ptr == &root) return;

	//retrieve allocator
	IRuntimeAlloc **a = reinterpret_cast<IRuntimeAlloc **>(ptr);
	IRuntimeAlloc *alc = *(a-1);

	//calculate reqSize
	natural reqSize = sizeof(IRuntimeAlloc *);

	for (const Path *p = reinterpret_cast<Path *>(ptr); ptr != &root; p = &p->parent) {
		//reserve space for element
		reqSize+=sizeof(Path);
		//if it is key, then reserve space for name
		if (p->isKey()) reqSize+=p->getKey().length();
	}
	//deallocate memory
	alc->dealloc(a,reqSize);

}
void *Path::operator new(size_t , void *p) {
	return p;
}
void Path::operator delete (void *, void *) {

}

CompareResult Path::compare(const Path &other) const {
	if (isRoot()) return other.isRoot()?cmpResultEqual:cmpResultLess;
	if (other.isRoot()) return cmpResultGreater;
	CompareResult z = parent.compare(other.parent);
	if (z == cmpResultEqual) {
		if (isIndex()) {
			if (other.isIndex()) {
				if (getIndex() <  other.getIndex()) return cmpResultLess;
				else if (getIndex() > other.getIndex()) return cmpResultGreater;
				else return cmpResultEqual;
			} else {
				return cmpResultLess;
			}
		} else if (other.isKey()) {
			return getKey().compare(other.getKey());
		} else {
			return cmpResultGreater;
		}
	} else {
		return z;
	}
}


Value getConstant(Constant c) {
	switch (c) {
	case constFalse: return Factory().newValue(false); break;
	case constTrue: return Factory().newValue(true); break;
	case constNull: return Factory().newValue(null); break;
	case constZero: return Factory().newValue(0); break;
	case constEmptyStr: return Factory().newValue(ConstStrA()); break;
	}
	throw; //should never reached
}

StringA toString(const INode* json, bool escapeUTF8) {
	AutoArrayStream<char, SmallAlloc<1024> > buff;
	serialize(json,buff,escapeUTF8);
	return StringA(buff.getArray());
}


}

}
