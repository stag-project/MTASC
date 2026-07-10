## MTASC - CLIENT - Framework / CC 20

### 

<div style="flex-direction: row; gap: 5px; justify-content: center; margin: 5px;">
    <span>
         Asynchronous client framework, users build client libraries relying on lightweight POSIX-Socket,
          OpenSSL, and C++ Thread. Since it’s aimed at 'client' 
          development, Epoll/Iocp are not used and supports IPv4/IPv6.
          <hr>
    </span>
    <img style="width: 100%; height:300px;" src="github_assets/mtasc-logo.png"></img>
</div>

![License](https://img.shields.io/badge/License-Apache2.0-blue.svg)
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
                Mainly added support for 'async_write_tls', changed the SFINAE split, and corrected the API name from 'tls' to 'tls'
            </td>
            <td><a href="https://github.com/chromes-air">chromes-air</a>
            </td>
        </tr>
        <tr> <td>2026/7/10|6:58[CHINA]</td>
            <td>
                Mainly added support for 'async_read_until_tls', and also changed the misspelling of SFINAE's 'lamdba' back to 'lambda' and completely fixed the spelling mistakes in TLS, and add a license statement
            </td>
            <td><a href="https://github.com/chromes-air">chromes-air</a>
            </td>
        </tr>
    </table>
</div>

<hr>

##
## Next target

<span>
Supports 'async_read_tls' function method , becoming a secure client framework
</span>

##

###

### It's not recommended to use it right now, but you can check out the code and submit a PR to make it more stable .

## UPDATE TLS CONNECT AND WRITE EXAMPLE CODE (Complicated)

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

int main () {
    CLIENT_THREAD_INIT(4);
    stagdeer::client::socketTcpPtrT TcpPtr = std::make_shared<stagdeer::client::socketTcp>
    ("www.baidu.com"  , 443 , "NULL");
    stagdeer::client::socketTcp::client_context client_ctx = TcpPtr->getClientContext();
    client_ctx.M_timeout =  5000000;
    client_ctx.M_is_enable_ipV6 = true;
    TcpPtr->async_resolver_domain(std::move(
        [TcpPtr = std::move(TcpPtr)](const std::error_code& ec , 
            struct stagdeer::client::socketTcp::client_context&& client_ctx) mutable {
            if (ec) {
                std::cerr << "Resolver domain failed: " << ec.message() << std::endl;
                return;
            }
            TcpPtr->async_try_connect_tcp(std::move([](const std::error_code& ec , 
                struct stagdeer::client::socketTcp::client_context&& client_ctx_){
                    if (ec) {
                        std::cerr << "Connect failed: " << ec.message() << std::endl;
                        return;
                    }
                    
                    std::cerr << "TCP handshake to " << "www.baidu.com" << " success" << std::endl;

                    stagdeer::client::socketTlsPtrT tlsPtr = 
                        std::make_shared<stagdeer::client::socketSSL>();
                        
                        struct stagdeer::client::socketSSL::openssl_options tls_options;
                        tls_options.enable_tls_v1 = true;

                        tlsPtr->async_create_tls(std::move([tlsPtr = std::move(tlsPtr)]
                        (const std::error_code& ec , 
                            struct stagdeer::client::socketSSL::openssl_options&& tls_options_, 
                                struct stagdeer::client::socketTcp::client_context&& client_context__
                            ) mutable {
                                if (ec) {
                                    std::cerr << "Cate tls failed: " << ec.message() << std::endl;
                                    return;
                                }
                                
                                stagdeer::client::clientToolPtr client_tool_ptr = stagdeer::client::clientTool::newClientTool();
                                std::string httpv1_tmp = client_tool_ptr->syncCreateHttpv1template("www.baidu.com",
                                     "/" , "NULL", stagdeer::httpMethod::GET,
                                      {{"Content-type" , "application/json"
                                    }
                                });
                                
                                std::cerr << "\n" << httpv1_tmp;

                                tlsPtr->async_try_connect_tls(std::move([tlsPtr , httpv1_tmp](const std::error_code& ec , 
                                    stagdeer::client::socketTcp::client_context&& client_context___){
                                        if (ec) {
                                            std::cerr << "TLS handshake failed: " << ec.message() << std::endl;
                                            return;
                                        }
                                        std::cerr << "TLS handshake to " << std::string("www.baidu.com") << " success" << std::endl;
                                        tlsPtr->async_write_tls(std::move([httpv1_tmp = std::move(httpv1_tmp) , tlsPtr = std::move(tlsPtr)](const std::error_code& ec , 
                                            struct stagdeer::client::socketTcp::client_context&& write_client_context , size_t write_bytes){
                                                if (ec) {
                                                    std::cerr << "Write message failed: " << ec.message() << std::endl;
                                                    return;
                                                }
                                                std::cerr << "Success write " << write_bytes << " Message" << std::endl;
                                                //TLS READ TEST
                                                tlsPtr->async_read_until_tls([](const std::error_code& ec , size_t read_until_bytes , 
                                                    std::shared_ptr<stagdeer::client::readBuffer>&& read_buffer_ptr 
                                                    , struct stagdeer::client::socketTcp::client_context&& read_until_context) mutable {
                                                        if (ec) {
                                                            std::cerr << "Read message falid: " << ec.message() << std::endl;
                                                            return;
                                                        }
                                                        std::cerr << "Read message : " << read_until_bytes << " Bytes" << std::endl;
                                                        std::cerr << "Message: " << std::endl;
                                                        std::cerr << read_buffer_ptr->peekData() << std::endl;
                                                        return;
                                                }, std::move(write_client_context), std::string("\r\n\r\n"));
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
