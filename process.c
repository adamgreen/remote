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
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "try_catch.h"
#include "process.h"

static void flagProcessStructureAsEmpty(Process* pProcess);
static void initPipes(Process* pProcess);
static void createPipes(Process* pProcess);
static void forkNewProcessAndSetupPipes(Process* pProcess);
static void setupPipesInChildProcess(Process* pProcess);
static void executeNewCommandInChildProcess(Process* pProcess);
static const char** getCommandArguments(Process* pProcess);
static void setChildPid(Process* pProcess, int pid);
static void setupFileDescriptorsUsedByParentToCommunicateWithChild(Process* pProcess);
static void closePipeFileDescriptors(Process* pProcess);
static void closePipeFileDescriptor(int fileDescriptor);
static void killChildProcess(Process* pProcess);

void Process_Init(Process* pProcess, Parameters* pParameters)
{
    flagProcessStructureAsEmpty(pProcess);
    pProcess->pParameters = pParameters;

    __try
    {
        __throwing_func( initPipes(pProcess) );
        __throwing_func( forkNewProcessAndSetupPipes(pProcess) );
    }
    __catch
    {
        __rethrow;
    }
}

void Process_Uninit(Process* pProcess)
{
    closePipeFileDescriptors(pProcess);
    killChildProcess(pProcess);
    flagProcessStructureAsEmpty(pProcess);
}

static void flagProcessStructureAsEmpty(Process* pProcess)
{
    memset(pProcess, 0xff, sizeof(*pProcess));
}

static void initPipes(Process* pProcess)
{
    __try
        createPipes(pProcess);
    __catch
        __rethrow;

    setupFileDescriptorsUsedByParentToCommunicateWithChild(pProcess);
}

static void createPipes(Process* pProcess)
{
    int result = -1;
    
    result = pipe(&pProcess->pipeFileDescriptors[STDIN_READ]);
    if (result)
        __throw(pipeException);
    
    result = pipe(&pProcess->pipeFileDescriptors[STDOUT_READ]);
    if (result)
        __throw(pipeException);

    result = pipe(&pProcess->pipeFileDescriptors[STDERR_READ]);
    if (result)
        __throw(pipeException);
}

static void setupFileDescriptorsUsedByParentToCommunicateWithChild(Process* pProcess)
{
    pProcess->stdin = pProcess->pipeFileDescriptors[STDIN_WRITE];
    pProcess->stdout = pProcess->pipeFileDescriptors[STDOUT_READ];
    pProcess->stderr = pProcess->pipeFileDescriptors[STDERR_READ];
}

static void forkNewProcessAndSetupPipes(Process* pProcess)
{
    int pid = -1;
    
    pid = fork();
    if (pid < 0)
    {
        __throw(forkException);
    }
    else if (pid == 0)
    {
        setupPipesInChildProcess(pProcess);
        executeNewCommandInChildProcess(pProcess);
    }
    else
    {
        setChildPid(pProcess, pid);
    }
}

static void setupPipesInChildProcess(Process* pProcess)
{
    int result = -1;
    
    result = dup2(pProcess->pipeFileDescriptors[STDIN_READ], fileno(stdin));
    if (result < 0)
        exit(1);
    result = dup2(pProcess->pipeFileDescriptors[STDOUT_WRITE], fileno(stdout));
    if (result < 0)
        exit(1);
    result = dup2(pProcess->pipeFileDescriptors[STDERR_WRITE], fileno(stderr));
    if (result < 0)
        exit(1);
}

static void executeNewCommandInChildProcess(Process* pProcess)
{
    int result = -1;
    
    result = execv("/bin/sh", (char* const*)getCommandArguments(pProcess));
    if (result < 0)
        exit(1);
}

static const char** getCommandArguments(Process* pProcess)
{
    return Parameters_GetCommandArguments(pProcess->pParameters);
}

static void setChildPid(Process* pProcess, int pid)
{
    pProcess->pid = pid;
}

static void closePipeFileDescriptors(Process* pProcess)
{
    int i = 0;
    
    for (i = 0 ; i < PROCESS_PIPELINE_FILE_DESCRIPTOR_COUNT ; i++)
    {
        closePipeFileDescriptor(pProcess->pipeFileDescriptors[i]);
    }
}

static void closePipeFileDescriptor(int fileDescriptor)
{
    if (fileDescriptor >= 0)
        close(fileDescriptor);
}

static void killChildProcess(Process* pProcess)
{
    if (pProcess->pid >= 0)
        kill(pProcess->pid, SIGKILL);
}
