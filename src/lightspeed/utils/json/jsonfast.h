/*
 *
 *  * OBSOLETE!!!!
 *
 *
 *  jsonfast.h
 *
 *  Created on: 20.5.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_JSONFAST_H_
#define LIGHTSPEED_JSONFAST_H_
#include "../../base/containers/stringpool.h"
#include "json.h"
#include "jsonimpl.h"
#include "../../base/containers/map.h"
#include "jsondefs.h"
#include "jsonserializer.h"


namespace LightSpeed {

class SeqFileOutput;

namespace JSON {


	typedef StringPoolStrRef<wchar_t> StrRefW_t;
	typedef StringPoolStrRef<char> StrRefA_t;

	class IFPool: public RefCntObj {
	public:
		virtual StrRefW_t addString(ConstStrW str) = 0;
		virtual StrRefA_t addString(ConstStrA str) = 0;
		virtual void convertStr(const StrRefW_t &src, StringA &trg) = 0;
		virtual void convertStr(const StrRefA_t &src, StringW &trg, bool skipInvalid) = 0;
		virtual IRuntimeAlloc &getAlloc() = 0;
		virtual PNode getBoolNode(bool v) = 0;
		virtual PNode getNullNode() = 0;
		virtual PNode getEmptyStrNode() = 0;
		virtual ~IFPool() {}
	};
	typedef RefCntPtr<IFPool> PFPool;

	class FRawValueW_t: public AbstractNode_t {
	public:
		FRawValueW_t(PFPool f, StrRefA_t value, bool string)
			:f(f),value(value),nodeType(detectType(value,string))  {}


		virtual NodeType getType() const {return nodeType;}
		virtual ConstStrA getStringUtf8() const {return value;}
		virtual integer getInt() const;
		virtual natural getUInt() const;
		virtual linteger getLongInt() const;
		virtual lnatural getLongUInt() const;
		virtual double getFloat() const;

		virtual bool getBool() const;

		virtual bool isNull() const {
			return nodeType == ndNull;
		}
		virtual INode *getVariable(ConstStrW ) const {return 0;}
		virtual INode *getVariable(ConstStrA ) const {return 0;}
		virtual natural getEntryCount() const {return 0;}
		virtual INode *getEntry(natural ) const {return 0;}
		virtual bool enumEntries(const IEntryEnum &) const {return false;}
		virtual const INode*  enableMTAccess() const {
			RefCntObj::enableMTAccess();
			 f.getMT();
			return this;
		}
		virtual bool operator==(const INode &other) const {
			return other.getType() == nodeType && other.getStringUtf8() == value;
		}
		virtual INode *clone(PFactory factory) const;

		virtual INode &operator[](ConstStrW v) const;
		virtual INode &operator[](ConstStrA v) const;
		virtual INode &operator[](natural index) const;
		virtual ConstStrW getString() const;
		virtual INode *add(PNode newNode);
		virtual INode *add(ConstStrW name, PNode newNode);
		virtual INode *add(ConstStrA name, PNode newNode);
		virtual bool empty() const {return false;}
		virtual INode* erase(ConstStrW name);
		virtual INode* erase(ConstStrA name);
		virtual INode* erase(natural index);

		virtual bool isUtf8() const {return true;}

	protected:
		PFPool f;
		StrRefA_t value;
		mutable String wvalue;
		NodeType nodeType;

		static NodeType detectType(const StrRefA_t &text, bool str);

	};


	typedef Map<StrRefA_t,PNode> FFieldMap_t;
	class FObject_t: public AbstractNode_t {
	public:
		FObject_t(PFPool f):f(f) {}

		void insertField(const StrRefA_t &name, PNode nd);

		virtual NodeType getType() const {return ndObject;}
		virtual ConstStrW getString() const {return ConstStrW();}
		virtual integer getInt() const {return integerNull;}
		virtual linteger getLongInt() const {return lintegerNull;}
		virtual double getFloat() const {return 0;}
		virtual bool getBool() const {return true;}
		virtual bool isNull() const {return false;}
		virtual INode *getVariable(ConstStrW var) const;
		virtual natural getEntryCount() const {return fields.length();}
		virtual INode *getEntry(natural ) const {return 0;}
		virtual bool enumEntries(const IEntryEnum &fn) const;

		virtual bool empty() const {return fields.empty();}
		virtual INode *replace(ConstStrA name, Value newValue, Value *prevValue = 0);
		virtual INode * clear();

		virtual INode *add(PNode nd);
		virtual INode *add(ConstStrW name, PNode nd);


		virtual INode * erase(ConstStrW name);
		virtual INode *erase(natural ) {return this;}

		virtual const INode * enableMTAccess() const;

		virtual INode *clone(PFactory factory) const;

		virtual bool operator==(const INode &other) const;


		virtual natural getUInt() const;
		virtual lnatural getLongUInt() const;
		virtual INode *getVariable(ConstStrA ) const;
		virtual INode &operator[](ConstStrW v) const;
		virtual INode &operator[](ConstStrA v) const;
		virtual INode &operator[](natural index) const;
		virtual ConstStrA getStringUtf8() const;
		virtual INode *add(ConstStrA name, PNode newNode);
		virtual INode* erase(ConstStrA name);

		virtual bool isUtf8() const {return true;}

	protected:
		PFPool f;
		FFieldMap_t fields;
	};


	///obsolete
	template<typename T>
	PNode parseFast(IIterator<char, T> &iter);
	///obsolete
	template<typename T>
	PNode parseFast(IIterator<char, T> &iter, IRuntimeAlloc &factory );
	///obsolete
	template<typename T>
	PNode parseFast(IIterator<char, T> &iter, const PFPool &pool );
	///obsolete
	template<typename T>
	PNode parseFast(const IIterator<char, T> &iter) {
		T cpy = iter;
		return parseFast(cpy);
	}
	///obsolete
	template<typename T>
	PNode parseFast(const IIterator<char, T> &iter, IRuntimeAlloc &factory ) {
		T cpy = iter;
		return parseFast(cpy,factory);
	}
	///obsolete
	template<typename T>
	PNode parseFast(const IIterator<char, T> &iter, const PFPool &pool ) {
		T cpy = iter;
		return parseFast(cpy,pool);
	}


	///Serializes JSON object to any output stream
	/**
	@param json pointer to JSON structure
	@param iter output iterator (must accept characters)
	@param escapeUTF8 set true, if you want to escape UTF8 characters with \uXXXX sequences. 
	  Set false, if you want to store UTF8 characters direct in UTF8 sequence. Use true
	  to generate JSON string which should be accepted on systems with different code page
	*/

	
	PNode fromStream( SeqFileInput &output);


	void toStream(const INode *json, SeqFileOutput &output, bool escapeUTF8);

	StringA toString(const INode *json, bool escapeUTF8);

	template<typename Alloc>
	void toString(const INode *json, AutoArray<char, Alloc> &string, bool escapeUTF8);




}}



#endif /* LIGHTSPEED_JSONFAST_H_ */
