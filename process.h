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
#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "parameters.h"

typedef enum
{
    STDIN_READ = 0,
    STDIN_WRITE,
    STDOUT_READ,
    STDOUT_WRITE,
    STDERR_READ,
    STDERR_WRITE,
    PROCESS_PIPELINE_FILE_DESCRIPTOR_COUNT
} ProcessFileDescriptors;

typedef struct
{
    Parameters* pParameters;
    int         pipeFileDescriptors[PROCESS_PIPELINE_FILE_DESCRIPTOR_COUNT];
    int         stdin;
    int         stdout;
    int         stderr;
    int         pid;
} Process;

void Process_Init(Process* pProcess, Parameters* pParameters);
void Process_Uninit(Process* pProcess);

#endif /* _PROCESS_H_ */
