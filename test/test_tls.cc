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
    stagdeer::client::socketTcpSocket tcpSocket = std::make_shared<stagdeer::client::socketTcp>
    ("www.baidu.com"  , 443 , "NULL");
    stagdeer::client::socketTcp::client_context client_ctx = tcpSocket->getClientContext();
    client_ctx.M_timeout =  5000000;
    client_ctx.M_is_enable_ipV6 = true;
    tcpSocket->async_resolver_domain(std::move(
        [tcpSocket = std::move(tcpSocket)](const std::error_code& ec , 
            struct stagdeer::client::socketTcp::client_context&& client_ctx) mutable {
            if (ec) {
                std::cerr << "Resolver domain failed: " << ec.message() << std::endl;
                return;
            }
            tcpSocket->async_try_connect_tcp(std::move([](const std::error_code& ec , 
                struct stagdeer::client::socketTcp::client_context&& client_ctx_){
                    if (ec) {
                        std::cerr << "Connect failed: " << ec.message() << std::endl;
                        return;
                    }
                    
                    std::cerr << "TCP handshake to " << "www.baidu.com" << " success" << std::endl;

                    stagdeer::client::socketTlsSocket tlsSocket = 
                        std::make_shared<stagdeer::client::socketSSL>();
                        
                        struct stagdeer::client::socketSSL::openssl_options tls_options;
                        tls_options.enable_tls_v1 = true;

                        tlsSocket->async_create_tls(std::move([tlsSocket = std::move(tlsSocket)]
                        (const std::error_code& ec , 
                            struct stagdeer::client::socketSSL::openssl_options&& tls_options_, 
                                struct stagdeer::client::socketTcp::client_context&& client_context__
                            ) mutable {
                                if (ec) {
                                    std::cerr << "Ceate tls socket failed: " << ec.message() << std::endl;
                                    return;
                                }
                                
                                stagdeer::client::clientToolPtr client_tool_ptr = stagdeer::client::clientTool::newClientTool();
                                std::string httpv1_tmp = client_tool_ptr->syncCreateHttpv1template("www.baidu.com",
                                     "/" , "NULL", stagdeer::httpMethod::GET,
                                      {{"Content-type" , "application/json"
                                    }
                                });
                                
                                std::cerr << "\n" << httpv1_tmp;

                                tlsSocket->async_try_connect_tls(std::move([tlsSocket , httpv1_tmp](const std::error_code& ec , 
                                    stagdeer::client::socketTcp::client_context&& client_context___){
                                        if (ec) {
                                            std::cerr << "TLS handshake failed: " << ec.message() << std::endl;
                                            return;
                                        }
                                        std::cerr << "TLS handshake to " << std::string("www.baidu.com") << " success" << std::endl;
                                        tlsSocket->async_write_tls(std::move([httpv1_tmp = std::move(httpv1_tmp) , 
                                            tlsSocket = std::move(tlsSocket)](const std::error_code& ec , 
                                            struct stagdeer::client::socketTcp::client_context&& write_client_context , size_t write_bytes){
                                                if (ec) {
                                                    std::cerr << "Write message failed: " << ec.message() << std::endl;
                                                    return;
                                                }
                                                std::cerr << "Success write " << write_bytes << " Message" << std::endl;
                                                //TLS READ TEST
                                                tlsSocket->async_read_until_tls([](const std::error_code& ec , size_t read_until_bytes , 
                                                    std::shared_ptr<stagdeer::client::readBuffer>&& read_buffer_ptr 
                                                    , struct stagdeer::client::socketTcp::client_context&& read_until_context) mutable {
                                                        if (ec) {
                                                            std::cerr << "Read message falid: " << ec.message() << std::endl;
                                                            return;
                                                        }
                                                        std::cerr << "Read message : " << read_until_bytes << " Bytes" << std::endl;
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