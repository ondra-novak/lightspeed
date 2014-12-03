/*
 * app.cpp
 *
 *  Created on: 14.12.2009
 *      Author: ondra
 */

#include <unistd.h>
#include "../framework/app.h"
#include <iostream>
#include "env.h"
#include <signal.h>
#include "seh.h"
#include "../containers/string.tcc"
#include <unistd.h>
#include "../../utils/FilePath.tcc"
#include "../../utils/FilePath.tcc"

namespace LightSpeed {

	static char  const *const *  envPtr;

	char const * const *  getEnv() {
		return envPtr;
	}

	int entryPoint(int argc, char* argv[], const char* const env[]) {
		int res;
		LinuxSEH::init();
		try {
			AppBase& app = AppBase::current();
			envPtr = env;
			app.init();
			//        sleep(30);
			char* cwdp = getcwd(0, 0);
			FilePath cwd = FilePath(String(cwdp));
			cwd = cwd / ConstStrA(argv[0]);
			free(cwdp);
			res = app.main_entry(argc, argv, cwd);
			app.done();
		} catch (std::exception& e) {
			std::cerr << "Exception on startup: " << e.what();
			res = 1;
		}
		return res;
	}

}


//force main declaration weak to prevent multiple definition of symbols when program defines own version of the main
int  __attribute__ ((weak)) main(int argc, char *argv[], const char * const env[]) {

	return LightSpeed::entryPoint( argc, argv, env);
}

void LightSpeed::IApp::exceptionDebug(const Exception &e, IApp *) {

    std::cerr << "Unhandled exception:" << std::endl
              << e.what() << std::endl;


}


void LightSpeed::AppBase::exceptionDebugLog(const Exception &e) {

    std::cerr << "DEBUG: Exception caught by framework:" << std::endl
              << e.what() << std::endl;

    onException(e);
}


