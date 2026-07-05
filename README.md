## MTASC - CLIENT - Frmework / ![C++ 20 <]

### 

<div style="flex-direction: row; gap: 5px; justify-content: center; margin: 5px;">
    <span>
         Asynchronous client framework, users build client libraries relying on lightweight POSIX-Socket,
          OpenSSL, and C++ Thread. Since it’s aimed at 'client' 
          development, Epoll/Iocp are not used and supports IPv4/IPv6.
           Welcome brothers to suggest RP, so the code becomes more stable !
          <hr>
    </span>
    <img style="width: 100%; border-radius:20px; height:300px;" src="github_assets/mtasc-logo.png"></img>
</div>

![License](https://img.shields.io/badge/License-Apachea2.0-blue.svg)
![C++](https://img.shields.io/badge/C++-20-blue.svg)
![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)

###

###
## RP/EVENT

###

###
<div style="display:flex; flex-direction: column;">

<div>
    <table>
        <th>|DATA/TIME|</th>        
        <th>|EVENT/RP|</th>        
        <th>|CONTRIBUTOR|</th>
        <tr>
            <td>2026/6/22|12:26[CHINA]</td>
            <td>First time submitting to the repository</td>
            <td><a href="https://github.com/chromes-air">chromes-air</a>  
        </tr>
        <tr> <td>2026/6/24|1:04[CHINA]</td>
            <td>Changed ParserUrl parsing to use a state machine instead of brute-force parsing, fixed pointer errors in IPv6 parsing encapsulation, and renamed the CMake project to 'stagdeer-matsc' with support for 'async_read_until/async_read'.</td>
            <td><a href="https://github.com/chromes-air">chromes-air</a>
            </td>
        </tr>
        <tr> <td>2026/6/24|11:33[CHINA]</td>
            <td>Changed the error callback parameters for more consistent engineering, and renamed struct member variables so they no longer break the single responsibility principle.</td>
            <td><a href="https://github.com/chromes-air">chromes-air</a>
            </td>
        </tr>
        <tr> <td>2026/7/1|11:19[CHINA]</td>
            <td>
                Supported OpenSSL connections and added initialization and wait macros for 'Thread'
            </td>
            <td><a href="https://github.com/chromes-air">chromes-air</a>
            </td>
        </tr>
        <tr> <td>2026/7/5|10:43[CHINA]</td>
            <td>
                Mainly added support for 'async_write_tls', changed the SFINAE split, and corrected the API name from 'stl' to 'tls'
            </td>
            <td><a href="https://github.com/chromes-air">chromes-air</a>
            </td>
        </tr>
    </table>
</div>

<hr>

###
## UPDATE

<div>
    <table>
        <th>UPDATE/TIME</th>
        <th>UPDATE/ERROR/FUNCTION</th>
        <th>UPDATE/FILE/LINE</th>
        <th>CONTRIBUTORS</th>
        <div>
            <tr>
                <td>2026/6/22|12:26[CHINA]</td>
                <td>Support for LINUX TCP connection/write/template generation/simple HTTPS/HTTP URL parsing will be supported tomorrow, and SSL handling will be organized after reading.</td>
                <td>TCP/socket_tcp.hpp</td>
                <td><a href="https://github.com/chromes-air">chromes-air</a></td>
            </tr>
            <tr>
                <td>2026/6/24|1:04[CHINA]</td>
                <td>Rewrote URLPARSER to use a state machine for parsing, supporting both query and normal URLs. Updated 'async_read_until/async_read' to implement basic TCP operations. Planning to handle 'Chunked' later, focusing on the 'Exml' project this week to support XML parsing, which will be helpful later. After next week, the main focus will be on implementing SSL connection reading.</td>
                <td>TCP/socket_tcp.hpp/Ipv4Addrs/Ipv6Addrs/Urlparser</td>
                <td><a href="https://github.com/chromes-air">chromes-air</a></td>
            </tr>
            <tr>
                <td>2026/6/24|11:31[CHINA]</td>
                <td>
                Refactored the low-level read logic, changing 'recv == 0' from being treated as a failure to a success since that's normal server behavior when there's no data to send. I also renamed 'client_addrs' to 'client_context' because it feels more straightforward and reasonable; 'addr' is for memory addresses, while 'context' means the context, so it fits the single-responsibility idea better. Plus, I changed the error callback notifications from 'int,string' to a unified 'std::error' type, making error info clearer and more professional. I tried parsing XML today—oh man, that's really tough—so I plan to first add 'SSL' support for Mtasc before diving into the XML parsing side of this project.
                </td>
                <td>TCP/socket_tcp.hpp/Ipv4Addrs/Ipv6Addrs/Urlparser</td>
                <td><a href="https://github.com/chromes-air">chromes-air</a></td>
            </tr>
               <tr>
                <td>2026/7/1|11:19[CHINA]</td>
                <td>
                    The OpenSSL connection function 'async_try_connect_stl' works for connecting to OpenSSL websites, but because different OpenSSL versions are kinda tricky, some site setups are annoying. For example, with the 'www' prefix, 'stl_v1.2' can connect, but without 'www', 'stl_v1.2' throws an error, and weirdly, there's no error message at all. This kind of thing really frustrates me, and I'm still looking for a solution,Besides, I added macros for 'Thread', like initialization and waiting.
                </td>
                <td>SSL/socket_ssl.hpp/Thread/Thread.h</td>
                <td><a href="https://github.com/chromes-air">chromes-air</a></td>
            </tr>
              <tr>
                <td>2026/7/5|10:46[CHINA]</td>
                <td>
                  Supports OpenSSL's 'async_write_tls' and fixes the spelling mistake from 'stl' to 'tls' in the TLS version enabling logic bug, as well as the SFINAE writing style. Please be patient for the next submission.
                </td>
                <td>SSL/socket_ssl.hpp/Thread/Thread.h</td>
                <td><a href="https://github.com/chromes-air">chromes-air</a></td>
            </tr>
        </div>
    </table>
</div>

##
## Next target

<span>
Supports 'async_stl_write' function method , becoming a secure client framework
</span>

##

###

### It's not recommended to use it right now, but you can check out the code and submit a PR to make it more stable .

## UPDATE STL CONNECT AND WRITE EXAMPLE CODE (Complicated)

###
I'll work on the documentation after the TCP/TLS code stabilizes.
###

```cpp
#include "../include/client/socket/SSL/socket_ssl.hpp"
#include "../include/thread/thread.h"
#include "../include/client/clientapp/clientTool.hpp"
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>
#include <utility>

int main (int argc , char* argv[]) {
    if (argc < 2) {
        printf("Invalid paramter\n");
        return -1;
    }
    CLIENT_THREAD_INIT(4);
    stagdeer::client::socketTcpPtrT TcpPtr = std::make_shared<stagdeer::client::socketTcp>
    (argv[1]  , 443 , "NULL");
    stagdeer::client::socketTcp::client_context client_ctx = TcpPtr->getClientContext();
    client_ctx.M_is_enable_ipV6 = true;
    TcpPtr->async_resolver_domain(std::move(
        [TcpPtr = std::move(TcpPtr) , argv](const std::error_code& ec , 
            struct stagdeer::client::socketTcp::client_context&& client_ctx) mutable {
            if (ec) {
                std::cerr << "Resolver domain failed: " << ec.message() << std::endl;
                return;
            }
            TcpPtr->async_try_connect_tcp(std::move([argv](const std::error_code& ec , 
                struct stagdeer::client::socketTcp::client_context&& client_ctx_){
                    if (ec) {
                        std::cerr << "Connect failed: " << ec.message() << std::endl;
                        return;
                    }
                    
                    std::cerr << "TCP handshake to " << argv[1] << " success" << std::endl;

                    stagdeer::client::socketTlsPtrT tlsPtr = 
                        std::make_shared<stagdeer::client::socketSSL>();
                        
                        struct stagdeer::client::socketSSL::openssl_options tls_options;
                        tls_options.enable_tls_v1 = true;

                        tlsPtr->async_create_tls(std::move([tlsPtr = std::move(tlsPtr) , argv]
                        (const std::error_code& ec , 
                            struct stagdeer::client::socketSSL::openssl_options&& tls_options_, 
                                struct stagdeer::client::socketTcp::client_context&& client_context__
                            ) mutable {
                                if (ec) {
                                    std::cerr << "Ceeate tls failed: " << ec.message() << std::endl;
                                    return;
                                }
                                
                                stagdeer::client::clientToolPtr client_tool_ptr = stagdeer::client::clientTool::newClientTool();
                                std::string httpv1_tmp = client_tool_ptr->syncCreateHttpv1template(argv[1],
                                     "/" , "NULL", stagdeer::httpMethod::GET,
                                      {{"Content-type" , "application/json"
                                    }
                                });

                                tlsPtr->async_try_connect_tls(std::move([argv , tlsPtr , httpv1_tmp](const std::error_code& ec , 
                                    stagdeer::client::socketTcp::client_context&& client_context___){
                                        if (ec) {
                                            std::cerr << "TLS handshake failed: " << ec.message() << std::endl;
                                            return;
                                        }
                                        std::cerr << "TLS handshake to " << std::string(argv[1]) << " success" << std::endl;
                                        tlsPtr->async_write_tls(std::move([httpv1_tmp = std::move(httpv1_tmp)](const std::error_code& ec , 
                                            struct stagdeer::client::socketTcp::client_context&& write_client_context , size_t write_bytes){
                                                if (ec) {
                                                    std::cerr << "Write message failed: " << ec.message() << std::endl;
                                                    return;
                                                }
                                                std::cerr << "Success write " << write_bytes << " Message" << std::endl;
                                                return;
                                            }), 
                                        std::move(client_context___), std::move(httpv1_tmp));
                                    }), std::move(client_context__));
                            }), 
                            std::move(tls_options), std::move(client_ctx_)
                        );
                }), std::move(client_ctx));
        }), std::move(client_ctx)
    );    
}
```
###
