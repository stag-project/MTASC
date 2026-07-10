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
#ifndef STAGDEER_CLIENT_IPV6
#define STAGDEER_CLIENT_IPV6

#include "../socket_header.h"
#include <stdexcept>
#include <string>
#include <utility>

namespace stagdeer {
    namespace ip {
        class Ipv6addrs {
            public:

            ~ Ipv6addrs() {
                freeaddrinfo(M_ipv6addrinfo__);
                return;
            }

            Ipv6addrs() = default;
            Ipv6addrs(stagdeer::ip::Ipv6addrs&& other_Ipv6addrs) 
                noexcept:
                    M_ipv6addrinfo__(other_Ipv6addrs.M_ipv6addrinfo__) {};
                    Ipv6addrs& operator=(stagdeer::ip::Ipv6addrs&& other_Ipv6addrs_operator)
                    noexcept{
                        if (this != &other_Ipv6addrs_operator) {
                            M_ipv6addrinfo__ = other_Ipv6addrs_operator.M_ipv6addrinfo__;
                        }
                        return *this;
                    }
            Ipv6addrs(const stagdeer::ip::Ipv6addrs&) = delete;
            Ipv6addrs& operator=(stagdeer::ip::Ipv6addrs&) = delete;

            Ipv6addrs(const char* M_hostname__ , uint16_t M_port__) {
                struct addrinfo M_hints{}, *M_result = nullptr;
                M_hints.ai_family = AF_INET6;
                M_hints.ai_socktype = SOCK_STREAM;
                M_hints.ai_protocol = IPPROTO_TCP;
                char M_addrs_port_cstr_buf[16];
                snprintf(M_addrs_port_cstr_buf , sizeof(M_addrs_port_cstr_buf) , "%u" , M_port__);
                int M_return = getaddrinfo(M_hostname__,  M_addrs_port_cstr_buf , &M_hints, &M_result);
                if (M_return != 0) {
                    const char* M_err_str = gai_strerror(M_return);
                    if (M_err_str == nullptr) {
                        throw std::runtime_error("Resolver hostname failed!: unknown error");
                    } throw std::runtime_error("Resolver hostname failed: " + 
                        std::string(M_err_str));
                }
                M_ipv6addrinfo__ = M_result;
            }

            struct addrinfo* getResolverResult() {
                if (!M_ipv6addrinfo__) {
                    throw std::runtime_error("M_ipv6addrinfo__ is NullPtr!");
                }
                return M_ipv6addrinfo__;
            }

            private:

            struct addrinfo* M_ipv6addrinfo__ = nullptr;
        };
    }
}

#endif