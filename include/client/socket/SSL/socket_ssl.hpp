#ifndef STAGDEER_CLIENT_SSL_SOCKET
#define STAGDEER_CLIENT_SSL_SOCKET

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <openssl/bio.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "../../util/type_util.hpp"
#include "../../socket/socket_header.h"
#include "../TCP/socket_tcp.hpp"
#include <openssl/tls1.h>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace stagdeer {
    namespace client {
        class socketSSL : public std::enable_shared_from_this<socketSSL> {

            public:

            socketSSL() = default;
            socketSSL(stagdeer::client::socketSSL&& M_other_ssl_object)
                :M_openssl_configure(std::move(M_other_ssl_object.M_openssl_configure)){}
                socketSSL& operator=(stagdeer::client::socketSSL&& M_other_ssl_object_operator){
                    if (this != &M_other_ssl_object_operator) {
                        M_openssl_configure = std::move(M_other_ssl_object_operator.M_openssl_configure);
                    }
                    return *this;
                }
            socketSSL(const stagdeer::client::socketSSL&) = delete;
            socketSSL& operator=(const stagdeer::client::socketSSL&) = delete;

            ~socketSSL() {
                if (M_openssl_configure.M_openssl_ctx_ptr && M_openssl_configure.M_openssl_ctx_ptr) {
                    free(M_openssl_configure.M_openssl_ctx_ptr);
                    SSL_CTX_free(M_openssl_configure.M_openssl_ctx_ptr);
                }
                return;
            }
            
            struct openssl_options {
                bool enable_custom_cert = false;
                bool enable_client_key = false;
                bool enable_tls_v1 = false;
            };

            void load_custom_CA_cert(const std::string& M_cert_path) {
                if (M_openssl_configure.M_CA_cert_path.empty()) {
                    M_openssl_configure.M_CA_cert_path = M_cert_path;
                    return;
                }
                return;
            }

            void set_stl_verify_depth(int M_number) {
                M_openssl_configure.M_stl_verify_depth = M_number;
                return;
            }
        
            void init_client_openssl() {
                if (!M_openssl_configure.M_openssl_init) {
                    M_openssl_configure.M_openssl_init = true;
                    M_init_openssl();
                }
                return;
            }

            template<typename Tp>
            auto async_create_tls(
                Tp&& callback_token,
                struct openssl_options&& M_ssl_options,
                struct stagdeer::client::socketTcp::client_context&& M_client_context
            ) noexcept(
                noexcept(
                    callback_token(
                        std::declval<const std::error_code&>(),
                        std::declval<stagdeer::client::socketSSL::openssl_options&&>(),
                        std::declval<stagdeer::client::socketTcp::client_context&&>()
                    )
                )
            ) -> decltype(
                std::declval<Tp>()(
                    std::declval<const std::error_code&>(),
                    std::declval<stagdeer::client::socketSSL::openssl_options&&>(),
                    std::declval<stagdeer::client::socketTcp::client_context&&>()
                ),
                typename stagdeer::util::lamdba_trais::constraint<
                    stagdeer::util::lamdba_trais::M_is_retTp<
                        typename stagdeer::util::lamdba_trais::M_get_lamdba_ret_Tp<
                            Tp, const std::error_code& , 
                                struct stagdeer::client::socketSSL::openssl_options&&,
                                struct stagdeer::client::socketTcp::client_context&&
                        >::__M_ret_lmdba, void
                    >::__is_M_ret_Tp
                >::type{}
            ) {
                init_client_openssl();
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
                                    return;
                                }
                            //TRY CREATE STL PTR
                            self -> M_openssl_configure.M_openssl_ptr = 
                                self -> M_create_stl(self -> M_openssl_configure.M_openssl_ctx_ptr,
                                     M_client_context.M_socketfd , M_client_context.M_addrs_host);
                                if (!self -> M_openssl_configure.M_openssl_ctx_ptr) {
                                    //CREATE FAILED
                                    std::error_code ec(errno , std::system_category());
                                    self -> M_thread_manager.getThreadManager()
                                        .asyncTaskvoid(std::move(callback_token), ec ,
                                         std::move(M_ssl_options) , std::move(M_client_context)
                                        );
                                    return;
                                }
                            std::error_code ec;
                            self->M_thread_manager.getThreadManager()
                                .asyncTaskvoid(std::move(callback_token),ec , 
                                    std::move(M_ssl_options) , std::move(M_client_context)
                                );
                            return;
                        }
                );
                return -1;
            }

            template<typename Tp>
            auto async_try_connect_tls(
                Tp&& callback_token,
                struct stagdeer::client::socketTcp::client_context&& M_client_context
            ) noexcept(
                noexcept(
                    callback_token(
                        std::declval<const std::error_code&>(),
                        std::declval<stagdeer::client::socketTcp::client_context&&>()
                    )
                )
            ) -> decltype(
                std::declval<Tp>()(
                    std::declval<const std::error_code&>(),
                    std::declval<stagdeer::client::socketTcp::client_context&&>()
                ),
                typename stagdeer::util::lamdba_trais::constraint<
                    stagdeer::util::lamdba_trais::M_is_retTp<
                        typename stagdeer::util::lamdba_trais::M_get_lamdba_ret_Tp<
                        Tp, const std::error_code&,
                            stagdeer::client::socketTcp::client_context&&
                        >::__M_ret_lmdba, void                                                                                                                       
                    >::__is_M_ret_Tp
                >::type{}
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
                            if (M_stl_connect_ret == 1) {
                                //CONNECT SUCCESS
                                std::error_code ec;
                                self -> M_thread_manager.getThreadManager()
                                    .asyncTaskvoid(std::move(callback_token), ec , 
                                        std::move(M_client_context)
                                );
                                return;
                            } else if (M_stl_connect_ret < 1 || M_stl_connect_ret <= 0) {
                                //STL HANDSHAKE FAILED
                                int M_handshake_err = SSL_get_error(self -> M_openssl_configure.M_openssl_ptr,
                                     M_stl_connect_ret);
                                     if (M_handshake_err == 0 || M_handshake_err == SSL_ERROR_WANT_READ 
                                        || M_handshake_err == SSL_ERROR_WANT_WRITE) {
                                        fd_set M_set_timeout;
                                        FD_ZERO(&M_set_timeout);
                                        FD_SET(M_client_context.M_socketfd, &M_set_timeout);
                                        timeval M_tv;
                                        M_tv.tv_sec = M_client_context.M_timeout;
                                        M_tv.tv_usec = 0;

                                        int M_wait_ret = select(M_client_context.M_socketfd + 1, &M_set_timeout, 
                                            nullptr, nullptr, &M_tv);
                                            if (M_wait_ret < 0) {
                                                //WAIT TIMEOUT
                                                std::error_code ec = std::make_error_code(std::errc::timed_out);
                                                self -> M_thread_manager.getThreadManager()
                                                    .asyncTaskvoid(std::move(callback_token), ec ,
                                                     std::move(M_client_context)
                                                );
                                                return;
                                            }
                                        
                                        //WAIT SUCCESS
                                     }
                                    struct stagdeer::client::socketTcp::client_context M_stl_retry_connect = self -> 
                                        M_ssl_handshake_retry(std::move(M_client_context) , M_handshake_err , 
                                        self -> M_openssl_configure.M_openssl_ptr ,
                                         self -> M_openssl_configure.M_openssl_ctx_ptr
                                        );
                                        if (M_stl_retry_connect.M_is_retry_success) {
                                            //STL HANDSHAKE RETRY SUCCESS
                                            std::error_code ec;
                                            M_client_context = std::move(M_stl_retry_connect);
                                            self -> M_thread_manager.getThreadManager()
                                                .asyncTaskvoid(std::move(callback_token), ec ,
                                                 std::move(M_client_context)
                                            );
                                            return;
                                        } else {
                                        //RETRY HANDSHAKE RETRY FAILED
                                            M_client_context = std::move(M_stl_retry_connect);
                                            std::error_code ec(errno , std::system_category());
                                            self -> M_thread_manager.getThreadManager()   
                                                .asyncTaskvoid(std::move(callback_token),
                                                    ec , std::move(M_client_context)
                                            );
                                            printf("Retry stl hanshake to %s\n" ,
                                                 std::string(M_client_context.M_addrs_host) + " falied");
                                            return;
                                        }
                            } else {
                                std::error_code ec = std::make_error_code(std::errc::connection_reset);
                                self -> M_thread_manager.getThreadManager()
                                    .asyncTaskvoid(std::move(callback_token),
                                        ec , std::move(M_client_context)
                                    );
                                printf("STL ERROR: %s\n" , self->M_get_stl_err());
                                return;
                            }
                        return;
                    }
                );
                return -1;
            }

            template<typename Tp>
            auto async_write_tls(
                Tp&& callback_token,
                struct stagdeer::client::socketTcp::client_context&& M_context__,
                const std::string& M_message__
            ) noexcept(
                noexcept(
                    callback_token(
                        std::declval<const std::error_code&>(),
                        std::declval<struct stagdeer::client::socketTcp::client_context&&>(),
                        std::declval<size_t>()
                    )
                )
            ) -> decltype(
                std::declval<Tp>()(
                    std::declval<const std::error_code&>(),
                    std::declval<struct stagdeer::client::socketTcp::client_context&&>(),
                    std::declval<size_t>()
                ),
                typename stagdeer::util::lamdba_trais::constraint<
                  stagdeer::util::lamdba_trais::M_is_retTp<
                    typename stagdeer::util::lamdba_trais::M_get_lamdba_ret_Tp
                        <Tp, const std::error_code& , 
                            struct stagdeer::client::socketTcp::client_context&&,
                            size_t
                        >::__M_ret_lmdba, void
                    >::__is_M_ret_Tp 
                >::type{}
            ) {
                if (M_context__.M_socketfd < 0) {
                    std::error_code ec = std::make_error_code(std::errc::bad_file_descriptor);
                    M_thread_manager.getThreadManager()
                        .asyncTaskvoid(std::move(callback_token), 
                            ec , std::move(M_context__) ,
                             size_t(0)
                        );
                    return -1;
                }

                if (M_openssl_configure.M_openssl_ptr) {
                    M_context__.M_clientMessage = M_message__;
                    M_context__.M_clientMessageLength = strlen(M_message__.c_str());
                    M_thread_manager.getThreadManager()
                        .asyncTaskvoid([
                            self = shared_from_this(),
                            callback_token = std::function<void(
                                const std::error_code&,
                                struct stagdeer::client::socketTcp::client_context&&,
                                size_t
                            )>(std::move(callback_token)),
                            M_context__ = std::move(M_context__)
                        ]() mutable {
                                //TRY WRITE
                                int M_write_tls_ret = SSL_write(self -> M_openssl_configure.M_openssl_ptr, 
                                    M_context__.M_clientMessage.c_str(), M_context__.M_clientMessageLength
                                );
                                    if (M_write_tls_ret < 0) {
                                        //WRITE FAILED
                                        //TRY RETRY WRITE
                                        struct stagdeer::client::socketTcp::client_context M_retry_write_ret = 
                                            self -> M_ssl_write_retry(std::move(M_context__), self -> M_openssl_configure.M_openssl_ptr, 
                                                self -> M_openssl_configure.M_openssl_ctx_ptr, 
                                                    SSL_get_error(self -> M_openssl_configure.M_openssl_ptr, M_write_tls_ret),
                                                     M_context__.M_clientMessageLength , 
                                                    M_context__.M_clientMessageLength - 
                                                    M_write_tls_ret);
                                                if (M_retry_write_ret.M_is_retry_success) {
                                                    //RETRY SUCCESS
                                                    std::error_code ec;
                                                    self -> M_thread_manager.getThreadManager()
                                                        .asyncTaskvoid(std::move(callback_token),
                                                            ec , std::move(M_retry_write_ret) , size_t(M_retry_write_ret.M_client_retry_write_bytes)
                                                        );
                                                    return;
                                                }
                                            //RETRY FAILED
                                            std::error_code ec = std::make_error_code(std::errc::io_error);
                                            self -> M_thread_manager.getThreadManager()
                                                .asyncTaskvoid(std::move(callback_token), ec , 
                                                    std::move(M_retry_write_ret), size_t(0)
                                                );
                                            printf("SSL ERROR: %s\n" , self -> M_get_stl_err().c_str());
                                            return;
                                    } else if (M_write_tls_ret > 0) {
                                        //WRITE SUCCESS
                                        std::error_code ec;
                                        self->M_thread_manager.getThreadManager()
                                        .asyncTaskvoid(std::move(callback_token),ec,
                                         std::move(M_context__),
                                            size_t(M_write_tls_ret)
                                        );
                                        return;
                                    } else if (M_write_tls_ret == 0) {
                                        //CONNECTION CLOSED
                                        SSL_shutdown(self -> M_openssl_configure.M_openssl_ptr);
                                        SSL_CTX_free(self -> M_openssl_configure.M_openssl_ctx_ptr);
                                        SSL_free(self -> M_openssl_configure.M_openssl_ptr);
                                        std::error_code ec = std::make_error_code(std::errc::connection_reset);
                                        CLOSE_FD(M_context__.M_socketfd);
                                        self -> M_thread_manager.getThreadManager()
                                            .asyncTaskvoid(std::move(callback_token), ec , 
                                                std::move(M_context__) , size_t(0)
                                            );
                                        return;
                                    }
                                    else {
                                        //OTHER ERROR
                                        std::error_code ec = std::make_error_code(std::errc::io_error);
                                        self -> M_thread_manager.getThreadManager()
                                            .asyncTaskvoid(std::move(callback_token), ec , 
                                                std::move(M_context__), size_t(0)
                                            );
                                        printf("SSL ERROR: %s\n" , self -> M_get_stl_err().c_str());
                                        return;
                                    }
                        return;
                    });
                }
                return -1;
            }

            template<typename Tp>
            auto async_read_until_tls(
                Tp&& callback_token,
                struct stagdeer::client::socketTcp::client_context&& M_context_,
                const std::string& M_delimiter
            ) noexcept(
                noexcept(
                    callback_token(
                        std::declval<const std::error_code&>(),
                        std::declval<size_t>(),
                        std::declval<std::shared_ptr<stagdeer::client::readBuffer>&&>(),
                        std::declval<struct stagdeer::client::socketTcp::client_context&&>()
                    )
                )
            ) -> decltype(
                std::declval<Tp>()(
                    std::declval<const std::error_code&>(),
                    std::declval<size_t>(),
                    std::declval<std::shared_ptr<stagdeer::client::readBuffer>&&>(),
                    std::declval<struct stagdeer::client::socketTcp::client_context&&>()
                ),
                typename stagdeer::util::lamdba_trais::constraint<
                    stagdeer::util::lamdba_trais::M_is_retTp<
                        typename stagdeer::util::lamdba_trais::M_get_lamdba_ret_Tp<
                            Tp, const std::error_code& , size_t , 
                            std::shared_ptr<
                                stagdeer::client::readBuffer&&
                            >,
                            struct stagdeer::client::socketTcp::client_context&&
                        >::__M_ret_lmdba, void>::__is_M_ret_Tp
                >::type{}
            ) {
              //CREATE READBUFFER PTR
              std::shared_ptr<stagdeer::client::readBuffer> M_read_buffer_ptr = std::make_shared<stagdeer::client::readBuffer>();
              if (M_context_.M_socketfd < 0 || !M_openssl_configure.M_openssl_ptr || !M_openssl_configure.M_openssl_ctx_ptr) {
                //INVLAID DATA
                std::error_code ec = std::make_error_code(std::errc::bad_file_descriptor);
                M_thread_manager.getThreadManager()
                    .asyncTaskvoid(std::move(callback_token), ec , size_t(0) , 
                        std::move(M_read_buffer_ptr) , std::move(M_context_)
                    );
                return -1;
              }
              M_thread_manager.getThreadManager()
                .asyncTaskvoid(std::move(
                    [self = shared_from_this() , callback_token = std::function<void(
                        const std::error_code& , size_t , std::shared_ptr<stagdeer::client::readBuffer&&>,
                            struct stagdeer::client::socketTcp::client_context&&
                    )>(std::move(callback_token)) , M_context_ = std::move(M_context_),
                    M_delimiter = std::move(M_delimiter)](){
                        
                    }
                ));
            }

            private:

            stagdeer::THREAD& M_thread_manager = stagdeer::THREAD::getInstance();

            SSL* M_create_stl(SSL_CTX* M_ssl_context , M_SOCKET_TP M_fd , const char* M_hostname) {
                if (!M_ssl_context) {
                    return nullptr;
                }
                SSL* M_openssl_ptr = SSL_new(M_ssl_context);
                SSL_set_tlsext_host_name(M_openssl_ptr, M_hostname);
                SSL_set_fd(M_openssl_ptr, M_fd);
                return M_openssl_ptr;
            }

            SSL_CTX* M_create_stl_context(struct openssl_options M_stl_options) {
                SSL_CTX* M_openssl_ctx_ptr = nullptr;
                if (M_stl_options.enable_tls_v1) {
                    M_openssl_ctx_ptr = SSL_CTX_new(TLSv1_2_client_method());
                } else {
                    M_openssl_ctx_ptr = SSL_CTX_new(TLS_client_method());
                }
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
                SSL_CTX_set_tlsext_servername_callback(M_openssl_ctx_ptr, nullptr);
                SSL_CTX_set_verify_depth(M_openssl_ctx_ptr,M_openssl_configure.M_stl_verify_depth);
                return M_openssl_ctx_ptr;
            }

            SSL* M_creater_new_retry_stl(SSL* _M_bad_stl__ , SSL_CTX* _M_ctx_stl__ , int _M_fd__ , const char* _M_hostname__) {
                free(_M_bad_stl__);
                SSL* M_new_ssl = SSL_new(_M_ctx_stl__);
                if (!M_new_ssl) {
                    //INVALID STL
                    return nullptr;
                }
                SSL_set_tlsext_host_name(M_new_ssl , _M_hostname__);
                SSL_set_fd(M_new_ssl, _M_fd__);
                return M_new_ssl;
            }

            struct client_ssl_configure {
                bool M_openssl_init = false;
                std::string M_CA_cert_path;
                SSL* M_openssl_ptr = nullptr;
                SSL_CTX* M_openssl_ctx_ptr = nullptr;
                int M_stl_verify_depth = 4;

                client_ssl_configure() = default;
                
                client_ssl_configure(const struct stagdeer::client::socketSSL::client_ssl_configure& M_client_configure_other_copy)
                    :M_openssl_ctx_ptr(M_client_configure_other_copy.M_openssl_ctx_ptr),
                    M_openssl_ptr(M_client_configure_other_copy.M_openssl_ptr),
                    M_openssl_init(M_client_configure_other_copy.M_openssl_init),
                    M_stl_verify_depth(M_client_configure_other_copy.M_stl_verify_depth),
                    M_CA_cert_path(std::move(M_client_configure_other_copy.M_CA_cert_path)){
                        M_openssl_ctx_ptr = nullptr;
                        M_openssl_ptr = nullptr;
                        M_openssl_init = false;
                        return;
                    }
                        client_ssl_configure& operator=(const struct stagdeer::client::socketSSL::client_ssl_configure&& M_client_configure_other_copy_operator) {
                            if (this != &M_client_configure_other_copy_operator) {
                                M_openssl_ctx_ptr = M_client_configure_other_copy_operator.M_openssl_ctx_ptr;
                                M_openssl_ctx_ptr = nullptr;
                                M_openssl_init = M_client_configure_other_copy_operator.M_openssl_init;
                                M_openssl_init = false;
                                M_openssl_ptr = M_client_configure_other_copy_operator.M_openssl_ptr;
                                M_openssl_ptr = nullptr;
                                M_CA_cert_path = std::move(M_client_configure_other_copy_operator.M_CA_cert_path);
                                M_stl_verify_depth = std::move(M_client_configure_other_copy_operator.M_stl_verify_depth);
                            }
                            return *this;
                        }
                        client_ssl_configure(struct stagdeer::client::socketSSL::client_ssl_configure&& M_client_configure_other_move)
                            :M_openssl_ctx_ptr(M_client_configure_other_move.M_openssl_ctx_ptr),
                            M_openssl_ptr(M_client_configure_other_move.M_openssl_ptr),
                            M_openssl_init(M_client_configure_other_move.M_openssl_init),
                            M_stl_verify_depth(M_client_configure_other_move.M_stl_verify_depth),
                            M_CA_cert_path(std::move(M_client_configure_other_move.M_CA_cert_path)) {
                                M_openssl_ctx_ptr = nullptr;
                                M_openssl_ptr = nullptr;
                                M_openssl_init = false;
                                return;
                            }
                            client_ssl_configure& operator=(struct stagdeer::client::socketSSL::client_ssl_configure&& M_client_configure_other_move_operator){
                                if (this != &M_client_configure_other_move_operator) {
                                    M_openssl_ctx_ptr = M_client_configure_other_move_operator.M_openssl_ctx_ptr;
                                    M_openssl_ctx_ptr = nullptr;
                                    M_openssl_init = M_client_configure_other_move_operator.M_openssl_init;
                                    M_openssl_init = false;
                                    M_openssl_ptr = M_client_configure_other_move_operator.M_openssl_ptr;
                                    M_openssl_ptr = nullptr;
                                    M_CA_cert_path = std::move(M_client_configure_other_move_operator.M_CA_cert_path);
                                    M_stl_verify_depth = std::move(M_client_configure_other_move_operator.M_stl_verify_depth);
                                }
                                return *this;
                            }
                        

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
                char* M_err_buffer = nullptr;
                BIO* M_newbio = BIO_new(BIO_s_mem());
                ERR_print_errors(M_newbio);
                long M_err_len = BIO_get_mem_data(M_newbio,&M_err_buffer);
                std::string M_error_message(M_err_buffer , M_err_len);
                BIO_free(M_newbio);
                return M_error_message;
            }

            void M_init_openssl() noexcept {
                SSL_library_init();
                OpenSSL_add_all_algorithms();
                SSL_load_error_strings();
            }

            struct stagdeer::client::socketTcp::client_context M_ssl_write_retry
                (struct stagdeer::client::socketTcp::client_context&& client_ctx,
                    SSL* M_retry_tls , SSL_CTX* M_retry_ctx , int TLS_RET 
                     , size_t M_strfull_len , size_t M_write_toal_data
                )  {
                   if (client_ctx.M_socketfd < 0 || !M_retry_tls || !M_retry_ctx) {
                    throw std::runtime_error("Invalid socket or tls pointer!");
                   }
                   
                   //UPDATE RETRY COUNT
                   client_ctx.M_connect_rety_count = 0;
                   while (client_ctx.M_connect_rety_count < client_ctx.M_max_rety_count) {
                    //RETRY LOGIC
                    //ADD COUNT
                    client_ctx.M_connect_rety_count ++;
                    SLEEP(1000 * client_ctx.M_connect_rety_count);
                    //GET WRITE ERROR
                    int M_write_err = SSL_get_error(M_retry_tls, TLS_RET);
                        if (M_write_err == SSL_ERROR_WANT_WRITE || M_write_err == SSL_ERROR_WANT_READ || SSL_ERROR_SYSCALL) {
                            if (
                                errno != EAGAIN ||
                                errno != EWOULDBLOCK ||
                                errno != EINTR ||
                                errno != ENOBUFS ||
                                errno != ETIMEDOUT
                            ) {
                                //CANOT RETRY
                                client_ctx.M_is_retry_success = false;
                                return client_ctx;
                            }
                            //RETRY WRITE
                            //WAIT EVENT
                            fd_set M_fdset; timeval M_retry_wait_tv;
                            FD_ZERO(&M_fdset); FD_SET(client_ctx.M_socketfd, &M_fdset);
                            M_retry_wait_tv.tv_sec = client_ctx.M_socketfd; M_retry_wait_tv.tv_usec = 0;
                            
                            int M_wait_ret = select(client_ctx.M_socketfd + 1, &M_fdset,
                                nullptr , nullptr, &M_retry_wait_tv
                            );
                                if (M_wait_ret < 0) {
                                    //WAIT TIMOUT
                                    if (client_ctx.M_max_rety_count <= client_ctx.M_connect_rety_count) {
                                        //RETRY FAILED
                                        client_ctx.M_is_retry_success = false;
                                        return client_ctx;
                                    }
                                }
                                //WAIT SUCCESS
                                while (M_write_toal_data < M_strfull_len) {
                                    int M_retry_write_ret = SSL_write(M_retry_tls, client_ctx.M_clientMessage.c_str() + 
                                        M_write_toal_data, sizeof(M_strfull_len - M_write_toal_data)
                                    );
                                        if (M_retry_write_ret < 0) {
                                            if (client_ctx.M_max_rety_count <= client_ctx.M_connect_rety_count) {
                                                //RETRY FAILED
                                                client_ctx.M_is_retry_success = false;
                                                return client_ctx;
                                            }
                                            //CONTINUTE RETRY WEITE
                                            continue;
                                        }

                                        if (M_retry_write_ret == 0) {
                                            //CONNECT CLOSED
                                            client_ctx.M_is_retry_success = false;
                                            return client_ctx;
                                        }

                                        if (M_retry_write_ret > 0) {
                                            if (M_retry_write_ret + M_write_toal_data == M_strfull_len) {
                                                //RETRY SUCCESS
                                                client_ctx.M_is_retry_success = true;
                                                return client_ctx;
                                            }
                                            //CONTINUE RETRY
                                            //UPDATE MESSAGE
                                            M_write_toal_data += M_retry_write_ret;
                                            client_ctx.M_client_retry_write_bytes = M_write_toal_data;
                                        }
                                    continue;
                                }
                            client_ctx.M_is_retry_success = false;
                            return client_ctx; 
                        } else {
                            const std::string M_openssl_error = M_get_stl_err();
                            throw std::runtime_error("SSL ERROR: " + M_openssl_error);
                        }
                    client_ctx.M_is_retry_success = false;
                    return client_ctx;
                   }
                   client_ctx.M_is_retry_success = false;
                   return client_ctx;
                }
            
            struct stagdeer::client::socketTcp::client_context M_ssl_handshake_retry
                (struct stagdeer::client::socketTcp::client_context&& client_ctx , 
                    int M_handshake_err , SSL* M_retry_stl , SSL_CTX* M_retry_ctx
                ) {
                    if (client_ctx.M_socketfd < 0) {
                     throw std::runtime_error("Invalid socket!");
                    }
                
                    client_ctx.M_connect_rety_count = 0;
                    while (client_ctx.M_connect_rety_count < client_ctx.M_max_rety_count) {
                        client_ctx.M_connect_rety_count ++;
                        SLEEP(1000 * client_ctx.M_connect_rety_count);
                        if (M_handshake_err == SSL_ERROR_WANT_READ || M_handshake_err == SSL_ERROR_WANT_WRITE) {
                            int M_ssl_handshake_retry = SSL_connect(M_retry_stl);
                                if (M_ssl_handshake_retry != 1) {
                                    //RETRY FAILED
                                    if (client_ctx.M_connect_rety_count > client_ctx.M_max_rety_count) {
                                        client_ctx.M_is_retry_success = false;
                                        return client_ctx;
                                    }
                                    continue;
                                }
                            //RETRY SUCCESS
                            client_ctx.M_is_retry_success = true;
                            return client_ctx;
                        }
                        //UPDATE NEW STL PTR
                        M_retry_stl = M_creater_new_retry_stl(M_retry_stl, 
                            M_retry_ctx, client_ctx.M_socketfd , client_ctx.M_addrs_host);
                            switch (M_handshake_err) {
                                case SSL_ERROR_SYSCALL: {
                                    if (errno == ECONNRESET || errno == ETIMEDOUT) {
                                        //CONNECT REST OR TIMEOUT
                                        int M_stl_retry_connect_ret = SSL_connect(M_retry_stl);
                                            if (M_stl_retry_connect_ret != 1) {
                                                //RETY FAILED
                                                if (client_ctx.M_connect_rety_count > client_ctx.M_max_rety_count) {
                                                    client_ctx.M_is_retry_success = false;
                                                    return client_ctx;
                                                }
                                                continue;
                                            }
                                        //RETRY SUCCESS
                                        client_ctx.M_is_retry_success = true;
                                        //UPDATE CONGIGURE STL PTR
                                        M_openssl_configure.M_openssl_ptr = M_retry_stl;
                                        return client_ctx;
                                    }
                                    client_ctx.M_is_retry_success = false;
                                    return client_ctx;
                                }
                                //ERROR CALL FAILED END
                            
                                case SSL_ERROR_SSL: {
                                    unsigned long err = ERR_get_error();
                                    printf("ERROR SSL\n");
                                    std::string M_ssl_err_message = M_get_stl_err();
                                    throw std::runtime_error(M_ssl_err_message);
                                }
                                //SSL ERROR
                            };
                        }
                        client_ctx.M_is_retry_success = false;
                        return client_ctx;
                    }
        };
        using socketTlsPtrT = std::shared_ptr<stagdeer::client::socketSSL>;
    }
}

#endif
