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
#ifndef _SERVER_H_
#define _SERVER_H_

#include <netdb.h>
#include "parameters.h"

typedef struct
{
    struct sockaddr_in  clientAddress;
    fd_set              selectReadSet;
    int                 listenSocket;
    int                 acceptSocket;
    int                 stdin;
    int                 stdout;
    int                 exitRunLoop;
    int                 highestReadFileDescriptor;
} Server;

void Server_Init(Server* pServer, Parameters* pParameters);
void Server_Uninit(Server* pServer);
void Server_WaitForClientToConnect(Server* pServer);
void Server_CloseClientConnection(Server* pServer);
void Server_Run(Server* pServer);
void Server_PrintClientAddress(Server* pServer);

#endif /* _SERVER_H_ */
