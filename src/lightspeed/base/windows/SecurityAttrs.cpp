#include "winpch.h"
#include "SecurityAttrs.h"
#include <AccCtrl.h>
#include <Aclapi.h>
#include <sddl.h>
#include "..\exceptions\systemException.h"




PSECURITY_DESCRIPTOR SecurityAttributes_t::createSecurityDesc(const _TCHAR *str) {
	
	PSECURITY_DESCRIPTOR pSD = NULL;  
	if (ConvertStringSecurityDescriptorToSecurityDescriptor(
		str, SDDL_REVISION_1, &pSD, NULL)) 
	{
		return pSD;
	} else {
		throw LightSpeed::ErrNoException(THISLOCATION,GetLastError());
	}
}

void  SecurityAttributes_t::freeSecurityDesc(PSECURITY_DESCRIPTOR desc)
{
	LocalFree((HLOCAL)desc);
}


static const _TCHAR *appContainerIdent = _T("S:(ML;;NW;;;LW)D:(A;;0x120083;;;WD)(A;;0x120083;;;AC)");
static const _TCHAR *appContainerRights = _T("S:(ML;;NW;;;LW)D:(A;;0x10120083;;;WD)(A;;0x10120083;;;AC)");

static const _TCHAR *lowIntegrityIdent = _T("S:(ML;;NW;;;LW)");



SecurityAttributes_t SecurityAttributes_t::getLowIntegrity(
	bool inheritsHandle) 
{
	

	try {
		return SecurityAttributes_t(
			createSecurityDesc(lowIntegrityIdent),inheritsHandle);
	} catch (LightSpeed::ErrNoException &) {
		return SecurityAttributes_t(inheritsHandle);
	}


}

SecurityAttributes_t SecurityAttributes_t::getAppCointanerIntegrity(bool inheritsHandle) {

	


	try {
		return SecurityAttributes_t(
			createSecurityDesc(appContainerIdent),inheritsHandle);
	} catch (LightSpeed::ErrNoException &) {
		return SecurityAttributes_t(inheritsHandle);
	}



}