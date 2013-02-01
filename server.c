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
#include "server.h"


static int g_controlCSignalled = 0;

static void flagStructureAsUninitialized(Server* pServer);
static void createListeningSocket(Server* pServer, uint16_t portNumber);
static void createSocket(Server* pServer);
static void allowBindToReuseAddress(Server* pServer);
static void bindSocket(Server* pServer, uint16_t portNumber);
static struct sockaddr_in constructLocalBindAddress(uint16_t portNumber);
static void listenOnSocket(Server* pServer);
static void saveFileDescriptorForStdinStdout(Server* pServer);
static void closeSocket(int socket);
static void waitForConsoleInputOrNewClientConnection(Server* pServer);
static void registerSignalHandlersToNotifyOnCtrlC(void);
static void controlCSignalHandler(int value);
static void sendControlCIfSignalled(Server* pServer);
static void setHighestReadFileDescriptorNumber(Server* pServer);
static int max(int val1, int val2);
static void moveDataBetweenClientAndConsole(Server* pServer);
static int waitForInputFromClientOrConsole(Server* pServer);
static int isUnexpectedError(int selectResult);
static int wasInterrupted(int selectResult);
static void processReadyData(Server* pServer);
static int doesConsoleHaveDataToRead(Server* pServer);
static int doesClientHaveDataToRead(Server* pServer);
static void sendDataFromConsoleToClient(Server* pServer);
static void sendDataFromClientToConsole(Server* pServer);


void Server_Init(Server* pServer, Parameters* pParameters)
{
    flagStructureAsUninitialized(pServer);
    
    __try
        createListeningSocket(pServer, Parameters_GetPortNumber(pParameters));
    __catch
        __rethrow;

    saveFileDescriptorForStdinStdout(pServer);
}

static void flagStructureAsUninitialized(Server* pServer)
{
    memset(pServer, 0xff, sizeof(*pServer));
}

static void createListeningSocket(Server* pServer, uint16_t portNumber)
{
    __try
    {
        __throwing_func( createSocket(pServer) );
        __throwing_func( allowBindToReuseAddress(pServer) );
        __throwing_func( bindSocket(pServer, portNumber) );
        __throwing_func( listenOnSocket(pServer) );
    }
    __catch
    {
        __rethrow;
    }
}

static void createSocket(Server* pServer)
{
    pServer->listenSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (pServer->listenSocket < 0)
        __throw(socketException);
}

static void allowBindToReuseAddress(Server* pServer)
{
    int optionValue;

    optionValue = 1;
    setsockopt(pServer->listenSocket, SOL_SOCKET, SO_REUSEADDR, &optionValue, sizeof(optionValue));
}

static void bindSocket(Server* pServer, uint16_t portNumber)
{
    struct sockaddr_in  bindAddress;
    int    result = -1;
    
    bindAddress = constructLocalBindAddress(portNumber);
    result = bind(pServer->listenSocket, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    if (result < 0)
        __throw(socketException);
}

static struct sockaddr_in constructLocalBindAddress(uint16_t portNumber)
{
    struct sockaddr_in address;
    
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(portNumber);
    
    return address;
}

static void listenOnSocket(Server* pServer)
{
    int    result = -1;

    result = listen(pServer->listenSocket, 0);
    if (result < 0)
        __throw(socketException);
}

static void saveFileDescriptorForStdinStdout(Server* pServer)
{
    pServer->stdin = fileno(stdin);
    pServer->stdout = fileno(stdout);
}

void Server_Uninit(Server* pServer)
{
    closeSocket(pServer->acceptSocket);
    closeSocket(pServer->listenSocket);

    flagStructureAsUninitialized(pServer);
}

static void closeSocket(int socket)
{
    if (socket >= 0)
        close(socket);
}

void Server_WaitForClientToConnect(Server* pServer)
{
    socklen_t addressLength = sizeof(pServer->clientAddress);

    waitForConsoleInputOrNewClientConnection(pServer);
    if (doesConsoleHaveDataToRead(pServer))
        __throw(userShutdownException);
    
    pServer->acceptSocket = accept(pServer->listenSocket, (struct sockaddr*)&pServer->clientAddress, &addressLength);
    if (pServer->acceptSocket < 0)
        __throw(socketException);
}

static void waitForConsoleInputOrNewClientConnection(Server* pServer)
{
    int selectResult = -1;
    
    do
    {
        FD_ZERO(&pServer->selectReadSet);
        FD_SET(pServer->stdin, &pServer->selectReadSet);
        FD_SET(pServer->listenSocket, &pServer->selectReadSet);
        
        selectResult = select(pServer->listenSocket + 1, &pServer->selectReadSet, NULL, NULL, NULL);
    }
    while (selectResult < 0 && errno == EINTR);
}

void Server_CloseClientConnection(Server* pServer)
{
    closeSocket(pServer->acceptSocket);
}

void Server_Run(Server* pServer)
{
    pServer->exitRunLoop = 0;
    registerSignalHandlersToNotifyOnCtrlC();
    setHighestReadFileDescriptorNumber(pServer);
    
    while (!pServer->exitRunLoop)
        moveDataBetweenClientAndConsole(pServer);
}

static void registerSignalHandlersToNotifyOnCtrlC(void)
{
    g_controlCSignalled = 0;
    signal(SIGINT, controlCSignalHandler);
}

static void controlCSignalHandler(int value)
{
    g_controlCSignalled = 1;
}

static void setHighestReadFileDescriptorNumber(Server* pServer)
{
    int highest = 0;
    
    highest = max(highest, pServer->acceptSocket);
    highest = max(highest, pServer->stdin);
    
    pServer->highestReadFileDescriptor = highest;
}

static int max(int val1, int val2)
{
    return (val1 > val2) ? val1 : val2;
}

static void moveDataBetweenClientAndConsole(Server* pServer)
{
    int     selectResult = -1;
    
    sendControlCIfSignalled(pServer);

    selectResult = waitForInputFromClientOrConsole(pServer);
    if (isUnexpectedError(selectResult))
        __throw(selectException);

    if (!wasInterrupted(selectResult))
    {
        __try
            processReadyData(pServer);
        __catch
            __rethrow;
    }
}

static void sendControlCIfSignalled(Server* pServer)
{
    static const char controlC = 0x03;

    if (g_controlCSignalled == 0)
        return;
        
    send(pServer->acceptSocket, &controlC, sizeof(controlC), 0);
    g_controlCSignalled = 0;
}

static int waitForInputFromClientOrConsole(Server* pServer)
{
    static const struct timeval oneSecondTimeout = { 1, 0 };
    struct timeval              selectTimeout;
    
    FD_ZERO(&pServer->selectReadSet);
    FD_SET(pServer->stdin, &pServer->selectReadSet);
    FD_SET(pServer->acceptSocket, &pServer->selectReadSet);

    selectTimeout = oneSecondTimeout;
    return select(pServer->highestReadFileDescriptor + 1, &pServer->selectReadSet, NULL, NULL, &selectTimeout);
}

static int isUnexpectedError(int selectResult)
{
    return selectResult < 0 && errno != EINTR;
}

static int wasInterrupted(int selectResult)
{
    return selectResult < 0 && errno == EINTR;
}

static void processReadyData(Server* pServer)
{
    if (doesConsoleHaveDataToRead(pServer))
    {
        __try
            sendDataFromConsoleToClient(pServer);
        __catch
            __rethrow;
    }
    if (doesClientHaveDataToRead(pServer))
    {
        __try
            sendDataFromClientToConsole(pServer);
        __catch
            __rethrow;
    }
}

static int doesConsoleHaveDataToRead(Server* pServer)
{
    return FD_ISSET(pServer->stdin, &pServer->selectReadSet);
}

static int doesClientHaveDataToRead(Server* pServer)
{
    return FD_ISSET(pServer->acceptSocket, &pServer->selectReadSet);
}

static void sendDataFromConsoleToClient(Server* pServer)
{
    char buffer[1];
    
    int bytesRead = read(pServer->stdin, buffer, sizeof(buffer));

    if (bytesRead < 0)
        __throw(consoleException);

    if (bytesRead == 0)
    {
        pServer->exitRunLoop = 1;
        return;
    }
    send(pServer->acceptSocket, buffer, bytesRead, 0);
}

static void sendDataFromClientToConsole(Server* pServer)
{
    char buffer[1];
    
    int bytesRead = recv(pServer->acceptSocket, buffer, sizeof(buffer), 0);
    
    if (bytesRead < 0)
        __throw(clientException);

    if (bytesRead == 0)
    {
        pServer->exitRunLoop = 1;
        return;
    }
    write(pServer->stdout, buffer, bytesRead);
}

void Server_PrintClientAddress(Server* pServer)
{
    size_t   i;
    uint32_t address = ntohl(pServer->clientAddress.sin_addr.s_addr);
    
    for (i = 0 ; i < sizeof(address) ; i++)
    {
        uint32_t byte = (address >> 24) & 0xff;
        
        printf("%d%s", byte, (i < sizeof(address)-1) ? "." : "");
        
        address <<= 8;
    }
}
