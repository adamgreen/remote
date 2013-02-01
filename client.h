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
#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <netdb.h>
#include "parameters.h"
#include "process.h"

typedef struct
{
    Process*            pChildProcess;
    struct sockaddr_in  serverAddress;
    fd_set              selectReadSet;
    int                 clientSocket;
    int                 stdout;
    int                 stdin;
    int                 exitRunLoop;
    int                 highestReadFileDescriptor;
} Client;

void Client_Init(Client* pClient, Parameters* pParameters);
void Client_Uninit(Client* pClient);
void Client_Run(Client* pClient, Process* pProcess);

#endif /* _CLIENT_H_ */
