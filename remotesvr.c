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
#include <ctype.h>
#include "try_catch.h"
#include "parameters.h"
#include "server.h"


static void displayUsage(void)
{
    printf("Usage:   remotesvr port\n"
           "  Where: port is the TCP/IP port for this server to listen upon.\n");
}


static void displayClientAddress(Server* pServer);
static int shouldConnectionBeAllowed(void);
static void eatConsoleInput(void);


int main(int argc, const char** argv)
{
    Parameters      parameters;
    Server          server;

    __try
    {
        Parameters_InitFromServerCommandLine(&parameters, argc, argv);
    }
    __catch
    {
        Parameters_Uninit(&parameters);
        displayUsage();
        return 1;
    }
    
    __try
    {
        Server_Init(&server, &parameters);
    }
    __catch
    {
        printf("error: Failed to initialize server (%d).\n", getExceptionCode());
        perror("       errno");
        Parameters_Uninit(&parameters);
        Server_Uninit(&server);
        return 1;
    }
    
    while (1)
    {
        printf("Waiting for client to connect...\n");
        __try
        {
            __throwing_func( Server_WaitForClientToConnect(&server) );
            __throwing_func( displayClientAddress(&server) );
            if (shouldConnectionBeAllowed())
            {
                __throwing_func( Server_Run(&server) );
                printf("Connection shutdown by client.\n");
            }
            Server_CloseClientConnection(&server);
        }
        __catch
        {
            Parameters_Uninit(&parameters);
            Server_Uninit(&server);
            if (getExceptionCode() == userShutdownException)
            {
                eatConsoleInput();
                printf("Shutting down at user's request.\n");
                return 0;
            }

            printf("error: Failed in server code (%d).\n", getExceptionCode());
            perror("       errno");
            return 1;
        }
    }
}

static void displayClientAddress(Server* pServer)
{
    printf("Client connection attempt from ");
    Server_PrintClientAddress(pServer);
    printf("\n");
}

static int shouldConnectionBeAllowed(void)
{
    char buffer[32];
    
    while (1)
    {
        printf("Do you accept this connection (y/n)? ");
        fgets(buffer, sizeof(buffer), stdin);
        if (tolower(buffer[0]) == 'y')
            return 1;
        else if (tolower(buffer[0]) == 'n')
            return 0;
    }
}

static void eatConsoleInput(void)
{
    char buffer[128];
    
    fgets(buffer, sizeof(buffer), stdin);
}
