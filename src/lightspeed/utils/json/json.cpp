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



INode *Object_t::getVariable(ConstStrA var) const {
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

bool Object_t::enumEntries(const IEntryEnum &fn) const {
	StrKeyA2 kk;
	for (FieldMap_t::Iterator iter = fields.getFwIter();iter.hasItems();) {
		const FieldMap_t::Entity &e = iter.getNext();
		kk = StrKeyA2(e.key);
		if (fn(*e.value.get(),kk)) return true;
	}
	return false;
}

void Object_t::insertField(const StringA &name, PNode nd) {
	if (isMTAccessEnabled()) fields.insert(name,nd.getMT());
	else fields.insert(name,nd);
}

INode *Object_t::add(PNode nd) {
	if (nd == nil) nd = getNullNode();
	if (nd == this) throw InvalidParamException(THISLOCATION,0,"JSON: Cycle detected");
	natural l = fields.length();
	TextFormatBuff<char,StaticAlloc<50> > fmt;
	fmt("unnamed%l") << l;
	return add(StringA(fmt.write()),nd);
}

INode *Object_t::add(ConstStrA name, PNode nd) {
	if (nd == nil) nd = getNullNode();
	if (nd == this) throw InvalidParamException(THISLOCATION,0,"JSON: Cycle detected");
	insertField(name,nd);
	return this;
}

INode *Object_t::clone(PFactory factory) const {
	PNode r = factory->newClass();
	for (FieldMap_t::Iterator iter = fields.getFwIter();iter.hasItems();) {
		const FieldMap_t::Entity &e = iter.getNext();
		r->add(e.key,e.value->clone(factory));
	}
	return r.detach();
}

bool Object_t::operator==(const INode &other) const {
	const Object_t *k = dynamic_cast<const Object_t *>(&other);
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


INode *Object_t::enableMTAccess()
{
	RefCntObj::enableMTAccess();
	for (FieldMap_t::Iterator iter = fields.getFwIter(); iter.hasItems();) {
		const FieldMap_t::Entity &e = iter.getNext();
		e.key.getMT();
		e.value->enableMTAccess();
	}
	return this;
}

bool Array_t::enumEntries(const IEntryEnum &fn) const {
	natural p = 0;
	for (FieldList_t::Iterator iter = list.getFwIter();iter.hasItems();) {
		const PNode &nd = iter.getNext();
		if (fn(*nd.get(),NumKey(p))) return true;
		++p;
	}
	return false;
}

INode *Array_t::add(PNode nd) {
	if (nd == nil) nd = getNullNode();
	if (nd == this) throw InvalidParamException(THISLOCATION,0,"JSON: Cycle detected");
	if (isMTAccessEnabled()) list.add(nd.getMT());
	else list.add(nd);
	return this;
}

INode *Array_t::add(ConstStrA , PNode nd) {
	return add(nd);
}

INode *Array_t::erase(natural index) {
	list.erase(index);
	return this;
}

void Array_t::internalAdd(PNode nd) {list.add(nd);}

INode *Array_t::clone(PFactory factory) const {
	PNode r = factory->newArray();
	for (FieldList_t::Iterator iter = list.getFwIter();iter.hasItems();) {
		const PNode &e = iter.getNext();
		r->add(e->clone(factory));
	}
	return r.detach();
}

bool Array_t::operator==(const INode &other) const {
	const Array_t *k = dynamic_cast<const Array_t *>(&other);
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



INode * Array_t::enableMTAccess()
{
	RefCntObj::enableMTAccess();
	for (FieldList_t::Iterator iter = list.getFwIter(); iter.hasItems();) {
		const PNode &e = iter.getNext();
		e.getMT();
		e->enableMTAccess();
	}
	return this;

}
template<typename JSONStream_t>
static PNode JSON_parseArray( JSONStream_t & parser, PFactory factory  )
{

	PNode res = factory->newArray();
	Array_t *arr = static_cast<Array_t *>(res.get());
	try {

		PNode x = JSON_parseNode(parser,factory);
		while (x != nil) {
			arr->internalAdd(x);
			if (parser(L" ]%")) {
				return res;
			} else if (!parser(L" ,%")) {
				throw ErrorMessageException(THISLOCATION,"Expecting ','");				}
			x = JSON_parseNode(parser,factory);
		}
		if (parser(L" ]%")) {
			return res;
		}
		throw ErrorMessageException(THISLOCATION,"Syntax error");
	} catch (Exception &e) {
		TextFormatBuff<wchar_t,StaticAlloc<100> > fmt;
		fmt("after index: %1") << arr->getEntryCount();
		throw ParseError_t(THISLOCATION,fmt.write()) << e;
	}
}

integer TextField_t::getInt() const {
	integer res;
	if (parseSignedNumber(x.getFwIter(),res,10)) return res;
	else return integerNull;
}

linteger TextField_t::getLongInt() const {
	linteger res;
	if (parseSignedNumber(x.getFwIter(),res,10)) return res;
	else return integerNull;
}

ConstStrA TextField_t::getStringUtf8() const {
	if (utf!=0) return *utf;
	StringA *k = new StringA(x.getUtf8());
	k->getMT();
	if (lockCompareExchangePtr<StringA>(&utf,0,k) != 0) {
		delete k;
	}
	return *utf;
}

INode * TextField_t::enableMTAccess() {
	RefCntObj::enableMTAccess();
	x = x.getMT();
	return this;
}

INode *TextField_t::clone(PFactory factory) const {
	return factory->newValue(x).detach();
}

bool TextField_t::operator==(const INode &other) const {
	if (other.getType() == ndString) return other.getString() == getString();
	else return false;
}

TextField_t::~TextField_t() {delete utf;}


double TextField_t::getFloat() const {
	TextParser<wchar_t,StaticAlloc<100> > parser;
	if (parser(L"%f1",x)) return parser[1];
	return 0;
}




integer TextFieldA_t::getInt() const {
	integer res;
	if (parseSignedNumber(x.getFwIter(),res,10)) return res;
	else return integerNull;
}

linteger TextFieldA_t::getLongInt() const {
	linteger res;
	if (parseSignedNumber(x.getFwIter(),res,10)) return res;
	else return integerNull;
}

ConstStrW TextFieldA_t::getString() const {
	if (unicode!=0) return *unicode;
	String *k = new String(x);
	k->getMT();
	if (lockCompareExchangePtr<String>(&unicode,0,k) != 0) {
		delete k;
	}
	return *unicode;
}

INode * TextFieldA_t::enableMTAccess() {
	RefCntObj::enableMTAccess();
	x = x.getMT();
	return this;
}

INode *TextFieldA_t::clone(PFactory factory) const {
	return factory->newValue(x).detach();
}

bool TextFieldA_t::operator==(const INode &other) const {
	if (other.getType() == ndString) return other.getStringUtf8() == getStringUtf8();
	else return false;
}

TextFieldA_t::~TextFieldA_t() {delete unicode;}


double TextFieldA_t::getFloat() const {
	TextParser<char,StaticAlloc<100> > parser;
	if (parser("%f1",x)) return parser[1];
	return 0;
}

ConstStrW IntField_t::getString() const {
	if (strx.empty()) {
		TextFormatBuff<wchar_t,StaticAlloc<200> > fmt;
		fmt("%1") << x;
		strx = fmt.write();
	}
	return strx;
}

ConstStrW IntField64_t::getString() const {
	if (strx.empty()) {
		TextFormatBuff<wchar_t,StaticAlloc<200> > fmt;
		fmt("%1") << x;
		strx = fmt.write();
	}
	return strx;
}

INode *IntField_t::clone(PFactory factory) const {
	return factory->newValue(x).detach();
}

INode *IntField64_t::clone(PFactory factory) const {
	return factory->newValue(x).detach();
}

bool IntField_t::operator==(const INode &other) const {
	const IntField_t *t = dynamic_cast<const IntField_t *>(&other);
	if (t == 0) return false;
	else return x == t->x;
}

bool IntField64_t::operator==(const INode &other) const {
	const IntField64_t *t = dynamic_cast<const IntField64_t *>(&other);
	if (t == 0) return false;
	else return x == t->x;
}

ConstStrW FloatField_t::getString() const {
	if (strx.empty()) {
		TextFormatBuff<wchar_t,StaticAlloc<200> > fmt;
		fmt("%1") << x;
		strx = fmt.write();
	}
	return strx;
}

INode *FloatField_t::clone(PFactory factory) const {
	return factory->newValue(x).detach();
}

bool FloatField_t::operator==(const INode &other) const {
	const FloatField_t *t = dynamic_cast<const FloatField_t *>(&other);
	if (t == 0) return false;
	else return x == t->x;
}

INode *Null_t::clone(PFactory ) const {
	return getNullNode().detach();
}

bool Null_t::operator==(const INode &other) const {
	const Null_t *t = dynamic_cast<const Null_t *>(&other);
	if (t == 0) return false;
	else return true;
}

INode *Delete_t::clone(PFactory factory) const {
	return factory->newDeleteNode().detach();
}

bool Delete_t::operator==(const INode &other) const {
	const Delete_t *t = dynamic_cast<const Delete_t *>(&other);
	if (t == 0) return false;
	else return true;
}

ErrorMessageException Delete_t::accessError(const ProgramLocation &loc) const {
	return ErrorMessageException(loc,"Member is deleted");
}

INode *Bool_t::clone(PFactory factory) const {
	return factory->newValue(b).detach();
}

bool Bool_t::operator==(const INode &other) const {
	const Bool_t *t = dynamic_cast<const Bool_t *>(&other);
	if (t == 0) return false;
	else return b == t->b;
}


template<typename JSONStream_t>
PNode JSON_parseNode( JSONStream_t &parser, PFactory factory )
{
	if (parser(L" {%")) {
		return JSON_parseClass(parser,factory);
	} else if (parser(L" [%")) {
		return JSON_parseArray(parser,factory);
/*			} else if (parser(L" \"%(0,)1%%!(1,1)[\\\\]%\"%")) {
		return factory->newValue(decodeString(parser[1].str()));*/
	} else if (parser(L" \"%(0,)[*^\\\\]1\"%") || parser(L" \"%(0,)*[\\\\](1,)[*^\\\\]1\"%")) {
		return factory->newValue(decodeString(parser[1].str()));
	} else if (parser(L" %f1%%")) {
		ConstStrW text = parser[1].str();
		if (text.find('.')!= naturalNull || text.find('E') != naturalNull || text.find('e') != naturalNull)
			return factory->newValue((double)parser[1]);
		else
			return factory->newValue((integer)parser[1]);
	} else if (parser(L" %(1,1)[nN][uU][lL][lL]1%%")) {
		return getNullNode();
	} else if (parser(L" %(1,1)[tT][rR][uU][eE]1%%")) {
		return factory->newValue(true);
	} else if (parser(L" %(1,1)[fF][aA][lL][sS][eE]1%%")) {
		return factory->newValue(false);
	} else
		return nil;
			
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

/*
INode * AbstractNode_t::add( ConstStrA name, PNode newNode )
{
	return add(StringA(name),newNode);
}*/




PNode getNullNode()
{
	static PNode nd = new Null_t;
	return nd.getMT();
}

PNode getDeleteNode()
{
	static PNode nd = new Delete_t;
	return nd.getMT();
}

PNode Factory_t::newClass() {return new Object_t;}
PNode Factory_t::newArray() {return new Array_t;}
PNode Factory_t::newValue(natural v) {return new IntField_t(v);}
PNode Factory_t::newValue(integer v) {return new IntField_t(v);}
#ifdef LIGHTSPEED_HAS_LONG_TYPES
PNode Factory_t::newValue(lnatural v) {return new IntField64_t(v);}
PNode Factory_t::newValue(linteger v) {return new IntField64_t(v);}
#endif
PNode Factory_t::newValue(float v) {return new FloatField_t(v);}
PNode Factory_t::newValue(double v) {return new FloatField_t(v);}
PNode Factory_t::newValue(bool v) {return new Bool_t(v);}
PNode Factory_t::newValue(ConstStrW v) {return new TextField_t(v);}
PNode Factory_t::newValue(ConstStrA v) {return new TextFieldA_t(v);}




class FastFactory_t: public FactoryAlloc_t<> {
public:

	ClusterAlloc alloc;

	FastFactory_t():FactoryAlloc_t<>(alloc) {}
	virtual IFactory *clone() {return new FastFactory_t;}
};



PFactory create()
{
	return new Factory_t;	
}


PFactory create(IRuntimeAlloc &alloc)
{
	return new FactoryAlloc_t<>(alloc);
}

PFactory createFast()
{
		return new FastFactory_t();
}

ConstStrA Factory_t::toString(const INode &nd )
{
	strRes.clear();
	serialize(&nd,strRes,this->escapeUTF);
	return strRes.getArray();
}

void FactoryCommon_t::toStream(const INode &nd, SeqFileOutput &stream) {
	JSON::toStream(&nd,stream,this->escapeUTF);
}





LightSpeed::JSON::PNode Factory_t::fromString( ConstStrA text )
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
					static_cast<const Object_t *>(chgnd),
					static_cast<const Object_t *>(&nd)));
			}
		} else {
			newcls->add(name.getString(),nd.clone(fact));
		}
		return false;
	}

	MergeClassDropOld_t(Object_t *newcls, const Object_t *changes, FactoryCommon_t *fact)
		:newcls(newcls),changes(changes),fact(fact) {}
protected:
	Object_t *newcls;
	const Object_t *changes;
	FactoryCommon_t *fact;
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
	MergeClassAddNew_t(Object_t *newcls, IFactory *fact)
		:newcls(newcls),fact(fact) {}
protected:
	Object_t *newcls;
	IFactory *fact;
};

LightSpeed::JSON::PNode FactoryCommon_t::merge( const INode &base, const INode &change )
{

	const Object_t *clsbase = dynamic_cast<const Object_t *>(&base);
	const Object_t *clschange = dynamic_cast<const Object_t *>(&change);
	if (NULL != clsbase && NULL != clschange) {
		return mergeClasses(clschange, clsbase);
	} else {
		return change.clone(this);
	}
	
}

LightSpeed::JSON::PNode FactoryCommon_t::mergeClasses( const Object_t * clschange, const Object_t * clsbase )
{
	PNode nwnode = this->newClass();
	Object_t *newcls = static_cast<Object_t *>(nwnode.get());
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

Iterator Object_t::getFwIter() const {

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

Iterator Array_t::getFwIter() const {

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

