#include "../include/client/socket/SSL/socket_ssl.hpp"
#include "../include/thread/thread.h"
#include <cstdio>
#include <iostream>
#include <memory>
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
                    std::shared_ptr<stagdeer::client::socketSSL> stlPtr = 
                        std::make_shared<stagdeer::client::socketSSL>();
                        
                        struct stagdeer::client::socketSSL::openssl_options stl_options;
                        stl_options.enable_tls_v1 = true;

                        stlPtr->async_create_stl(std::move([stlPtr = std::move(stlPtr) , argv]
                        (const std::error_code& ec , 
                            struct stagdeer::client::socketSSL::openssl_options&& stl_options_, 
                                struct stagdeer::client::socketTcp::client_context&& client_context__
                            ) mutable {
                                if (ec) {
                                    std::cerr << "Ceeate stl failed: " << ec.message() << std::endl;
                                    return;
                                }
                                stlPtr->async_try_connect_stl(std::move([argv](const std::error_code& ec , 
                                    stagdeer::client::socketTcp::client_context&& client_context___){
                                        if (ec) {
                                            std::cerr << "STL handshake failed: " << ec.message() << std::endl;
                                            return;
                                        }
                                        std::cerr << "STL handshake success to " << std::string(argv[1]) << std::endl;
                                        return;
                                    }), std::move(client_context__));
                            }), 
                            std::move(stl_options), std::move(client_ctx_)
                        );
                }), std::move(client_ctx));
        }), std::move(client_ctx)
    );    
}