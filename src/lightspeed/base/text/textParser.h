/*
 * textParser.h
 *
 *  Created on: 1.2.2011
 *      Author: ondra
 */

#ifndef LIGHTSPPED_TEXT_TEXTPARSER_H_
#define LIGHTSPPED_TEXT_TEXTPARSER_H_

#include "../containers/constStr.h"
#include "../containers/autoArray.h"
#include "../memory/pointer.h"
#include "newline.h"


#pragma once

namespace LightSpeed {



class StdAlloc;

///Object taking responsibility to (un)format number from the international format to C format
class INumberUnformatter {
public:
	virtual ConstStrA operator()(ConstStrA number) = 0;
	virtual ~INumberUnformatter() {}
};

template<typename T> class IVtIterator;
template<typename T> class IVtWriteIterator;

///Parses input controlled by pattern
/**
 * This class implements similar parsing as standard scanf, or regular expression.
 * In compare to scanf it is more powerful, in compare to regular expression is simplified
 * and doesn't contain as many features as regular expressions. It is
 * implemnted using one-pass parsing (regular expressions need two-pass parsing,
 * where expression is compiled during first pass)
 *
 * Pattern is interpreded in two main modes
 *  - format validation
 *  - data extraction
 *
 * Format validation is performed by comparing pattern characters with input
 * text and when pattern matches, result if validation is true. Data extraction
 * uses special escape sequences in pattern, which defines places where
 * data are located and which format data have.
 *
 * Default mode is format validation. Starting by the first character if pattern,
 * object compares pattern character by character with the input stream.
 *
 * @code
 * TextParser<char> parser;
 * bool a = parser("Hello world",input);
 * @endcode
 *
 * This example returns true, when input contains text "Hello world" exactly as
 * requested (case sensitive and no other characters are allowed)
 *
 * @code
 * bool a = parser("Hello world%",input);
 * @endcode
 *
 * This example will match text "Hello world" at the beginning of the input
 * The character '%' at the end of pattern means, that input can continue,
 * method will not test end of input ( hasItems() )
 *
 * To enter into data extraction mode, you have to use character '%' anywhere
 * in pattern excluding end of pattern. To match character '%' instead to
 * entering into the data extraction mode, double the character. There are
 * several places, where you have to double, triple or quadple this character
 * to reach requested operation.
 *
 * Pattern to data extraction starts with '%' and ends by number optionally
 * followed by '%'. Number specifies one-based index, under which extracted
 * value will be available to the program. The simplest example is:
 *
 * @code
 * bool a = parser("%1",input);
 * bool b = parser("%1%",input);
 * @endcode
 *
 * Both are valid. Note, if you want to place '%' at the end of pattern, you
 * have to double '%' everytime. The only correct form is "%1%%",
 *
 * If you want to extract two fields separated by comma, use following example
 *
 * @code
 * bool a = parser("%1,%2",input);
 * int p1 = parser[1];
 * int p2 = parser[2];
 * @endcode
 *
 * Note that parser doesn't check validity of data, so input can contain
 * text instead expected number which leads to throwing an  exception
 * during transfering the result into a variable
 *
 * The simplest form uses "asterix search" while pattern is matched. Above
 * example requests, that fields are separated by comma. Method
 * repeatedly loads first field and test input for comma. When comma is found,
 * it is discarded and method starts load second field until end of input
 * is reached. "Asterix search" is recursive searching, which can do backward
 * steps, when remaining part of input doesn't match the pattern. Fields
 * with asterix search are repeatedly loaded and when remaining pattern matches,
 * method stops and returns the result.
 *
 * Instead to using asterix search, you can define, how input should look like and
 * when reading should stop
 *
 * @code
 *   //extract only a-z characters (stop on first character outside the range
 * bool a = parser("%[a-z]1%%",input);
 *
 *   //extract only a-z and characters ] and [ (first ] after [ is not interpreted)
 * bool a = parser("%[][a-z]1%%",input);
 *
 *   //extract only +,- and numbers (first '-' is not interprted)
 * bool a = parser("%[-+0-9]1%%",input);
 *
 *   //extract until digit is reached
 * bool a = parser("%[^0-9]1%%",input);
 *
 *   //use asterix search, but always stop on digit.
 * bool a = parser("%[^*0-9]1,%2",input);
 *
 *   //extract only characters ^ and \ (first \\ escapes first ^)
 * bool a = parser("%[\\^\\]%1%%",input);
 *
 *   //extract one to ten characters a-z. Stops on 11 character even if will be from given range
 * bool a = parser("%(1,10)[a-z]%1%%",input);
 * @endcode
 * 
 * 
 *
 * Inside data extraction mode, you can define range of expected size of the result
 * (min,max) where min < max. If max is *, it means that range is unlimited:
 *  (5,*) - five or more characters
 *
 * Inside the data extraction mode, you can combine multiple defintions, each
 * is processed in order as it is defined.
 *
 * @code
 *   //match sign optionally and number as sequence of digits, store into field 1
 * bool a = parser("%(0,1)[-+](1,*)[0-9]1",input);
 * @endcode
 *
 * To invoke asterix search, use star "*"
 *
 * @code
 *   //Match 0-9 then everything enough to match a-z and store into 1
 * bool a = parser("%[0-9]*[a-z]1",input);
 * @endcode
 *
 * To extract text inside quotes
 *
 * @code
 *   //Extract text from quotes
 * bool a = parser("%'\"\"1",input);
 * @endcode
 *
 * Quote extraction starts with single quote mark followed by character used
 * as quote and then follows character used to escape quote inside of text.
 * Using two same characters means, that quotes are doubled. Used another
 * character means, that this character must prepend quote to interpret
 * sequence as quote, not end of quoted text.
 *
 * @code
 *   //Extract text from quotes which will be escaped by \
 * bool a = parser("%'\"\\1",input);
 * @endcode
 *
 * Note that you still need escape characters in C string.
 *
 * Additional extraction commands:
 * @a %d1 - matches and extracts integer number optionally with a sign
 * @a %u1 - matches and extracts unsigned number, only + is allowed
 * @a %f1 - matches and extracts float number
 * @a %x1 - matches and extracts hex number
 * @a %o1 - matches and extracts octal number
 *
 *
 * Field 0 is used to extract and ignore data. You can use field 0 to
 * advanced matching without extracting. For example to match end of string,
 * use this pattern "%0this is the end". %0 starts asterix search, which
 * match only on text at the end of the input. Collected data before this text
 * are discarded.
 *
 * You can use one fields from multiple places in the pattern. Collected
 * data in the field are merged in order of appearance. Implementation note: This
 * is not yet efficient, because this means, that collected data will be
 * reinserted into the collector making fragmentation of the collector container
 * and taking some extra memory.
 *
 * Some advanced pattern commands
 * @b in @b format @b validation mode, use \b to eat all white characters. Using
 * of \n matches end of line which depends on current platform. Under MS Windows
 * it have to match sequence \r\n
 *
 * Quadpling of '%'. If you want to extract data separated by '%': "%1%%%%2".
 * Just see, why you need four '%'. First '%' enters into extraction mode and
 * extracts first field. Even if number means the exit from the extraction
 * mode, '%' is interpreted as optional exit (sometime can be useful, for example
 * in the pattern "%1%0" - stops extraction on character 0). 3 of '%' remains. To
 * match '%' we need double '%', so next 2 of '%' are interpreted as match of '%'.
 * Finally last of '%' is start of extraction mode for second field.
 *
 * Tripling of '%' is often used at the end of pattern: "yes%%%", while
 * text "yes" ends with "%" and additional data are allowed".
 *
 * Look to this pattern: "%1%%", as described above, first '%' starts extraction
 * mode, second '%' must be specified to exit extraction mode and third
 * '%' specified, there can be more characters in the input. Note that "%1%"
 * is different (equivalent to "%1")
 *
 * @par Format validation specialities
 *
 * As described above, format validation mode performs comparsion character
 * by character and accepts format only if pattern matches. There are some
 * special characters, which are validated differently
 *
 * Character ' ' space matches space or tab or nothing. If there are more
 * such a characters, one space matches all of them. You can specify white 
 * characters by calling functions setWS()
 *
 * Character '\n' matches end of the line, not character itself. If end
 * if line is defined as different character or character sequence, it matches
 * predefined sequence. See setNL() function
 *
 * Character '\t' matches space or tab and there must be at least one character
 * otherwise it is not matched
 *
 * Character '\b' matches any whitespace including new line, and there must
 * be at least one character
 *
 * If you want to match exactly these characters, use data extraction form with
 * argument 0
 *
 * %(1,1)[\n]0% - match exactly one \n character
 *
 *
 * Assertations
 * new from 12.4 version
 * 
 * use %! to change parameter extraction to negative assertion. If extraction is successful, 
 * pattern is rejected. If extraction is unsuccessful, extraction continues with next character. 
 * you can use this to assert what input must not contain
 *
 * "%1%%![\]%" - first argument is extracted using *search.
 * Extraction stops, when remaining part of pattern is validated.
 * Assertion ensures, that extraction of backslash is not possible.
 * Then check for presence of quote. If backslash founds, pattern
 * is not validated and *search continues to search end of argument
 * You cannot replace this by [^\] because this extracts character
 * and doesn't validate quote
 * 
 * 
 *

 *@tparam T type of character used while parsing
 *@tparam Alloc allocator used to allocate internal buffers
 *@tparam Cmp functor used to compare pattern with source. Useful to make case insensitive comparsion for example
 */
template<typename T, typename Alloc = StdAlloc, typename Cmp = DefaultCompare<T> >
class TextParser {
public:

	typedef  ConstStringT<T> Str;

	///Initializes parser
	TextParser():nl(DefaultNLString<T>()) {}
	///Initializes parser and allocator
	/**
	 * @param alloc instance of allocator
	 */
	TextParser(const Alloc &alloc):fragments(alloc),nl(DefaultNLString<T>()) {}

	TextParser(const Alloc &alloc, const Cmp &cmp)
			:cmp(cmp),fragments(alloc),nl(DefaultNLString<T>()) {}

	///Parses from array
	/**
	 * @param pattern pattern
	 * @param source array contains the input
	 * @retval true matched and parsed
	 * @retval false not matched (result is undefined)
	 */
	template<typename Impl>
	bool operator()(Str pattern, const ArrayT<T,Impl> &source);

	///Parsed from stream
	/**
	 * @param pattern pattern
	 * @param source iterator through the stream
	 * @retval true matched and extracted
	 * @retval false not matched and not extracted
	 *
	 * @note Source stream must support branching of iterators. When
	 * iterator is branches, result is two iterators, which can
	 * read source independently to each other. This is not the case
	 * of SeqInputStream, which doesn't support iterator branching, but you
	 * can use TextInBuffer class to extend any such iterator with this feature.
	 * Or you can use TextParser extension declared as TextOut which has
	 * been designed to work with iterator without branch support especially
	 * work with console or external file or pipe.
	 *
	 *
	 */
	template<typename Impl>
	bool operator()(Str pattern, const IIterator<T, Impl> &source);

	class Result {
	public:
		operator Str() const;
		operator T() const;
		operator int() const;
		operator unsigned int() const;
		operator lnatural() const;
		operator linteger() const;
		operator float() const;
		operator double() const;

		ConstStringT<T> str() const;

		Result(const TextParser &owner, natural index):owner(owner),index(index) {}
		natural hex() const;
		natural basedNum(natural base) const;
	protected:
		const TextParser &owner;
		natural index;
	};

	///Retrieves field
	/**
	 * @param pos one-based index of field. Field 0 is not accessible
	 * @return
	 */
	Result operator[](int pos) const;

	void setUnformatter(INumberUnformatter *f);
	INumberUnformatter *getUnformatter() const;

	///Sets new sequence for new line.
	/**
	 * @param nl new "new line" sequence.
	 */
	TextParser &setNL(const ConstStringT<T> &nl) {this->nl = nl;return *this;}
	TextParser &setWS(const ConstStringT<T> &ws) {this->ws = ws;return *this;}

	 ///Custom parser - allows to define own rules to parse data
	 class ICustomParser {
	 public:
		 ///Handles custom format parsing
		 /**
		  * @param symbol contains symbol appeared next after '%'
		  * @param pattern contains pattern iterator. Parser can read additional symbols from pattern
		  * @param data contains data iterator. Parser should read data and analyze them
		  * @param output Iterator to write output. Note that output is subject of range checking.
		  *   Iterator is limited to maximum characters that can be parsed. If count
		  *   of character written is less than required minimum, custom parser returns
		  *   false ignoring returned value from this function.
		  * @param alloc reference to allocator. Parser should use this allocator to
		  * 	allocate memory
		  * @param cmp functor used to compare pattern with data. Functor returns true, when
		  *    arguments are same
		  * @param nextTry useful to support asterix searching. After function returns
		  *    true, parser continues read next pattern and matches data. If parser
		  *    fails to  match data to pattern, function is called again
		  *    to continue parsing. This is signaled by nextTry is set to true. Otherwise
		  *    field is set to false.
		  * @param state pointer to store user data. There is space to store user pointer.
		  *    The pointer is same while function is called again due nextTry is equal to true
		  * @retval true Continue parsing. If future parsing is not successful,
		  * 	function is called again having nextTry set to true. To reject chance
		  * 	to continue parsing in order to do asterix search, make return false
		  * 	as response to nextTry is true.
		  * @retval false Stop parsing, pattern is not matched (data cannot be accepted)
		  *		You should return false when nextTry is true and there is no space
		  *		to store next data.
		  *
		  *
		  */
		 virtual bool parse(const T &symbol,
						 IVtIterator<T> &pattern,
						 IVtIterator<T> &data,
						 IVtWriteIterator<T> &output,
						 Alloc &alloc,
						 const Cmp &cmp,
						 bool nextTry,
						 void **state) = 0;

		 ///Called when parser destroyes instance of custom parser giving handler chance to manage own resources
		 /**
		  * @param state pointer to instance data, which can be released now.
		  *
		  * @note function is called always, regardless to result of parsing. Function
		  * is NOT called, when user data pointer is set to zero.
		  */
		 virtual void release(void **state) = 0;

	 };

	 ///Activates custom parser
	 /**
	  * @param parser pointer to parser. Use NULL to deactivate custom parser.
	  * @return function returns previous custom parser.
	  *
	  * @note object does't take ownership of the instance of the parser. It must be managed by caller.
	  */
	 ICustomParser *setCustomParser(ICustomParser *parser);

	 ///Retrieves custom parser
	 ICustomParser *getCustomParser() const;

protected:

	class Fragment: public FlatArrMid<const AutoArray<T,Alloc> &>{
	public:
		typedef FlatArrMid<const AutoArray<T,Alloc> &> Super;
		Fragment(const AutoArray<T,Alloc> &data):Super(data,0,0) {}
		Fragment(const AutoArray<T,Alloc> &data, natural beg, natural len):Super(data,beg,len) {}

		Fragment &operator=(const Fragment &other) {
			this->~Fragment();
			return *new(this) Fragment(other);
		}
	};
	Cmp cmp;
	AutoArray<Fragment, Alloc> fragments;
	AutoArray<T, Alloc> data;
	Pointer<INumberUnformatter> unform;
	ConstStringT<T> nl,ws;
	Pointer<ICustomParser> custom;

	typedef typename Str::Iterator PatternIter;
	typedef std::pair<natural,natural> Range;

	struct StepState {
		natural dataBeg;
		Range range;
		bool defaultAsterix;

		StepState(natural dataBeg):dataBeg(dataBeg),range(1,naturalNull),defaultAsterix(true) {}
	};

	template<typename SourceIter>
	bool parse(PatternIter &patiter, SourceIter &srciter);
	template<typename SourceIter>
	bool parseArg(PatternIter &patiter, SourceIter &srciter);
	template<typename SourceIter>
	bool parseArg(PatternIter &patiter, SourceIter &srciter, StepState &state);

	template<typename SourceIter>
	bool parseCharList(PatternIter &patiter, SourceIter &srciter, StepState &state);
	template<typename SourceIter>
	bool parseAsterix(PatternIter &patiter, SourceIter &srciter, StepState &state);
	template<typename SourceIter>
	bool parseAsterixEnter(PatternIter &patiter, SourceIter &srciter, StepState &state);
	template<typename SourceIter>
	bool parseQuoted(PatternIter &patiter, SourceIter &srciter, StepState &state);
	template<typename SourceIter>
	bool parseClass(PatternIter &patiter, SourceIter &srciter, StepState &state, ConstStrA pattern);
	template<typename SourceIter>
	bool parseAlternateBranch(PatternIter &patiter,SourceIter &srciter);
	template<typename SourceIter>
	bool skipAlternateBranch(PatternIter &patiter,SourceIter &srciter);


	void skipCharlist(PatternIter &iter);

	natural parseRange(PatternIter &patiter, Range &range);
	void skipRange(PatternIter &iter);
	void skipQuoted(PatternIter &iter);
	void skipPattern(PatternIter &iter);	
	bool skipToNextBranch(PatternIter &patiter);
	static void retrivalError(const ProgramLocation &loc);
	bool inclass;

	template<typename SourceIter>
	bool handleCustomPatterns(const T &command, PatternIter &patiter, SourceIter &srciter, StepState &state);
	template<typename SourceIter>
	bool parseNumber( SourceIter &srciter, natural rangeMax, bool negativeSign, natural base);
	template<typename SourceIter>
	bool parseFloat(SourceIter &srciter, natural rangeMax);
	template<typename SourceIter>
	bool parseIdentifier( SourceIter &srciter, natural rangeMax);

};





namespace _intr {

	template<typename T>
	T stringToUnsignedNumber(ConstStrA num, natural base);
	template<typename T>
	T stringToSignedNumber(ConstStrA num, natural base);
	template<typename T>
	T stringToFloatNumber(ConstStrA num);


	template<typename T, typename Alloc>
	class StoreToCharBuff: public AutoArray<char, Alloc> {
		typedef AutoArray<char, Alloc> Super;
	public:
		StoreToCharBuff(ConstStringT<T> par);
	};

	template<typename Alloc>
	class StoreToCharBuff<wchar_t,Alloc>: public AutoArray<char, Alloc> {
		typedef AutoArray<char, Alloc> Super;
	public:
		StoreToCharBuff(ConstStringT<wchar_t> par);
	};
}

template<typename Iter, typename NumT>
bool parseUnsignedNumber(Iter &iter, NumT &number, natural base = 10);
template<typename Iter, typename NumT>
bool parseUnsignedNumber(const Iter &iter, NumT &number, natural base = 10);
template<typename Iter, typename NumT>
bool parseSignedNumber(Iter &iter, NumT &number, natural base = 10);
template<typename Iter, typename NumT>
bool parseSignedNumber(const Iter &iter, NumT &number, natural base = 10);

}

#endif /* LIGHTSPPED_TEXT_TEXTPARSER_H_ */
