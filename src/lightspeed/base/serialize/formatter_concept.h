#ifndef LIGHTSPEED_SERIALIZER_FORMATTER_CONCEPT_H_
#define LIGHTSPEED_SERIALIZER_FORMATTER_CONCEPT_H_

#include "basicTypes.h"
#include "../containers/constStr.h"
#include "sectionId.h"
namespace LightSpeed
{


    ///Describes formatter/parser concept
    /** This is the concept. Deriving this class will not have
     *  effect in program code, because class contains only declarations.
     *  They should to help you build own formatter/parser 
     */
    class Formatter_Concept {

    public:

		template<typename T>
		class TypeTable: public Serializable<T> {};
               
        ///declaration of function that performs data exchange
        /** Function is called during variable serialization.
         * 
         * At this point, variable has been processed and has not been
         * found none handler that could handle this variable. It
         * excepts, that formatter knows the way, how to serialize this
         * type. Formatter don't need to declare this function as an
         * template. It can be declared as list of overloaded methods,
         * each for different type
         * 
         * @param object object to exchange.
         * @exception IOException any I/O error
         * @exception SerializerException serializator exceptions
         * @exception UnsupportedFeature if formatter doesn't support this object
         */
        template<class T> void exchange(T &object);
            
       
        bool isStoring() const;

        ///Opens named section
        /** Function should take name and create or open new section it the stream
         Function can choose section by name, or it can complette ignore the name, if
         section is identified by the order. Once section is opened, it remains
         active until section is closed. Sections can be nested, so it is valid to open
         section inside of another opened section. Each openSection must to have corresponding
         closeSection

         @param section section name
         @retval true section has been opened
         @retval false section has not been found
         @note if stream doesn't support sections or sections with the names, 
                    function should return true.
         @see closeSection, openTaggedSection
         */

        bool openSection(const StdSectionName &section);

        ///Closes named section
        /**
        @param section section to close
        
        Function should close all nested sections opened inside this section. If this
         happened during reading, stream should skip all unreadable data. If stream
         doesn't support this feature, exception can be thrown.

         @see openSection, closeTaggedSection

         */
        void closeSection(const StdSectionName &section);


        ///Opens section with specified tag
        /**
         * Main difference between openSection and openTaggedSection is, that tagged
         * section must be written into the stream  with its name, because name
         * is mandatory and following data depends on it.
         *
         * @param sectName specifies name of section. Prototyp uses StdSectionName due
         * compatibility reason, but you can use any class which has base of SectionId. There
         * can be two versions of function. First version allows only constant parameter
         * which is used while storing and can be limited during loading while
         * loading serializer is trying to guess name of following section from
         * the limited set of names where one name from the list will success.
         * Second version allows writable parameter, where parser stores
         * name of following section in case, that variable contains different name
         *
         * @retval true section has been opened. In all cases, parameter is not modified
         * and can be used for closing the section
         * @retval false section has not been opened, because parser expection
         * another section. If parameter is writable, function stores name
         * of expected section into it.
         *
         * @see closeTaggedSection, openSection
         */
        bool openTaggedSection(const StdSectionName &sectName);

        bool openTaggedSection(StdSectionName &sectName);


        ///Closes section opened by openTaggedSection
        /**
         * Works similar to closeSection, but can be used to close tagged section.
         * Do not mix closeSection with closeTaggedSection. Use apropriate function
         * to close section
         *
         * @param sectName section to close
         */
        void closeTaggedSection(StdSectionName &sectName);

        
        ///Opens or skips section.
        /** In open mode, function replaces openSection. So don't forget
         * to close section by calling closeSection. In skip mode, section
         * is skipped, you don't need to close the section
         * 
         * The current mode is reported by the 'skip' mode.
         * 
         * @b Storing: When function is called, parameter 'skip' have to contain
         *   result of variable and default value comparsion. The value can
         *   be true, that content of variable is equal to the default value,
         *   or false, if they are not equal. Function modifies this variable
         *   before its return, and you have to carry out the final decision. 
         *   Function should not change parameter 'skip' from 'false' to 'true', 
         *   but it can change parameter from 'true' to 'false', if skipping
         *   is not allowed.
         * 
         * @b Loading: When function is called, parameter 'skip' can be
         *   uninitialized. After return, parameter skip can contain true, 
         *   if section has been skipped... so you can return default value,
         *   or false when section exists and has been opened. Serialization
         *   have to continer and you have to call closeSection after 
         *   serialization is done.
         * 
         * @param blockName name of section to skip or open
         * @param skip reference to variable, that contains required operation
         * and which receives final decision.
         * 
         * @note When function's decision is 'skip = true', section is not
         * opened and you should not call closeSection
         * 
         * @noe When function's decisition is 'skip = false', section is
         * opened and you have to close the section.
         * 
         * @see openSection, closeSection           
         */             
		bool openSectionDefValue(const StdSectionName &name, bool cmpResult);

		/// Opens array section
		/** 
		   Array section should be openned, when caller wants to story
		   limited count of items, for example while serializing containers
		   Each item don't need the name, fomatter will able to find begin
		   and end of each item
		 * @param count specifies count of object in the array. When writing
					this value is mandatory and must contain true count
					of items. While reading, this value is ignored
		 * @return Count of items in the next array. Function can return 
					naturalNull, if count is not known. This may happen
					when formatter doesn't store count before the array
					and uses closing mark to find end of array. In
					all cases, you need to use returned value to make
					memory reservation, but must be also prepared, that
					reserved memory may not be enough to read whole array
					
			@note array section may be nested.
		 */
		natural openArray(natural count);

		///Marks or ask for next item
		/**Call this function between serialization of each item in the array
		   You needn't to call this function before serialization of the first 
		   item. While storing, you also needn't to call this function after
		   the last item. But while loading, this is only way, how to 
		   find, that there is another item. You will check returned value
		   in the while cycle and reads item until false returned

		   If function returns true while loading, you have to also check
		   count of items returned by openArray decreased on every loading
		   cycle. Even if this function returns true, you have to stop loading
		   items, when the count reaches zero. This because formatters that
		   stores count of items at the beginning of the section will not probably
		   store item-separators and thus they are not able to return correct
		   information.

			@retval true no informations or default behaviour
			@retval false no more items in the array
			*/
		bool nextArrayItem();

		///Closes array section
		void closeArray();

		///Returns true, if there is no more information in current section and can be closed
		/**
		 * @retval true there are data in the section
		 * @retval false there no more data in current section
	     *
		 * Formatter can use this to report end of reserved space. 
		 * Parser will report end of section
		 */
		bool hasSectionItems() const;
};


} // namespace LightSpeed

#endif /*FORMATTER_CONCEPT_H_*/
