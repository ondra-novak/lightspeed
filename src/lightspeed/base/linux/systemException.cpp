/*
 * systemException.cpp
 *
 *  Created on: 11.9.2009
 *      Author: ondra
 */

#include <string.h>
#include "../exceptions/systemException.h"

namespace LightSpeed {

void ErrNoException::getSystemErrorMessage(ExceptionMsg &msg, int error_nr) {

    char buff[2048] = "";
    strerror_r(error_nr,buff,sizeof(buff));
    msg(buff);
}

}
