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
#ifndef _PARAMETERS_H_
#define _PARAMETERS_H_

#include <stdint.h>

typedef struct
{
    const char** ppCommandArguments;
    const char*  address;
    uint16_t     portNumber;
} Parameters;

void         Parameters_InitFromServerCommandLine(Parameters* pParameters, int argc, const char** argv);
void         Parameters_InitFromClientCommandLine(Parameters* pParameters, int argc, const char** argv);
void         Parameters_Uninit(Parameters* pParameters);
void         Parameters_Display(Parameters* pParameters);
const char** Parameters_GetCommandArguments(Parameters* pParameters);
const char*  Parameters_GetAddress(Parameters* pParameters);
uint16_t     Parameters_GetPortNumber(Parameters* pParameters);

#endif /* _PARAMETERS_H_ */
