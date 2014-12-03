/*@file
 *
 * This file contains functions which throws most often used exceptions.
 * File has very little dependencies to other files, and also it doesn't
 * depend on exception.h. Sometime it solves problem with circular dependencies
 *
 * non-template functions are implemented in liblightspeed directly. Template
 * functions are implemented in throws.tcc and should be included in every
 * *.cpp file which use them.
 *
 * throws.h
 *
 *  Created on: 6.10.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_EXCEPTIONS_THROWS_H_
#define LIGHTSPEED_EXCEPTIONS_THROWS_H_

#pragma once

#include "../types.h"
#include "../debug/programlocation.h"
#include <typeinfo>



namespace LightSpeed {


	///throws exception IteratorNoMoreItems
   /**
    * Function replaces throw IteratorNoMoreItems when the exception class
    *  is not yes available. Function is implemented later, but this
    *  prototype is enough to throw the exception
    *
    * @param loc Location in the source
    * @param subject subject
    * @exception IteratorNoMoreItems throws this exception,
    */

   struct ProgramLocation;
   void throwIteratorNoMoreItems(const ProgramLocation &loc,
                                       const std::type_info &subject);

   ///throws exception WriteIteratorNoSpace
   /**
    * Function replaces throw WriteIteratorNoSpace when the exception class
    *  is not yes available. Function is implemented later, but this
    *  prototype is enough to throw the exception
    *
    * @param loc Location in the source
    * @param subject subject
    * @exception WriteIteratorNoSpace throws this exception,
    */

   void throwWriteIteratorNoSpace(const ProgramLocation &loc,
                                       const std::type_info &subject);

   ///throws unsupported feature
   /**
    * @param loc program location
    * @param instance instance of the class, which doesn't support feature
    * @param feature name of unsupported feature
    */
   template<typename X>
   void throwUnsupportedFeature(const ProgramLocation &loc,
                                const X *instance,
                                const char *feature);


   void throwAllocatorLimitException(const ProgramLocation &loc,
                               natural reqCount,
                               natural avalableCount,
                               const std::type_info &allocatorType);
   void throwOutOfMemoryException(const ProgramLocation &loc,natural reqCount);

   template<typename T>
   void throwRangeException_FromTo(const ProgramLocation &loc,
               T rangeFrom , T rangeTo, T val);
   template<typename T>
   void throwRangeException_From(const ProgramLocation &loc,
               T rangeFrom , T val);
   template<typename T>
   void throwRangeException_To(const ProgramLocation &loc,
               T rangeTo , T val);
   template<typename T>
   void throwRangeException_NoRange(const ProgramLocation &loc,T val);

   void throwRangeException(const ProgramLocation &loc);

   void throwNullPointerException(const ProgramLocation &loc);

   void exceptionRethrow(const ProgramLocation &loc);


}  // namespace LightSpeed


#endif /* LIGHTSPEED_EXCEPTIONS_THROWS_H_ */
