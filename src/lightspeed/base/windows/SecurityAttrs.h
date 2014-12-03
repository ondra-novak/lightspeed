#pragma once
#include "..\memory\sharedResource.h"



#ifndef CORE_EXTERN
#define CORE_EXTERN
#endif

class CORE_EXTERN SecurityAttributes_t: public SECURITY_ATTRIBUTES ,
						  public LightSpeed::SharedResource {
public:
	SecurityAttributes_t(bool inheritHandle) {
		this->nLength = sizeof(SECURITY_ATTRIBUTES);
		this->bInheritHandle = inheritHandle?TRUE:FALSE;
		this->lpSecurityDescriptor = 0;
	}


	SecurityAttributes_t(PSECURITY_DESCRIPTOR desc, bool inheritHandle) {
		this->nLength = sizeof(SECURITY_ATTRIBUTES);
		this->bInheritHandle = inheritHandle?TRUE:FALSE;
		this->lpSecurityDescriptor = desc;
	}
	
	~SecurityAttributes_t() {
		if (!isShared() && lpSecurityDescriptor) {
			freeSecurityDesc(lpSecurityDescriptor);
		}
	}
	

	static SecurityAttributes_t getLowIntegrity(bool inheritsHandle = false);
	static SecurityAttributes_t getAppCointanerIntegrity(bool inheritsHandle = false);

	static PSECURITY_DESCRIPTOR createSecurityDesc(const _TCHAR *str);
	static void freeSecurityDesc(PSECURITY_DESCRIPTOR desc);

protected:



};