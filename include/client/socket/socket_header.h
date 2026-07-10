/**
    Copyright [stag-project] [stag-project]

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
#ifndef STAGDEER_SOCKET_HEADER
#define STAGDEER_SOCKET_HEADER

#ifdef __linux__
    #define STAGDEER_GNU_LINUX
#else
    #define STAGDEER_WIN32
#endif

#ifdef STAGDEER_GNU_LINUX
    //TODO: Linux header files
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <cstring>
#else
    //TODO: Windows header files
#endif

#ifdef STAGDEER_GNU_LINUX
    #define socket_setFcntl(__M_socket_fd)\
    int __M_flags = fcntl(__M_socket_fd, F_GETFL, 0);\
    fcntl(__M_socket_fd, F_SETFL, __M_flags | O_NONBLOCK)
#else
    u_long __M_mode = 1;\
    ioctlsocket(__M_socket_fd, FIONBIO, &__M_mode)
#endif

#ifdef STAGDEER_GNU_LINUX
    #define INVALID_FD -1
    typedef int M_SOCKET_TP;
    #else
    //TODO: Windows fd Type
#endif

#ifdef STAGDEER_GNU_LINUX
    #define CLOSE_FD(M_fd__)\
        close(M_fd__)
    #else
    //TODO: Windows close fd
#endif

#endif