/*
 * configParser.tcc
 *
 *  Created on: 6.5.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_UTILS_CONFIGPARSER_TCC_
#define LIGHTSPEED_UTILS_CONFIGPARSER_TCC_

#pragma once

#include "configParser.h"
#include "../base/text/textParser.tcc"
#include "../base/containers/arraySet.tcc"


namespace LightSpeed {

template<typename Iter>
class ConfigParser<Iter>::Error: public Exception {
public:
    enum ErrCode {
        unclosedSection,
        exceptingSeparator,
        exceptingEndOfQuotedValue,
    };

    LIGHTSPEED_EXCEPTIONFINAL;
    Error(const ProgramLocation &loc,
            ErrCode code,
            const StringA &iniloc):Exception(loc),
            code(code),iniloc(iniloc) {}
    const StringA &getIniLoc() const {return iniloc;}
    ErrCode getErrorCode() const {return code;}
    ~Error() throw() {}
protected:
    ErrCode code;
    StringA iniloc;

    void message(ExceptionMsg &msg) const
    {
        switch (code)
        {
            case unclosedSection: msg("Unclosed section name");break;
            case exceptingSeparator: msg("expecting serparator ="); break;
            case exceptingEndOfQuotedValue: msg("Excepting end of quoted value"); break;
            default: msg("unexcepted error");break;
        }

        msg(" (at: %1)") << iniloc.cStr();
    }

};

template<typename Iter>
bool ConfigParser<Iter>::getNextLine() {

    skipNotInteresting();
    if (input.hasItems()) {
        if (input.peek() == '[') {
            parseSection();
            return false;
        }
        else if (input.peek() == '.') {
        	return parseSpecialCommand();
        }
        else {
            parseField();
            return true;
        }
    } else{
        curSection.clear();
        return false;
    }
}

template<typename Iter>
const typename ConfigParser<Iter>::Str *ConfigParser<Iter>::getField(const char *field) const {
	if (curVar == ConstStrA(field)) return &curValue;
    else return 0;
}

template<typename Iter>
bool ConfigParser<Iter>::get(const char *field, integer &val) const {
    const Str *f = getField(field);
    if (f == 0) return false;
    parser("%1",*f);
    val = parser[1];
    return true;
}

template<typename Iter>
bool ConfigParser<Iter>::get(const char *field, double &val) const {
    const Str *f = getField(field);
    if (f == 0) return false;
    parser("%1",*f);
    val = parser[1];
    return true;
}

template<typename Iter>
bool ConfigParser<Iter>::get(const char *field, float &val) const {
    const Str *f = getField(field);
    if (f == 0) return false;
    parser("%1",*f);
    val = parser[1];
    return true;
}
template<typename Iter>
bool ConfigParser<Iter>::get(const char *field, StringCore<char> &val) const {
    const Str *f = getField(field);
    if (f == 0) return false;
    val = *f;
    return true;
}
template<typename Iter>
bool ConfigParser<Iter>::get(const char *field, Str &val) const {
    const Str *f = getField(field);
    if (f == 0) return false;
    val = *f;
    return true;
}

template<typename Iter>
const typename ConfigParser<Iter>::Str &ConfigParser<Iter>::getSection() const {return curSection;}

template<typename Iter>
void ConfigParser<Iter>::skipSection() {
    while (getNextLine());
}


template<typename Iter>
void ConfigParser<Iter>::parseSection() {

    bufferInit();
    char x = input.getNext();
    while (input.hasItems() && (x = input.peek())!=']' && x != '\r' && x != '\n') {
    	bufferPut(input.getNext());
    }
    if (x != ']')
    		throw Error(THISLOCATION, Error::unclosedSection, bufferGet());
    curSection = trim(bufferGet());
    input.skip();
}

template<typename Iter>
void ConfigParser<Iter>::parseField() {

    curVar = parseFieldName();
    curValue = parseValue();
}

template<typename Iter>
typename ConfigParser<Iter>::Str ConfigParser<Iter>::parseFieldName() {
    bufferInit();
    char nx = 0;
    while (input.hasItems() && (nx = input.peek()) != '='
    		&& nx != '\r' && nx != '\n') {
        bufferPut(input.getNext());
    }
    if (nx != '=') throw Error(THISLOCATION,
						Error::exceptingSeparator,bufferGet());
    input.skip();
    return trim(bufferGet());
}

template<typename Iter>
bool ConfigParser<Iter>::parseSpecialCommand() {
	if (curSection != '.') {
		curSection = ".";
		return false;
	}
    bufferInit();
    char nx = 0;
    while (input.hasItems() && (nx = input.peek()) != ' '
    		&& nx != '\r' && nx != '\n' ) {
        bufferPut(input.getNext());
    }
    if (nx != ' ') throw Error(THISLOCATION,
						Error::exceptingSeparator,bufferGet());
    input.skip();
    curVar =  trim(bufferGet());
    curValue = parseValue();
    return true;

}

template<typename Iter>
char ConfigParser<Iter>::getNextEscaped() {
	char nx = input.getNext();
	switch (nx) {
		case 'n':return('\n');
		case 'r':return('\r');
		case 't':return('\t');
		case 'a':return('\a');
		case 'b':return('\r');
		case '0':return('\0');
		case '\r':
		case '\n': while (input.hasItems() && isspace(input.peek()))
						input.skip();
				   return -1;
		default: return(nx);
	}
}

template<typename Iter>
typename ConfigParser<Iter>::Str ConfigParser<Iter>::parseValue() {
    bufferInit();
    if (!input.hasItems() ) return bufferGet();
    char nx = input.getNext();
    if (nx == '"' ) {
        while (input.hasItems()) {
        	nx = input.getNext();
			if (nx == '\\' && input.hasItems()) {
				nx = getNextEscaped();
				if (nx>=0) bufferPut(nx);
			} else if (nx == '"' || nx == '\r' || nx == '\n' ) {
				break;
			} else {
				bufferPut(nx);
			}
        }
        if (nx != '"') throw Error(THISLOCATION,
						Error::exceptingEndOfQuotedValue,bufferGet());

    } else while (input.hasItems()) {
			if (nx == '\r' || nx == '\n') {
				break;
			} else {
				bufferPut(nx);
			}
			nx = input.getNext();
    }
    return trim(bufferGet());
}

template<typename Iter>
void ConfigParser<Iter>::bufferInit() {
    buffer.clear();
}

template<typename Iter>
void ConfigParser<Iter>::bufferPut(char x) {
    buffer.add(x);
}

template<typename Iter>
typename ConfigParser<Iter>::Str ConfigParser<Iter>::bufferGet() {
	return Str(buffer,curVar.getAllocator());

}

template<typename Iter>
typename ConfigParser<Iter>::Str ConfigParser<Iter>::trim(const Str &other) {
    int begin = 0;
    int end = -1;
    for (unsigned int i = 0; i < other.length(); i++)
        if (!isspace((unsigned char)other.at(i))) {
            begin = i;
            end = 0;
            for (i = i + 1; i < other.length(); i++)
                if (!isspace((unsigned char)other.at(i))) end = i;
        }
    return other.mid(begin, end - begin + 1);
}

template<typename Iter>
int ConfigParser<Iter>::skipNotInteresting() {

    int i = 0;
    bool repeat;
    do {
        repeat = false;
        if (!input.hasItems())
        	break;
        i = input.peek();
        if (isspace(i)) {
        	input.skip();
        	repeat = true;
        }
        else if (i == '#') {
            repeat = true;
            while (i != '\n' && input.hasItems()) i = input.getNext();
        }
    }
    while(repeat);
    return i;

}

template<typename Iter, typename Fn>
void IniConfigT::merge(Iter &iter, bool ignoreErrors, Fn fn) {
	ConfigParser<Iter> parser(iter);
	parser.skipSection();
	while (!parser.curSection.empty()) {
		try {
			while (parser.getNextLine()) {
				if (parser.curVar[0] == '.') {
					if (parser.curVar == ".include") {
						merge2(fn(parser.curValue),ignoreErrors,fn);
					}

				} else {
					Str curSection = addPool(parser.curSection);
					Str curField = addPool(parser.curVar);
					Str curValue = addPool(parser.curValue);
					fields.insert(SectionField(curSection,curField),curValue);
				}
			}
		} catch (typename ConfigParser<Iter>::Error&) {
			if (!ignoreErrors)
				throw;
		}
	}
}

template<typename Iter, typename Fn>
void IniConfigT::merge2(Iter iter, bool ignoreErrors, Fn fn) {
	merge(iter,ignoreErrors,fn);
}

}

#endif /* LIGHTSPEED_UTILS_CONFIGPARSER_TCC_ */
