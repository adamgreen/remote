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
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "try_catch.h"
#include "client.h"


static int g_childHasSignalled = 0;
static int g_controlCSignalled = 0;


static void flagStructureAsUninitialized(Client* pClient);
static void connectToServer(Client* pClient, Parameters* pParameters);
static void createSocket(Client* pClient);
static struct sockaddr_in lookupServerAddress(Client* pClient, Parameters* pParameters);
static void connectSocket(Client* pClient);
static void closeSocket(int socket);
static void setChildProcess(Client* pClient, Process* pChildProcess);
static void registerSignalHandlersToNotifyOnCtrlCAndChildExit(void);
static void childSignalHandler(int childPid);
static void controlCSignalHandler(int value);
static void moveDataBetweenChildAndServer(Client* pClient);
static int waitForInputFromChildServerOrConsole(Client* pClient);
static int isUnexpectedError(int selectResult);
static int wasInterrupted(int selectResult);
static int didTimeoutOccurAfterChildProcessSignalled(int selectResult);
static void processReadyData(Client* pClient);
static int doesChildStdoutHaveDataToRead(Client* pClient);
static int doesChildStderrHaveDataToRead(Client* pClient);
static int doesConsoleHaveDataToRead(Client* pClient);
static int doesServerHaveDataToRead(Client* pClient);
static void sendDataFromChildToServerAndConsole(Client* pClient, int fileDescriptor);
static void sendDataFromConsoleToServerAndChild(Client* pClient);
static void sendDataFromServerToConsoleAndChild(Client* pClient);
static void notifyServerIfControlCWasPressed(Client* pClient);
static void setHighestReadFileDescriptorNumber(Client* pClient);
static int max(int val1, int val2);


void Client_Init(Client* pClient, Parameters* pParameters)
{
    flagStructureAsUninitialized(pClient);
    
    __try
        connectToServer(pClient, pParameters);
    __catch
        __rethrow;
        
    pClient->stdin = fileno(stdin);
    pClient->stdout = fileno(stdout);
}

static void flagStructureAsUninitialized(Client* pClient)
{
    memset(pClient, 0xff, sizeof(*pClient));
}

static void connectToServer(Client* pClient, Parameters* pParameters)
{
    __try
    {
        __throwing_func( createSocket(pClient) );
        __throwing_func( pClient->serverAddress = lookupServerAddress(pClient, pParameters) );
        __throwing_func( connectSocket(pClient) );
    }
    __catch
    {
        __rethrow;
    }
}

static void createSocket(Client* pClient)
{
    pClient->clientSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (pClient->clientSocket < 0)
        __throw(socketException);
}

static struct sockaddr_in lookupServerAddress(Client* pClient, Parameters* pParameters)
{
    struct hostent*     pHostEntry = NULL;
    struct sockaddr_in  address;
    
    pHostEntry = gethostbyname(Parameters_GetAddress(pParameters));
    if (!pHostEntry)
        __throw_and_return(dnsLookupException, address);

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    memcpy(&address.sin_addr.s_addr, pHostEntry->h_addr_list[0], sizeof(address.sin_addr.s_addr));
    address.sin_port = htons(Parameters_GetPortNumber(pParameters));
    
    return address;
}

static void connectSocket(Client* pClient)
{
    int result = -1;
    
    result = connect(pClient->clientSocket, 
                     (const struct sockaddr*)&pClient->serverAddress, sizeof(pClient->serverAddress));
    if (result < 0)
        __throw(socketException);
}

void Client_Uninit(Client* pClient)
{
    closeSocket(pClient->clientSocket);

    flagStructureAsUninitialized(pClient);
}

static void closeSocket(int socket)
{
    if (socket >= 0)
        close(socket);
}

void Client_Run(Client* pClient, Process* pChildProcess)
{
    pClient->exitRunLoop = 0;
    setChildProcess(pClient, pChildProcess);
    registerSignalHandlersToNotifyOnCtrlCAndChildExit();
    setHighestReadFileDescriptorNumber(pClient);
    
    while (!pClient->exitRunLoop)
        moveDataBetweenChildAndServer(pClient);
}

static void setChildProcess(Client* pClient, Process* pChildProcess)
{
    pClient->pChildProcess = pChildProcess;
}

static void registerSignalHandlersToNotifyOnCtrlCAndChildExit(void)
{
    g_childHasSignalled = 0;
    g_controlCSignalled = 0;
    
    signal(SIGINT, controlCSignalHandler);
    signal(SIGCHLD, childSignalHandler);
}

static void childSignalHandler(int childPid)
{
    g_childHasSignalled = 1;
}

static void controlCSignalHandler(int value)
{
    g_controlCSignalled = 1;
}

static void moveDataBetweenChildAndServer(Client* pClient)
{
    int     selectResult = -1;
    
    notifyServerIfControlCWasPressed(pClient);

    selectResult = waitForInputFromChildServerOrConsole(pClient);
    if (isUnexpectedError(selectResult))
        __throw(selectException);

    if (!wasInterrupted(selectResult))
    {
        __try
            processReadyData(pClient);
        __catch
            __rethrow;
    }
    
    if (didTimeoutOccurAfterChildProcessSignalled(selectResult))
        pClient->exitRunLoop = 1;
}

static int waitForInputFromChildServerOrConsole(Client* pClient)
{
    static const struct timeval oneSecondTimeout = { 1, 0 };
    struct timeval              selectTimeout;
    
    FD_ZERO(&pClient->selectReadSet);
    FD_SET(pClient->pChildProcess->stdout, &pClient->selectReadSet);
    FD_SET(pClient->pChildProcess->stderr, &pClient->selectReadSet);
    FD_SET(pClient->stdin, &pClient->selectReadSet);
    FD_SET(pClient->clientSocket, &pClient->selectReadSet);

    selectTimeout = oneSecondTimeout;
    return select(pClient->highestReadFileDescriptor + 1, &pClient->selectReadSet, NULL, NULL, &selectTimeout);
}

static int isUnexpectedError(int selectResult)
{
    return selectResult < 0 && errno != EINTR;
}

static int wasInterrupted(int selectResult)
{
    return selectResult < 0 && errno == EINTR;
}

static int didTimeoutOccurAfterChildProcessSignalled(int selectResult)
{
    return selectResult == 0 && g_childHasSignalled;
}

static void processReadyData(Client* pClient)
{
    __try
    {
        if (doesChildStdoutHaveDataToRead(pClient))
            __throwing_func( sendDataFromChildToServerAndConsole(pClient, pClient->pChildProcess->stdout) );

        if (doesChildStderrHaveDataToRead(pClient))
            __throwing_func( sendDataFromChildToServerAndConsole(pClient, pClient->pChildProcess->stderr) );

        if (doesConsoleHaveDataToRead(pClient))
            __throwing_func( sendDataFromConsoleToServerAndChild(pClient) );

        if (doesServerHaveDataToRead(pClient))
            __throwing_func( sendDataFromServerToConsoleAndChild(pClient) );
    }
    __catch
    {
        __rethrow;
    }
}

static int doesChildStdoutHaveDataToRead(Client* pClient)
{
    return FD_ISSET(pClient->pChildProcess->stdout, &pClient->selectReadSet);
}

static int doesChildStderrHaveDataToRead(Client* pClient)
{
    return FD_ISSET(pClient->pChildProcess->stderr, &pClient->selectReadSet);
}

static int doesConsoleHaveDataToRead(Client* pClient)
{
    return FD_ISSET(pClient->stdin, &pClient->selectReadSet);
}

static int doesServerHaveDataToRead(Client* pClient)
{
    return FD_ISSET(pClient->clientSocket, &pClient->selectReadSet);
}

static void sendDataFromChildToServerAndConsole(Client* pClient, int fileDescriptor)
{
    char buffer[1];
    
    int bytesRead = read(fileDescriptor, buffer, sizeof(buffer));
    if (bytesRead < 0)
        __throw(childException);
    if (bytesRead == 0)
    {
        pClient->exitRunLoop = 1;
        return;
    }

    send(pClient->clientSocket, buffer, bytesRead, 0);
    write(pClient->stdout, buffer, bytesRead);
}

static void sendDataFromConsoleToServerAndChild(Client* pClient)
{
    char buffer[1];
    
    int bytesRead = read(pClient->stdin, buffer, sizeof(buffer));
    if (bytesRead < 0)
        __throw(consoleException);
    if (bytesRead == 0)
    {
        pClient->exitRunLoop = 1;
        return;
    }

    write(pClient->pChildProcess->stdin, buffer, bytesRead);
    send(pClient->clientSocket, buffer, bytesRead, 0);
}

static void sendDataFromServerToConsoleAndChild(Client* pClient)
{
    char buffer[1];
    
    int bytesRead = recv(pClient->clientSocket, buffer, sizeof(buffer), 0);
    if (bytesRead < 0)
        __throw(serverException);
    if (bytesRead == 0)
    {
        pClient->exitRunLoop = 1;
        return;
    }

    if (buffer[0] == 0x03)
    {
        static const char controlC[2] = "^C";
        
        kill(pClient->pChildProcess->pid, SIGINT);
        write(pClient->stdout, controlC, sizeof(controlC));
    }
    else
    {
        write(pClient->pChildProcess->stdin, buffer, bytesRead);
        write(pClient->stdout, buffer, bytesRead);
    }
}

static void notifyServerIfControlCWasPressed(Client* pClient)
{
    static const char controlC[2] = "^C";
    
    if (g_controlCSignalled == 0)
        return;
        
    send(pClient->clientSocket, controlC, sizeof(controlC), 0);
    g_controlCSignalled = 0;
}

static void setHighestReadFileDescriptorNumber(Client* pClient)
{
    int highest = 0;
    
    highest = max(highest, pClient->clientSocket);
    highest = max(highest, pClient->stdin);
    highest = max(highest, pClient->pChildProcess->stdout);
    highest = max(highest, pClient->pChildProcess->stderr);
    
    pClient->highestReadFileDescriptor = highest;
}

static int max(int val1, int val2)
{
    return (val1 > val2) ? val1 : val2;
}
