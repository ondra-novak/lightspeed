/*
 * configParser.cpp
 *
 *  Created on: 18.10.2013
 *      Author: ondra
 */
#include "configParser.tcc"
#include "../base/text/textstream.h"
#include "../base/containers/linkedList.h"
#include "../base/streams/fileiobuff.tcc"
#include "../base/framework/app.h"
#include "FilePath.tcc"
#include "../base/containers/map.tcc"

namespace LightSpeed {

IniConfigT::Str IniConfigT::addPool(ConstStrA text) {
		const Str *x = allstr.find(text);
		if (x) return *x;
		else {
			Str r = strpool(text);
			allstr.insert(r);
			return r;
		}

    }


ConstStrA::Iterator IniConfigT::errorIncludeCB(ConstStrA) {
		return ConstStrA().getFwIter();
}


const IniConfigT::Str *IniConfigT::getValue(ConstStrA section, const ConstStrA field, natural index ) const {

    	FieldMap::ListIter v = fields.find(SectionField(section,field));
    	while (v.hasItems()) {
    		if (index == 0) return &v.getNext();
    		index--;
    		v.skip();
    	}
    	return 0;

}

void IniConfigT::RequiredFieldException::message(ExceptionMsg &msg) const {
	msg("Config parameter required: [%1]::%2(%3)") << section << field << index;
}

IniConfigT::Iterator::Iterator(const FieldMap &container, ConstStrA section, ConstStrA prefix)
    		:innerIter(container.seek(SectionField(section,ConstStrA()))) {
    		if (innerIter.hasItems()) {
    			const FieldMap::Entity &e = innerIter.peek();
    			if (e.key.section == section) {
    				this->section = e.key.section;
    				if (e.key.field.head(prefix.length()) == prefix) {
    					prefix = e.key.field.head(prefix.length());
    				}
    			}
    		}
    	}

bool IniConfigT::Iterator::match(const FieldMap::Entity &e)const  {
    		return e.key.section == section
    				&& (prefix.empty() || e.key.field.head(prefix.length()) == prefix);
    	}

bool IniConfigT::Iterator::hasItems() const {
    		return innerIter.hasItems() && match(innerIter.peek());
}

const IniConfigT::FieldVal &IniConfigT::Iterator::peek() const {
    		const FieldMap::Entity &e = innerIter.peek();
    		storedItem.field = e.key.field;
    		storedItem.val = e.value;
    		return storedItem;
    	}

const IniConfigT::FieldVal &IniConfigT::Iterator::getNext() {
    		const FieldMap::Entity &e = innerIter.getNext();
    		storedItem.field = e.key.field;
    		storedItem.val = e.value;
    		return storedItem;
    	}


IniConfigT::Iterator IniConfigT::enumFields(ConstStrA section, ConstStrA prefix) const {
    	return Iterator(fields,section,prefix);
    }



IniConfigT::Section &IniConfigT::Section::operator=(const Section &other) {
			section = other.section;
			prefix = other.prefix;
			return *this;
		}

const IniConfigT::Str *IniConfigT::Section::getValue(const ConstStrA &field, natural index) const {
			if (prefix.empty()) {
				return owner.getValue(section,field,index);
			} else {
				StringA fieldName(prefix+field);
				return owner.getValue(section,fieldName,index);
			}
		}

IniConfigT::Section IniConfigT::Section::openSection(const ConstStrA &name) const{
			return Section(owner,StringA(section+ConstStrA(".")+name),ConstStrA());
		}

IniConfigT::Section IniConfigT::Section::openPrefix(const ConstStrA &pfx) const{
			return Section(owner,section,StringA(prefix+pfx));
		}

bool IniConfigT::Section::get(StringCore<char> &val, const ConstStrA &var, natural index ) const{
			const Str *r = getValue(var,index);
			if (r) val = *r;
			return r != 0;
		}
bool IniConfigT::Section::get(ConstStrA &val, const ConstStrA &var, natural index ) const{
	const Str *r = getValue(var,index);
	if (r) val = *r;
	return r != 0;
}
bool IniConfigT::Section::get(String &val, const ConstStrA &var, natural index) const {
			const Str *r = getValue(var,index);
			if (r) val = String(ConstStringT<char>(*r));
			return r != 0;
		}
bool IniConfigT::Section::get(natural &val, const ConstStrA &var, natural index) const {try {
			const Str *r = getValue(var,index);
			if (r) {
				parser(" %1 ",ConstStrA(*r));
				val = parser[1];
			}
			return r != 0;
		} catch (...) {
			Exception::rethrow(THISLOCATION);throw;
		}}

bool IniConfigT::Section::get(integer &val, const ConstStrA &var, natural index) const {try {
			const Str *r = getValue(var,index);
			if (r) {
				parser(" %1 ",*r);
				val = parser[1];
			}
			return r != 0;
		} catch (...) {
			Exception::rethrow(THISLOCATION);throw;
		}}

bool IniConfigT::Section::get(float &val, const ConstStrA &var, natural index) const {try {
			const Str *r = getValue(var,index);
			if (r) {
				parser(" %1 ",*r);
				val = parser[1];
			}
			return r != 0;
		}  catch (...) {
			Exception::rethrow(THISLOCATION);throw;
		}}

bool IniConfigT::Section::get(double &val, const ConstStrA &var, natural index ) const {try{
			const Str *r = getValue(var,index);
			if (r) {
				parser(" %1 ",*r);
				val = parser[1];
			}
			return r != 0;
		} catch (...) {
			Exception::rethrow(THISLOCATION);throw;
		}}

bool IniConfigT::Section::get(bool &val, const ConstStrA &var, natural index) const {try {
			const Str *r = getValue(var,index);
			if (r) {
				if (*r == ConstStrA("true") || *r == ConstStrA("yes") || *r == ConstStrA("on"))
					val = true;
				else if (*r == ConstStrA("false") || *r == ConstStrA("no") || *r == ConstStrA("off"))
					val = false;
				else {
					natural v;
					parser(" %1 ",ConstStrA(*r));
					v = parser[1];
					val = v == 0?false:true;
				}
			}
			return r != 0;
		} catch (...) {
			Exception::rethrow(THISLOCATION);throw;
		}}


IniConfigT::Iterator IniConfigT::Section::getFwIter() const {
			return owner.enumFields(section,prefix);
		}


class IniIncludeLoader {
public:

	explicit IniIncludeLoader(natural openFlags):openFlags(openFlags) {}

	LinkedList<FilePath> includeStack;

	class Stream: public SeqTextInA, public SharedResource {
	public:
		Stream(const SeqFileInput &in, IniIncludeLoader &owner)
			:SeqTextInA(in),owner(owner) {}
		~Stream() {
			if (!isShared())
				owner.closeInclude();
		}

		IniIncludeLoader &owner;
	};

	Stream openInclude(ConstStrA path) {
		String wpath = path;
		return openInclude(wpath);
	}
	Stream openInclude(String wpath) {
		if (includeStack.empty()) {
			FilePath base(AppBase::current().getAppPathname());
			FilePath p = base / wpath;
			includeStack.insertFirst(p);
			wpath = p;
		} else {
			const FilePath &base = includeStack.getFirst();
			FilePath p = base / wpath;
			includeStack.insertFirst(p);
			wpath = p;
		}
		try {
			SeqFileInput input(wpath,openFlags);
			PInputStream bufferedInput = new IOBuffer<>(input.getStream());
			return Stream(SeqFileInput(bufferedInput),*this);
		} catch (Exception &e) {
			throw FailedToParseINIFileException(THISLOCATION,wpath) << e;
		}

	}

	void closeInclude() {
		includeStack.eraseFirst();
	}

	natural openFlags;

};

class IniIncludeLoaderFn {
public:
	IniIncludeLoaderFn(IniIncludeLoader &x):_x(x) {}
	IniIncludeLoader::Stream operator()(ConstStrA name) const {
		return _x.openInclude(name);
	}
	IniIncludeLoader &_x;
};

void IniConfigT::parseIniFromFile(ConstStrW fname, natural openFlags, bool ignoreErrors) {

	IniIncludeLoader loader(openFlags);
	IniIncludeLoader::Stream stream = loader.openInclude(fname);
	IniIncludeLoaderFn fn(loader);
	parseIni(stream,ignoreErrors,fn);

}

void FailedToParseINIFileException::message(ExceptionMsg& msg) const {
	msg("Failed to parse ini file: %1") << name;
}





}

