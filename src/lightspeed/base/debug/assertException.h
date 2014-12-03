/*
 * assertException.h
 *
 *  Created on: 7.9.2009
 *      Author: ondra
 */

#pragma once
#ifndef _LIGHTSPEED_ASSERTEXCEPTION_H_
#define _LIGHTSPEED_ASSERTEXCEPTION_H_

#include "../memory/scopePtr.h"

namespace LightSpeed {
#if 0

    ///Implements Assert failed exception
        /**
          Assert failed exception is exception thrown when program fails on
          assert condition. Unlike other standard libraries, "assertion failed"
          is not lead to program exit through abort() but as other exception,
          it is thrown out. This allows program to control errors reported
          in unexpected conditions and try to recover this error state
          */

        class AssertFailedException: public ErrorMessageException {

        public:
            AssertFailedException(const ProgramLocation &loc,
                const char *expression)
                : Exception(loc),ErrorMessageException(loc,"Expression is not true."),expression(expression) {}
            AssertFailedException(const ProgramLocation &loc,
                const char *expression, const std::string &desc)
                : Exception(loc),ErrorMessageException(loc,desc),expression(expression) {}

            const char *getExpression() const {return expression;}

            virtual ~AssertFailedException() throw() {}

        protected:
            ScopePtr<char *,ReleaseDeleteArrayRule> expression;

            static char *allocExpression(const char *expression) {
                natural len = 0;
                while (expression[len++]);
                char *res = new char[len];
                for(natural x = 0; x < len; x++) res[x] = expression[x];
                return res;
            }

            virtual void getMsgText(std::ostream &msg) const {
                msg << "Assertion has failed: '" << expression << "'. ";
                ErrorMessageException::getMsgText(msg);
            }
        };


#endif
}

#endif /* _LIGHTSPEED_ASSERTEXCEPTION_H_ */
