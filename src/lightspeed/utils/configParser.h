#pragma once

#include <cctype>
#include "../base/text/textParser.h"
#include "../base/containers/arraySet.h"
#include "../base/containers/set.h"
#include "../base/memory/rtAlloc.h"
#include "../base/memory/smallAlloc.h"
#include "../base/containers/stringpool.h"
#include "../base/containers/map.h"


namespace LightSpeed {


template<typename Iter>
class ConfigParser
{
    Iter input;
    AutoArray<char, RTAlloc> buffer;
    TextParser<char> parser;
 public:
    typedef StringTC<char, StrCmpCI> Str;
  

    Str curSection;
    Str curVar;
    Str curValue;

    class Error;

    ConfigParser(Iter input):input(input),buffer(RTAlloc(StdAlloc::getInstance())) {}

    bool getNextLine();
    const Str *getField(const char *field) const;
    bool get(const char *field, integer &val) const ;
    bool get(const char *field, double &val) const;
    bool get(const char *field, float &val) const;
    bool get(const char *field, StringCore<char> &val) const;
    bool get(const char *field, Str &val) const;

    const Str &getSection() const ;
    
    void skipSection();
    

protected:
    void parseSection();
    void parseField();

    Str parseFieldName();
    bool parseSpecialCommand();
    char getNextEscaped();

    Str parseValue();
    void bufferInit();
    
    void bufferPut(char x);

    Str bufferGet();
    Str trim(const Str &other);
    int skipNotInteresting();
};


class IniConfigT {
protected:
	typedef StringPoolStrRef<char> Str;


	struct SectionField: public ComparableLess<SectionField> {
		Str section;
		Str field;

		bool lessThan(const SectionField &other) const {
			StrCmpCI<char> cmp;
			CompareResult r = cmp(section,other.section);
			if (r == cmpResultLess) return true;
			if (r == cmpResultEqual) {
				r = cmp(field,other.field);
				if (r == cmpResultLess) return true;
			}
			return false;
		}

		SectionField(Str section, Str field)
			:section(section),field(field) {}
	};
private:
	typedef MultiMap<SectionField,Str> FieldMap;

    typedef Set<Str> NameSet;
	FieldMap fields;
	NameSet allstr;
	StringPoolA strpool;


    Str addPool(ConstStrA text);
	template<typename Iter, typename Fn>
	void merge(Iter &iter, bool ignoreErrors, Fn fn) ;

	template<typename Iter, typename Fn>
	void merge2(Iter iter, bool ignoreErrors, Fn fn);

public:

	static ConstStrA::Iterator errorIncludeCB(ConstStrA include);

	template<typename Iter>
    explicit IniConfigT(Iter iter,bool ignoreErrors = false) {
    	parseIni(iter,ignoreErrors,errorIncludeCB);
    }

	template<typename Iter, typename Fn>
    IniConfigT(Iter iter,Fn fn, bool ignoreErrors) {
    	parseIni(iter,ignoreErrors,fn);
    }

	explicit IniConfigT(ConstStrW fname, natural openFlags, bool ignoreErrors = false) {
		parseIniFromFile(fname,openFlags,ignoreErrors);
	}

	template<typename Iter, typename Fn>
    void parseIni(Iter iter,bool ignoreErrors, Fn specCmdHandler) {

        fields.clear();
        allstr.clear();
        strpool.clear();

		merge(iter, ignoreErrors, specCmdHandler);
    }

	void parseIniFromFile(ConstStrW fname,natural openFlags,  bool ignoreErrors);

    const Str *getValue(ConstStrA section, const ConstStrA field, natural index = 0) const;
    class RequiredFieldException: public Exception {
    public:
    	LIGHTSPEED_EXCEPTIONFINAL;
    	RequiredFieldException(const ProgramLocation &loc, StringA section,
    			StringA field,natural index)
			:Exception(loc),section(section),field(field),index(index) {}
    	~RequiredFieldException() throw() {}
    protected:
    	StringA section;
    	StringA field;
    	natural index;

    	void message(ExceptionMsg &msg) const;
    };

    struct FieldVal {
    	ConstStrA field;
    	ConstStrA val;
    };


    class Iterator: public IteratorBase<FieldVal,Iterator> {
    public:
    	Iterator(const FieldMap &container, ConstStrA section, ConstStrA prefix);

    	bool hasItems() const;
    	const FieldVal &peek() const;
    	const FieldVal &getNext();

    protected:
    	FieldMap::Iterator innerIter;
    	ConstStrA section;
    	ConstStrA prefix;
    	mutable FieldVal storedItem;

    	bool match(const FieldMap::Entity &e)const;

    };

    Iterator enumFields(ConstStrA section, ConstStrA prefix=ConstStrA()) const;



    friend class Section;
    class Section {

        const IniConfigT &owner;
		StringA section,prefix;
		mutable TextParser<char, SmallAlloc<256> > parser;

    public:
		Section(const IniConfigT &owner, const StringA &section, const StringA &prefix)
			:owner(owner),section(section),prefix(prefix) {}

		Section &operator=(const Section &other);

		const Str *getValue(const ConstStrA &field, natural index) const;

		Section openSection(const ConstStrA &name) const;

		Section openPrefix(const ConstStrA &pfx) const;

		bool get(StringCore<char> &val, const ConstStrA &var, natural index = 0) const;
		bool get(ConstStrA &val, const ConstStrA &var, natural index = 0) const;
		bool get(String &val, const ConstStrA &var, natural index = 0) const;
		bool get(natural &val, const ConstStrA &var, natural index = 0) const;
		bool get(integer &val, const ConstStrA &var, natural index = 0) const;
		bool get(float &val, const ConstStrA &var, natural index = 0) const;

		bool get(double &val, const ConstStrA &var, natural index = 0) const;
		bool get(bool &val, const ConstStrA &var, natural index = 0) const;

		template<typename T>
		void required(T &val, const ConstStrA &var, natural index = 0) const {
			if (get(val,var,index)) return;
			else {
				throw RequiredFieldException(
						THISLOCATION,section,prefix+var,index);
			}
		}

        const IniConfigT &getConfig() const {return owner;}
        const StringA &getSection() const {return section;}
		const StringA &getPrefix() const {return prefix;}

		Iterator getFwIter() const;
    };

    Section openSection(const ConstStrA &section) const {
    	return Section(*this,section,ConstStrA());
    }




};


class FailedToParseINIFileException: public Exception {
public:
	LIGHTSPEED_EXCEPTIONFINAL;
	FailedToParseINIFileException(const ProgramLocation &loc, String name)
		:Exception(loc),name(name) {}
	~FailedToParseINIFileException() throw () {}

	const String &getFilename() const {return name;}

protected:
	String name;

	void message(ExceptionMsg &msg) const;
};

class IniConfigSection: public IniConfigT::Section {
public:
	IniConfigSection(const IniConfigT::Section &sect):
		IniConfigT::Section (sect) {}
};

class IniConfig: public IniConfigT {
public:
	IniConfig(ConstStrW fname, natural openFlags, bool ignoreErrors = false)
		:IniConfigT(fname,openFlags,ignoreErrors) {}

	template<typename Iter>
    IniConfig(Iter iter, bool ignoreError = false):IniConfigT(iter,ignoreError) {}

	IniConfigSection openSection(const ConstStrA &section) const {
		return IniConfigT::openSection(section);
	}
	IniConfigSection openSection(const char * section) const {
		return IniConfigT::openSection(ConstStrA(section));
	}
};


  
}


