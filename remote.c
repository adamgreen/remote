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
#include "try_catch.h"
#include "parameters.h"
#include "process.h"
#include "client.h"


static void displayUsage(void)
{
    printf("Usage:   remote server port \"command\"\n"
           "  Where: server is the TCP/IP address of the server.\n"
           "         port is the TCP/IP port that the server is listening upon.\n"
           "         command is the command to be executed by the shell and\n"
           "           provide interactive I/O to the remote user.\n");
}


int main(int argc, const char** argv)
{
    Parameters      parameters;
    Client          client;
    Process         process;

    __try
    {
        Parameters_InitFromClientCommandLine(&parameters, argc, argv);
    }
    __catch
    {
        Parameters_Uninit(&parameters);
        displayUsage();
        return 1;
    }
    
    __try
    {
        Client_Init(&client, &parameters);
    }
    __catch
    {
        printf("error: Failed to initialize client (%d).\n", getExceptionCode());
        perror("       errno");
        Parameters_Uninit(&parameters);
        Client_Uninit(&client);
        return 1;
    }
    
    __try
    {
        __throwing_func( Process_Init(&process, &parameters) );
        __throwing_func( Client_Run(&client, &process) );
        printf("Connection being shutdown.\n");
    }
    __catch
    {
        printf("error: Failed in run (%d).\n", getExceptionCode());
        perror("       errno");
    }

    Process_Uninit(&process);
    Client_Uninit(&client);
    Parameters_Uninit(&parameters);
    
    return 0;
}
