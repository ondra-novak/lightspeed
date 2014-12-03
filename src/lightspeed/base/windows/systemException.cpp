/*
 * systemException.cpp
 *
 *  Created on: 11.9.2009
 *      Author: ondra
 */

#include "winpch.h"
#include "../exceptions/systemException.h"

namespace LightSpeed {

void ErrNoException::getSystemErrorMessage(ExceptionMsg &msg, int error_nr) {

    wchar_t buffer[2048] = L"<n/a>";
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
        0,error_nr,LANG_USER_DEFAULT,buffer,2048,0);

    msg("%1")<<buffer;
}

}
