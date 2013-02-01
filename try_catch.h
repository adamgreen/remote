/* Copyright 2012 Adam Green (http://mbed.org/users/AdamGreen/)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#ifndef _TRY_CATCH_H_
#define _TRY_CATCH_H_

static const int noException = 0;
static const int invalidCommandLineException = 1;
static const int outOfMemoryException = 2;
static const int pipeException = 3;
static const int forkException = 4;
static const int socketException = 5;
static const int dnsLookupException = 6;
static const int consoleException = 7;
static const int clientException = 8;
static const int selectException = 9;
static const int childException = 10;
static const int serverException = 11;
static const int userShutdownException = 12;

extern int g_exceptionCode;

#define __try \
        do \
        { \
            clearExceptionCode();

#define __throwing_func(X) \
            X; \
            if (g_exceptionCode) \
                break;

#define __catch \
        } while (0); \
        if (g_exceptionCode)

#define __throw(EXCEPTION) \
        { \
            setExceptionCode(EXCEPTION); \
            return; \
        }

#define __throw_and_return(EXCEPTION, RETURN) \
        {\
            setExceptionCode(EXCEPTION); \
            return RETURN; \
        }
        
#define __rethrow return

#define __rethrow_and_return(RETURN) return RETURN

static inline int getExceptionCode(void)
{
    return g_exceptionCode;
}

static inline void setExceptionCode(int exceptionCode)
{
    g_exceptionCode = exceptionCode > g_exceptionCode ? exceptionCode : g_exceptionCode;
}

static inline void clearExceptionCode(void)
{
    g_exceptionCode = noException;
}

#endif /* _TRY_CATCH_H_ */
