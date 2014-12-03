/*
 * serviceapp.cpp
 *
 *  Created on: 22.2.2011
 *      Author: ondra
 */

#include "../framework/serviceapp.h"
#include "winpch.h"

namespace LightSpeed {

	
void ServiceAppBase::daemonMode() {
	DWORD handles[3]={STD_INPUT_HANDLE,STD_OUTPUT_HANDLE,STD_ERROR_HANDLE};	
	for(int i = 0; i < 3; i++) {
		HANDLE h = GetStdHandle(handles[i]);		
		if (h != 0 && h != INVALID_HANDLE_VALUE)  {
			DWORD type = GetFileType(h);
			if (type != FILE_TYPE_UNKNOWN && type != FILE_TYPE_CHAR) CloseHandle(h);
			SetStdHandle(handles[i],0);
		}
	}
	FreeConsole();
}


String ServiceApp::getDefaultInstanceName(const ConstStrW arg0) const {
	natural k = arg0.findLast('\\');
	return arg0.offset(k+1);
}
}
