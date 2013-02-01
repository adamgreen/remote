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
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include "try_catch.h"
#include "parameters.h"

static void     zeroOutParametersStructure(Parameters* pParameters);
static void     allocateAndPopulateCommandArguments(Parameters* pParameters, int argc, const char** argv);
static int      commandArgumentCount(void);
static void     allocateCommandArguments(Parameters* pParameters, int numberOfArguments);
static void     populateCommandArguments(Parameters* pParameters, int argc, const char** argv);
static uint16_t parsePortNumber(const char* pPortNumberAsString);
static void     displayCommandArguments(Parameters* pParameters);

void Parameters_InitFromServerCommandLine(Parameters* pParameters, int argc, const char** argv)
{
    zeroOutParametersStructure(pParameters);
    
    if (argc < 2)
        __throw(invalidCommandLineException);
    
    pParameters->portNumber = parsePortNumber(argv[1]);
}

void Parameters_InitFromClientCommandLine(Parameters* pParameters, int argc, const char** argv)
{
    zeroOutParametersStructure(pParameters);
    
    if (argc < 4)
        __throw(invalidCommandLineException);
    
    pParameters->address = argv[1];
    pParameters->portNumber = parsePortNumber(argv[2]);
    allocateAndPopulateCommandArguments(pParameters, argc, argv);
}

void Parameters_Uninit(Parameters* pParameters)
{
    free(pParameters->ppCommandArguments);
    zeroOutParametersStructure(pParameters);
}

void Parameters_Display(Parameters* pParameters)
{
    printf("Port number: %d\n", pParameters->portNumber);
    displayCommandArguments(pParameters);
}

const char** Parameters_GetCommandArguments(Parameters* pParameters)
{
    return pParameters->ppCommandArguments;
}

const char*  Parameters_GetAddress(Parameters* pParameters)
{
    return pParameters->address;
}

uint16_t Parameters_GetPortNumber(Parameters* pParameters)
{
    return pParameters->portNumber;
}


static void zeroOutParametersStructure(Parameters* pParameters)
{
    memset(pParameters, 0, sizeof(*pParameters));
}

static void allocateAndPopulateCommandArguments(Parameters* pParameters, int argc, const char** argv)
{
    __try
        allocateCommandArguments(pParameters, commandArgumentCount());
    __catch
        __rethrow;
    
    populateCommandArguments(pParameters, argc, argv);
}

static int commandArgumentCount(void)
{
    static const int argumentsToAddForShellInvocationAndNullTermination = 3;
    static const int commandStringIsOneArgument = 1;

    return commandStringIsOneArgument + argumentsToAddForShellInvocationAndNullTermination;
}

static void allocateCommandArguments(Parameters* pParameters, int numberOfArguments)
{
    pParameters->ppCommandArguments = malloc(numberOfArguments * sizeof(*pParameters->ppCommandArguments));
    if (!pParameters->ppCommandArguments)
        __throw(outOfMemoryException);
}

static void populateCommandArguments(Parameters* pParameters, int argc, const char** argv)
{
    const char**  ppDest = pParameters->ppCommandArguments;
    
    *ppDest++ = "sh";
    *ppDest++ = "-c";
    *ppDest++ = argv[3];
    *ppDest++ = NULL;
}

static uint16_t parsePortNumber(const char* pPortNumberAsString)
{
    int portNumber = atoi(pPortNumberAsString);
    
    if (portNumber < 0 || portNumber > USHRT_MAX)
        __throw_and_return(invalidCommandLineException, 0);

    return (uint16_t)portNumber;
}

static void displayCommandArguments(Parameters* pParameters)
{
    const char** ppCurrent = pParameters->ppCommandArguments;
    
    printf("Command line of program to host: ");
    while (*ppCurrent)
    {
        printf("%s ", *ppCurrent);
        ppCurrent++;
    }
    printf("\n");
}
