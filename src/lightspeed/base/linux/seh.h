#pragma once

#ifndef LINUXLIGHTSPEED_SEH
#define LINUXLIGHTSPEED_SEH

#include <iostream>
#include <signal.h>
#include <setjmp.h>

namespace LightSpeed {

///begin of SEH try section - use similar to try-catch
#define __seh_try for (::LightSpeed::LinuxSEH __sehContext;__sehContext.enter();) if (__sehContext.catchRes(sigsetjmp(__sehContext,1)))
///begin of SEH except section.
/**
 * @param x specify variable name (int) which receives signal number
 */
#define __seh_except(x) else if (int x = __sehContext.except())

///throws SEH exception. Useful to rethrow exception to the next handler
/**
 * @param x value passed to the __seh_except. If value is 0, causes repeating of __seh_try block
 */
#define __seh_throw(x) ::LightSpeed::LinuxSEH::throwf(x)
///Helper object to handle SEH under Linux
/** It is always accessible as __sehContext inside try-except block */
class LinuxSEH {
public:

	///constructor - prepares context
	LinuxSEH();
	///destructor - performs cleanup
	~LinuxSEH();

	///called inside macro __seh_try
	/** causes, that for cycle will be executed once */
	inline bool enter()  {return swp = !swp;}
	///allows to convert context to jmp_buf to inicialize setjmp
	operator __jmp_buf_tag *() { return jmpbuf;}
	///catches signal number after handler is executed. It is part of __seh_try macro
	/**
	 * @param val value returned by setjmp. After handler is executed, it returns signal number
	 */
	inline bool catchRes(int val) {return (signum = val) == 0;}

	///used to implement __seh_except. Performs cleanup object's internal state and returns signal number
	int except();

	///signal handler
	static void signalHandler(int sig);
	///initializes LinuxSEH signal actions - SIGSEGV, SIGBUS and SIGILL
	static void init();
	///initializes LinuxSEH to catch specified signal
	/**
	 * @param signum signal number to catch using SEH
	 */
	static void initSignal(int signum);

	static void throwf(int signum);

protected:

	jmp_buf jmpbuf;
	__jmp_buf_tag *prevjmpbuf;
	mutable bool swp;
	int signum;

	static const struct sigaction standardSigAction;
};

}

#endif
