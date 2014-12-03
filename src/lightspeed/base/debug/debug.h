#pragma once

//#include "../obsolete/exceptLib.h"
#include "programlocation.h"

#undef assert

namespace LightSpeed {



	static inline void assertImpl(bool cond, const char *expression, const char *desc, const ProgramLocation &loc) {
		/*if (cond) throw AssertFailedException(loc,expression,desc);*/
	}

	static inline void assertImpl(bool cond, const char *expression, const ProgramLocation &loc) {
		/*if (cond) throw AssertFailedException(loc,expression);*/
	}

};

///Assert command works similar to POSIX assert(x)
/** 
  Main difference is, that assert failed is thrown as exception. This macro
  remains also in the release version. To make assert test for debug only
  version, use assertDebug macro

  @param x expression to test
  */
#define assert(x) ::LightSpeed::assertImpl(x,#x,THISLOCATION);

///Assert command works similar to POSIX assert(x)
/** 
Main difference is, that assert failed is thrown as exception. This macro
remains also in the release version. To make assert test for debug only
version, use assertDebug macro

@param x expression to test
@param desc description added to the error message, when assertion fails
*/
#define assertDesc(x,desc) ::LightSpeed::assertImpl(x,#x,desc,THISLOCATION);


#if defined(_DEBUG) || defined(DEBUG) 


///Assert command, activated only when _DEBUG or DEBUG macro is defined
#define assertDebug(x) assert(x)
///Assert command, activated only when _DEBUG or DEBUG macro is defined
#define assertDebugDesc(x,desc) assertDesc(x,desc)

#define verifyDebug(x) assert(x)

#define verifyDebugDesc(x,desc) assert(x,desc)

#else

///Assert command, activated only when _DEBUG or DEBUG macro is defined
#define assertDebug(x) 
///Assert command, activated only when _DEBUG or DEBUG macro is defined
#define assertDebugDesc(x,desc) 

#define verifyDebug(x) (x)

#define verifyDebugDesc(x,desc) (x)

#endif
