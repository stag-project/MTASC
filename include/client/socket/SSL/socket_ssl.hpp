#ifndef STAGDEER_CLIENT_SSL_SOCKET
#define STAGDEER_CLIENT_SSL_SOCKET

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "../../util/type_util.hpp"
#include "../TCP/socket_tcp.hpp"
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace stagdeer {
    namespace client {
        class socketSSL : public std::enable_shared_from_this<socketSSL> {

            public:

            struct openssl_options {
                bool enable_custom_cert = false;
                bool enable_client_key = false;
            };

            void load_custom_CA_cert(const std::string& M_cert_path) {
                if (M_openssl_configure.M_CA_cert_path.empty()) {
                    M_openssl_configure.M_CA_cert_path = M_cert_path;
                    return;
                }
                return;
            }

            void set_stl_verify_depth(int M_number) {
                
            }
        
            void init_client_openssl() {
                if (!M_openssl_configure.M_openssl_init) {
                    M_openssl_configure.M_openssl_init = true;
                    M_init_openssl();
                }
                return;
            }

            template<typename Tp>
            inline typename stagdeer::util::lamdba_trais::constraint<
                stagdeer::util::lamdba_trais::M_is_retTp<
                    typename stagdeer::util::lamdba_trais::M_get_lamdba_ret_Tp<
                        Tp, const std::error_code& , 
                            stagdeer::client::socketSSL::openssl_options&&,
                            stagdeer::client::socketTcp::client_context&&
                    >::__M_ret_lmdba, void
                >::__is_M_ret_Tp
            >::type
            async_create_stl(
                Tp&& callback_token,
                struct openssl_options&& M_ssl_options,
                struct stagdeer::client::socketTcp::client_context&& M_client_context
            ) noexcept(
                noexcept(
                    callback_token(
                        std::declval<const std::error_code&>(),
                        std::declval<stagdeer::client::socketTcp::client_context&&>(),
                        std::declval<stagdeer::client::socketSSL::openssl_options&&>()
                    )
                )
            ) {
                if (!M_openssl_configure.M_openssl_init) {
                    std::error_code ec = std::make_error_code(std::errc::bad_file_descriptor);
                    M_thread_manager.getThreadManager()
                        .asyncTaskvoid(std::move(callback_token), ec , 
                            std::move(M_ssl_options) , std::move(M_client_context)
                        );
                    return -1;
                }
                M_thread_manager.getThreadManager() 
                    .asyncTaskvoid([self = shared_from_this(), 
                        callback_token = std::function<void(
                            const std::error_code&,
                            stagdeer::client::socketSSL::openssl_options&&,
                            stagdeer::client::socketTcp::client_context&&
                        )>(std::move(callback_token)),
                         M_ssl_options = std::move(M_ssl_options) ,
                          M_client_context = std::move(M_client_context)]
                        () mutable {
                        //TRY CREATE STL CONTEXT
                            self -> M_openssl_configure.M_openssl_ctx_ptr = 
                                self -> M_create_stl_context(M_ssl_options);
                                if (!self -> M_openssl_configure.M_openssl_ctx_ptr) {
                                    //CREATE FAILED
                                    std::error_code ec(errno , std::system_category());
                                    self -> M_thread_manager.getThreadManager()
                                        .asyncTaskvoid(std::move(callback_token),
                                        ec , std::move(M_ssl_options) , std::move(M_client_context)
                                    );
                                    return -1;
                                }
                            //TRY CREATE STL PTR
                            self -> M_openssl_configure.M_openssl_ptr = 
                                self -> M_create_stl(self -> M_openssl_configure.M_openssl_ctx_ptr,
                                     M_client_context.M_socketfd);
                                if (!self -> M_openssl_configure.M_openssl_ctx_ptr) {
                                    //CREATE FAILED
                                    std::error_code ec(errno , std::system_category());
                                    self -> M_thread_manager.getThreadManager()
                                        .asyncTaskvoid(std::move(callback_token), ec ,
                                         std::move(M_ssl_options) , std::move(M_client_context)
                                        );
                                    return -1;
                                }
                            std::error_code ec;
                            self->M_thread_manager.getThreadManager()
                                .asyncTaskvoid(std::move(callback_token),ec , 
                                    std::move(M_ssl_options) , std::move(M_client_context)
                                );
                            return 1;
                        }
                );
            }

            template<typename Tp>
            inline typename stagdeer::util::lamdba_trais::constraint<
                stagdeer::util::lamdba_trais::M_is_retTp<
                    typename stagdeer::util::lamdba_trais::M_get_lamdba_ret_Tp<
                    Tp, const std::error_code&,
                        stagdeer::client::socketTcp::client_context&&
                    >::__M_ret_lmdba, void
                >::__is_M_ret_Tp
            >::type
            async_try_connect_stl(
                Tp&& callback_token,
                struct stagdeer::client::socketTcp::client_context&& M_client_context
            ) noexcept(
                noexcept(callback_token(
                    std::declval<const std::error_code&>(),
                    std::declval<stagdeer::client::socketTcp::client_context&&>()
                ))
            ) {
                if (M_client_context.M_socketfd < 0 || !M_openssl_configure.M_openssl_ptr) {
                    std::error_code ec = std::make_error_code(std::errc::bad_file_descriptor);
                    M_thread_manager.getThreadManager()
                        .asyncTaskvoid(std::move(callback_token), ec , 
                        std::move(M_client_context)
                    );
                    return -1;
                }

                M_thread_manager.getThreadManager()
                    .asyncTaskvoid([
                        self = shared_from_this(),
                        callback_token = std::function<void(
                            const std::error_code& ,
                            struct stagdeer::client::socketTcp::client_context&&
                        )>(std::move(callback_token)),
                        M_client_context = std::move(M_client_context)
                    ]() mutable {
                        int M_stl_connect_ret = SSL_connect(self -> 
                                M_openssl_configure.M_openssl_ptr);
                            if (M_stl_connect_ret < 0) {
                                //VERIFY HANDSHAKE

                            } else if (M_stl_connect_ret == 0) {
                                //STL HANDSHAKE FAILED
                                int M_handshake_err = SSL_get_error(self -> M_openssl_configure.M_openssl_ptr,
                                     M_stl_connect_ret);
                                    struct stagdeer::client::socketTcp::client_context M_stl_retry_connect = self -> 
                                        M_ssl_handshake_retry(std::move(M_client_context) , M_handshake_err);
                                        if (M_stl_retry_connect.M_is_retry_success) {
                                            //STL HANDSHAKE RETRY SUCCESS
                                            std::error_code ec;
                                            self -> M_thread_manager.getThreadManager()
                                                .asyncTaskvoid(std::move(callback_token), ec ,
                                                 std::move(M_client_context)
                                            );
                                            return 2;
                                        } else {
                                        //RETRY HANDSHAKE RETRY FAILED
                                            
                                        }
                            }
                            //STL HANDSHAKE SUCCESS
                    }
                );
            }

            private:

            stagdeer::THREAD M_thread_manager;

            SSL* M_create_stl(SSL_CTX* M_ssl_context , M_SOCKET_TP M_fd) {
                if (!M_ssl_context) {
                    return nullptr;
                }
                SSL* M_openssl_ptr = SSL_new(M_ssl_context);
                SSL_set_fd(M_openssl_ptr, M_fd);
                return M_openssl_ptr;
            }

            SSL_CTX* M_create_stl_context(struct openssl_options M_stl_options) {
                SSL_CTX* M_openssl_ctx_ptr = SSL_CTX_new(TLS_client_method());
                SSL_CTX_set_verify(M_openssl_ctx_ptr, SSL_VERIFY_PEER, nullptr);
                if (M_stl_options.enable_custom_cert) {
                    //USE CUSTOM CA CERT
                    if (SSL_CTX_load_verify_file(M_openssl_ctx_ptr ,
                         M_openssl_configure.M_CA_cert_path.c_str()) != 1) {
                        ERR_print_errors_fp(stderr);
                        SSL_CTX_free(M_openssl_ctx_ptr);
                        return nullptr;
                    }
                } else {
                    //USE SYSTEMS DEFAULT CA CERT
                    if (SSL_CTX_set_default_verify_paths(M_openssl_ctx_ptr) != 1) {
                        ERR_print_errors_fp(stderr);
                        SSL_CTX_free(M_openssl_ctx_ptr);
                        return nullptr;   
                    }
                }
                SSL_CTX_set_verify_depth(M_openssl_ctx_ptr,M_openssl_configure.M_stl_verify_depth);
                return M_openssl_ctx_ptr;
            }


            struct client_ssl_configure {
                bool M_openssl_init = false;
                std::string M_CA_cert_path;
                SSL* M_openssl_ptr = nullptr;
                SSL_CTX* M_openssl_ctx_ptr = nullptr;
                int M_stl_verify_depth = 4;

                ~client_ssl_configure() {
                    if (M_openssl_ctx_ptr && M_openssl_ptr) {
                        free(M_openssl_ptr);
                        SSL_CTX_free(M_openssl_ctx_ptr);
                    }
                    return;
                }
            };

            struct client_ssl_configure M_openssl_configure;

            std::string M_get_stl_err() {
                
            }

            void M_init_openssl() {
                SSL_library_init();
                OpenSSL_add_all_algorithms();
                SSL_load_error_strings();
            }
            
            struct stagdeer::client::socketTcp::client_context M_ssl_handshake_retry
                (struct stagdeer::client::socketTcp::client_context client_ctx , int M_handshake_err) {
                    if (client_ctx.M_socketfd < 0) {
                        throw std::runtime_error("Invalid socket!");
                    }
                    
                }
        };
    }
}

#endif