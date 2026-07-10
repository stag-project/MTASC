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

#ifndef STAGDEER_CLIENT_SSL_SOCKET
#define STAGDEER_CLIENT_SSL_SOCKET

#include <algorithm>
#include <bits/types/struct_timeval.h>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
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
#include <sys/select.h>
#include <system_error>
#include <unistd.h>
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

            void set_tls_verify_depth(int M_number) {
                M_openssl_configure.M_tls_verify_depth = M_number;
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
                typename stagdeer::util::lambda_trais::constraint<
                    stagdeer::util::lambda_trais::M_is_retTp<
                        typename stagdeer::util::lambda_trais::M_get_lambda_ret_Tp<
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
                        //TRY CREATE TLS CONTEXT
                            self -> M_openssl_configure.M_openssl_ctx_ptr = 
                                self -> M_create_tls_context(M_ssl_options);
                                if (!self -> M_openssl_configure.M_openssl_ctx_ptr) {
                                    //CREATE FAILED
                                    std::error_code ec(errno , std::system_category());
                                    self -> M_thread_manager.getThreadManager()
                                        .asyncTaskvoid(std::move(callback_token),
                                        ec , std::move(M_ssl_options) , std::move(M_client_context)
                                    );
                                    return;
                                }
                            //TRY CREAT TLS PTR
                            self -> M_openssl_configure.M_openssl_ptr = 
                                self -> M_create_tls(self -> M_openssl_configure.M_openssl_ctx_ptr,
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
                typename stagdeer::util::lambda_trais::constraint<
                    stagdeer::util::lambda_trais::M_is_retTp<
                        typename stagdeer::util::lambda_trais::M_get_lambda_ret_Tp<
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
                        int M_tls_connect_ret = SSL_connect(self -> 
                                M_openssl_configure.M_openssl_ptr);
                            if (M_tls_connect_ret == 1) {
                                //CONNECT SUCCESS
                                std::error_code ec;
                                self -> M_thread_manager.getThreadManager()
                                    .asyncTaskvoid(std::move(callback_token), ec , 
                                        std::move(M_client_context)
                                );
                                return;
                            } else if (M_tls_connect_ret < 1 || M_tls_connect_ret <= 0) {
                                //TLS HANDSHAKE FAILED
                                int M_handshake_err = SSL_get_error(self -> M_openssl_configure.M_openssl_ptr,
                                     M_tls_connect_ret);
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
                                    struct stagdeer::client::socketTcp::client_context M_tls_retry_connect = self -> 
                                        M_ssl_handshake_retry(std::move(M_client_context) , M_handshake_err , 
                                        self -> M_openssl_configure.M_openssl_ptr ,
                                         self -> M_openssl_configure.M_openssl_ctx_ptr
                                        );
                                        if (M_tls_retry_connect.M_is_retry_success) {
                                            //TLS HANDSHAKE RETRY SUCCESS
                                            std::error_code ec;
                                            M_client_context = std::move(M_tls_retry_connect);
                                            self -> M_thread_manager.getThreadManager()
                                                .asyncTaskvoid(std::move(callback_token), ec ,
                                                 std::move(M_client_context)
                                            );
                                            return;
                                        } else {
                                        //RETRY HANDSHAKE RETRY FAILED
                                            M_client_context = std::move(M_tls_retry_connect);
                                            std::error_code ec(errno , std::system_category());
                                            self -> M_thread_manager.getThreadManager()   
                                                .asyncTaskvoid(std::move(callback_token),
                                                    ec , std::move(M_client_context)
                                            );
                                            return;
                                        }
                            } else {
                                std::error_code ec = std::make_error_code(std::errc::connection_reset);
                                self -> M_thread_manager.getThreadManager()
                                    .asyncTaskvoid(std::move(callback_token),
                                        ec , std::move(M_client_context)
                                    );
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
                typename stagdeer::util::lambda_trais::constraint<
                  stagdeer::util::lambda_trais::M_is_retTp<
                    typename stagdeer::util::lambda_trais::M_get_lambda_ret_Tp
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
                                        return;
                                    }
                        return;
                    });
                }
                return -1;
            }

            template<typename Tp>
            [[gnu::always_inline]] [[gnu::hot]]
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
                typename stagdeer::util::lambda_trais::constraint<
                    stagdeer::util::lambda_trais::M_is_retTp<
                        typename stagdeer::util::lambda_trais::M_get_lambda_ret_Tp<
                            Tp, const std::error_code& , size_t , 
                            std::shared_ptr<
                                stagdeer::client::readBuffer
                            >&&,
                            struct stagdeer::client::socketTcp::client_context&&
                        >::__M_ret_lmdba, void>::__is_M_ret_Tp
                >::type{}
            ) {
              //CREATE READBUFFER PTR
              std::shared_ptr<stagdeer::client::readBuffer> M_read_buffer_ptr = std::make_shared<stagdeer::client::readBuffer>();
              if (M_context_.M_socketfd < 0 || !M_openssl_configure.M_openssl_ptr 
                    || !M_openssl_configure.M_openssl_ctx_ptr || M_delimiter.empty()) {
                //INVLAID DATA`
                std::error_code ec = std::make_error_code(std::errc::bad_file_descriptor);
                M_thread_manager.getThreadManager()
                    .asyncTaskvoid(std::move(callback_token), ec , size_t(0) , 
                        std::move(M_read_buffer_ptr) , std::move(M_context_)
                    );
                return -1;
            }
            M_context_.M_client_read_buffer = std::move(M_read_buffer_ptr);
              M_thread_manager.getThreadManager()
                .asyncTaskvoid(std::move(
                    [self = shared_from_this() ,
                    M_context_ = std::move(M_context_), callback_token = std::function<void(
                        const std::error_code& , size_t , std::shared_ptr<stagdeer::client::readBuffer>&&,
                            struct stagdeer::client::socketTcp::client_context&&
                    )>(std::move(callback_token)) , 
                        M_delimiter = std::move(M_delimiter)]() mutable {
                        //CREATE STATE
                        enum class M_READ_STATE { VERIFY, READING, FOUND, ERROR, CLOSE };
                        int M_tls_ret = 0;
                        M_READ_STATE M_new_state = M_READ_STATE::VERIFY;
                        while (M_new_state != M_READ_STATE::FOUND) {
                            switch (M_new_state) {
                                case M_READ_STATE::VERIFY: {
                                    bool M_found_delimiter = false;
                                    const char* M_tolal_found_data = M_context_.M_client_read_buffer->peekData();
                                    size_t M_readble_bytes = M_context_.M_client_read_buffer->readableBytes();
                                    for (int M_read_pointer = 0; M_read_pointer + M_delimiter.size() 
                                        <= M_readble_bytes; ++ M_read_pointer) {
                                            if (memcmp(M_tolal_found_data + M_readble_bytes,
                                                M_delimiter.c_str(), M_delimiter.size()) == 0) {
                                                    //FOUND SUCCESS
                                                    M_new_state = M_READ_STATE::FOUND;
                                                    M_found_delimiter = true;
                                                break;
                                            }
                                    }

                                    if (M_found_delimiter) {
                                        break;
                                    }

                                    //NOT FOUND CONTINUE READING
                                    M_new_state = M_READ_STATE::READING;
                                }

                                //VERIFY END
                                case M_READ_STATE::READING: {
                                    //WAIT DATA
                                    struct timeval M_new_tv {.tv_sec = M_context_.M_timeout , .tv_usec = 0};
                                    fd_set M_fd_set;
                                    FD_ZERO(&M_fd_set); FD_SET(M_context_.M_socketfd, &M_fd_set);
                                    int M_wait_ret = select(M_context_.M_socketfd + 1, &M_fd_set, 
                                        nullptr, nullptr, &M_new_tv);
                                        
                                        if (M_wait_ret < 0) {
                                            //WAIT TIMEOUT
                                            std::error_code ec = std::make_error_code(std::errc::timed_out);
                                            self->M_thread_manager.getThreadManager()
                                                .asyncTaskvoid(std::move(callback_token), ec , size_t(0) , 
                                                std::move(M_context_.M_client_read_buffer) , 
                                                std::move(M_context_)
                                            );
                                            return;
                                        }

                                     char M_read_cache_buffer[596];
                                     M_tls_ret = SSL_read(self -> M_openssl_configure.M_openssl_ptr,
                                         M_read_cache_buffer, sizeof(M_read_cache_buffer)
                                        );
                                        if (M_tls_ret < 0) {
                                            //READ FAILED
                                            //CHANGE STATUS RETRY
                                            M_new_state = M_READ_STATE::ERROR;
                                            continue;
                                        }
                                        if (M_tls_ret == 0) {
                                            //CONNECTION CLOSED
                                            std::error_code ec = std::make_error_code(std::errc::connection_reset);
                                            self -> M_thread_manager.getThreadManager()
                                                .asyncTaskvoid(std::move(callback_token), ec , size_t(0),
                                                    std::move(M_context_.M_client_read_buffer) , std::move(M_context_)
                                                );
                                                
                                        }
                                        if (M_tls_ret > 0) {
                                            //READ SUCCESS
                                            M_context_.M_client_read_buffer->appendTobuffer(M_read_cache_buffer, M_tls_ret);
                                            //CHANGE STATUS
                                            M_new_state = M_READ_STATE::VERIFY;
                                            break;
                                        }
                                }

                                //READING END

                                case M_READ_STATE::FOUND: {
                                    std::error_code ec;
                                    self -> M_thread_manager.getThreadManager()
                                        .asyncTaskvoid(std::move(callback_token), 
                                            ec , size_t(M_context_.M_client_read_buffer->readableBytes()),
                                             std::move(M_context_.M_client_read_buffer) 
                                            , std::move(M_context_)
                                    );
                                    return;
                                }

                                //FOUND END

                                case M_READ_STATE::CLOSE: {
                                    //CONNECTION CLOSED
                                    std::error_code ec(std::make_error_code(std::errc::connection_reset));
                                    self->M_thread_manager.getThreadManager()
                                        .asyncTaskvoid(std::move(callback_token),
                                            ec , size_t(M_context_.M_client_read_buffer->readableBytes()), 
                                                std::move(M_context_.M_client_read_buffer) 
                                                , std::move(M_context_) 
                                    );
                                    return;
                                }

                                //CLOSE END

                                case M_READ_STATE::ERROR: {
                                    //RETRY CONTINUE
                                    struct stagdeer::client::socketTcp::client_context M_retry_ctx =
                                        self -> M_ssl_read_until_retry(std::move(M_context_), 
                                            self -> M_openssl_configure.M_openssl_ptr, self -> M_openssl_configure.M_openssl_ctx_ptr, 
                                                M_tls_ret, M_delimiter
                                            );
                                        M_context_ = std::move(M_retry_ctx);
                                        if (M_context_.M_is_retry_success) {
                                            //RETRY READ SUCCESS
                                            std::error_code ec;
                                            self -> M_thread_manager.getThreadManager()
                                                .asyncTaskvoid(std::move(callback_token), ec ,
                                                    size_t(M_context_.M_client_read_buffer->readableBytes()),
                                                        std::move(M_context_.M_client_read_buffer),
                                                            std::move(M_context_)
                                                );
                                            return;
                                        }
                                    //RETRY READ FAILED
                                    int M_tls_read_err = SSL_get_error(self -> M_openssl_configure.M_openssl_ptr, M_tls_ret);
                                    self -> print_tls_ERR(M_tls_read_err);
                                    std::error_code ec = std::make_error_code(std::errc::io_error);
                                    self -> M_thread_manager.getThreadManager()
                                        .asyncTaskvoid(std::move(callback_token), ec
                                            ,size_t(0) , std::move(M_context_.M_client_read_buffer),
                                                std::move(M_context_)
                                    );
                                    return;
                                }
                                //ERROR END
                            }
                        }
                        //WHILE END
                    }
                ));
                return -1;
            }

            private:

            stagdeer::THREAD& M_thread_manager = stagdeer::THREAD::getInstance();

            SSL* M_create_tls(SSL_CTX* M_ssl_context , M_SOCKET_TP M_fd , const char* M_hostname) {
                if (!M_ssl_context) {
                    return nullptr;
                }
                SSL* M_openssl_ptr = SSL_new(M_ssl_context);
                SSL_set_tlsext_host_name(M_openssl_ptr, M_hostname);
                SSL_set_fd(M_openssl_ptr, M_fd);
                return M_openssl_ptr;
            }

            SSL_CTX* M_create_tls_context(struct openssl_options M_tls_options) {
                SSL_CTX* M_openssl_ctx_ptr = nullptr;
                if (M_tls_options.enable_tls_v1) {
                    M_openssl_ctx_ptr = SSL_CTX_new(TLSv1_2_client_method());
                } else {
                    M_openssl_ctx_ptr = SSL_CTX_new(TLS_client_method());
                }
                SSL_CTX_set_verify(M_openssl_ctx_ptr, SSL_VERIFY_PEER, nullptr);
                if (M_tls_options.enable_custom_cert) {
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
                SSL_CTX_set_verify_depth(M_openssl_ctx_ptr,M_openssl_configure.M_tls_verify_depth);
                return M_openssl_ctx_ptr;
            }

            SSL* M_creater_new_retry_tls(SSL* _M_bad_tls__ , SSL_CTX* _M_ctx_tls__ , int _M_fd__ , const char* _M_hostname__) {
                free(_M_bad_tls__);
                SSL* M_new_ssl = SSL_new(_M_ctx_tls__);
                if (!M_new_ssl) {
                    //INVALID TLS
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
                int M_tls_verify_depth = 4;

                client_ssl_configure() = default;
                
                client_ssl_configure(const struct stagdeer::client::socketSSL::client_ssl_configure& M_client_configure_other_copy)
                    :M_openssl_ctx_ptr(M_client_configure_other_copy.M_openssl_ctx_ptr),
                    M_openssl_ptr(M_client_configure_other_copy.M_openssl_ptr),
                    M_openssl_init(M_client_configure_other_copy.M_openssl_init),
                    M_tls_verify_depth(M_client_configure_other_copy.M_tls_verify_depth),
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
                                M_tls_verify_depth = std::move(M_client_configure_other_copy_operator.M_tls_verify_depth);
                            }
                            return *this;
                        }
                        client_ssl_configure(struct stagdeer::client::socketSSL::client_ssl_configure&& M_client_configure_other_move)
                            :M_openssl_ctx_ptr(M_client_configure_other_move.M_openssl_ctx_ptr),
                            M_openssl_ptr(M_client_configure_other_move.M_openssl_ptr),
                            M_openssl_init(M_client_configure_other_move.M_openssl_init),
                            M_tls_verify_depth(M_client_configure_other_move.M_tls_verify_depth),
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
                                    M_tls_verify_depth = std::move(M_client_configure_other_move_operator.M_tls_verify_depth);
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

            void print_tls_ERR(int M_tls_err) {
                char M_ERR_buf[256];
                ERR_error_string_n(M_tls_err, M_ERR_buf, sizeof(M_ERR_buf));
                printf("OpenSSL error stack: %s\n", M_ERR_buf);
                return;
            }

            std::string M_get_tls_err() {
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

            struct stagdeer::client::socketTcp::client_context M_ssl_read_until_retry
                (struct stagdeer::client::socketTcp::client_context&& client_ctx ,
                    SSL* M_retry_tls , SSL_CTX* M_retry_ctx , int TLS_RET
                        , std::string& M_delimiter
                ) {
                    if (client_ctx.M_socketfd < 0 || !M_retry_tls || !M_retry_ctx) {
                     throw std::runtime_error("Invliad client context");
                    }
                    
                //UPDATE COUNT
                client_ctx.M_connect_rety_count = 0;
                int M_tls_read_err = SSL_get_error(M_retry_tls, TLS_RET);
                    if (M_tls_read_err == SSL_ERROR_SYSCALL || M_tls_read_err == SSL_ERROR_WANT_WRITE 
                        || M_tls_read_err == SSL_ERROR_WANT_READ) {
                            //VERIFY ERRNO
                            if (M_tls_read_err == SSL_ERROR_WANT_READ || M_tls_read_err == SSL_ERROR_WANT_WRITE 
                                || M_tls_read_err == 2) {
                                //WAIT SOCKET I/O EVENT`
                                struct timeval M_new_tv;
                                fd_set M_select_set;
                                FD_ZERO(&M_select_set); FD_SET(client_ctx.M_socketfd, &M_select_set);
                                
                                //SETTING EVENT WAIT TIMEOUT
                                M_new_tv.tv_usec = 0;
                                M_new_tv.tv_sec = client_ctx.M_timeout;
                                int M_wait_socket_event = select(client_ctx.M_socketfd + 1,
                                    &M_select_set, nullptr, nullptr,
                                    &M_new_tv);
                                    if (M_wait_socket_event < 0 ) {
                                        //WAIT TIMEOUT
                                        client_ctx.M_is_retry_success = false;
                                        return client_ctx;
                                    }
                                //CONTINUE RETRY
                            }
                        
                        while (client_ctx.M_max_rety_count > client_ctx.M_connect_rety_count) {
                            //UPDATE COUNT
                            client_ctx.M_connect_rety_count ++;
                            //VERIFY RETRY CONUT
                            if (client_ctx.M_max_rety_count <= client_ctx.M_connect_rety_count) {
                                //RETRY FAILED
                                client_ctx.M_is_retry_success = false;
                                return client_ctx;
                            }

                            //SLEEP
                            SLEEP(client_ctx.M_connect_rety_count * 1000);
                            //VERIFY READ RESULT`
                            for (int M_index = 0; M_index + M_delimiter.size() <= client_ctx.M_client_read_buffer->readableBytes();
                                ++ M_index) {
                                    if (memcmp(client_ctx.M_client_read_buffer->peekData() + M_index, 
                                        M_delimiter.c_str(), M_delimiter.size()) == 0) {
                                            //RETRY FIND SUCCESS
                                            client_ctx.M_is_retry_success = true;
                                            return client_ctx;
                                        }
                                    //NOT FIND CONTINUE FIND
                                }
                            
                            //READING DATA`
                            char M_cache_buffer[1086];
                            int M_recv_ret = SSL_read(M_retry_tls, M_cache_buffer, sizeof(M_cache_buffer));
                                if (M_recv_ret < 0) {
                                    //READ FAILED
                                    int M_new_tls_err = SSL_get_error(M_retry_tls, M_recv_ret);
                                        if (M_new_tls_err == SSL_ERROR_WANT_READ || M_new_tls_err == SSL_ERROR_WANT_WRITE) {
                                            if (client_ctx.M_connect_rety_count < client_ctx.M_max_rety_count) {
                                                client_ctx.M_timeout += 100000;
                                                continue;
                                            }
                                            print_tls_ERR(M_new_tls_err);
                                            client_ctx.M_is_retry_success = false;
                                            return client_ctx;
                                        }
                                    if (client_ctx.M_connect_rety_count >= client_ctx.M_max_rety_count) {
                                        //RETRY FAILED
                                        print_tls_ERR(M_new_tls_err);
                                        client_ctx.M_is_retry_success = false;
                                        return client_ctx;
                                    }
                                    //CONTINUE RETRY
                                    break;
                                }
                                
                                if (M_recv_ret > 0) {
                                    //READ SUCCESS
                                    //APPEND TO BUFFER
                                    client_ctx.M_client_read_buffer->appendTobuffer(M_cache_buffer, M_recv_ret);
                                    if (client_ctx.M_connect_rety_count >= client_ctx.M_max_rety_count) {
                                        //ADD CONUT USE TO VERIFY
                                        client_ctx.M_max_rety_count --;
                                    }
                                    //TO VERIFY
                                    continue;
                                }

                                if (M_recv_ret == 0) {
                                    //CONNECT CLOSED
                                    client_ctx.M_is_retry_success = false;
                                    return client_ctx;
                                }
                        }
                    } else {
                        //OTHER ERROR CANOT RETRY
                        print_tls_ERR(M_tls_read_err);
                        client_ctx.M_is_retry_success = false;
                        return client_ctx;
                    }
                print_tls_ERR(M_tls_read_err);
                client_ctx.M_is_retry_success = false;
                return client_ctx;
            }

            struct stagdeer::client::socketTcp::client_context M_ssl_write_retry
                (struct stagdeer::client::socketTcp::client_context&& client_ctx,
                    SSL* M_retry_tls , SSL_CTX* M_retry_ctx , int TLS_RET 
                     , size_t M_strfull_len , size_t M_write_toal_data
                )  {
                   if (client_ctx.M_socketfd < 0 || !M_retry_tls || !M_retry_ctx) {
                    throw std::runtime_error("Invalid client context!");
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
                            print_tls_ERR(M_write_err);
                            return client_ctx;
                        }
                    client_ctx.M_is_retry_success = false;
                    return client_ctx;
                   }
                   client_ctx.M_is_retry_success = false;
                   return client_ctx;
                }
            
            struct stagdeer::client::socketTcp::client_context M_ssl_handshake_retry
                (struct stagdeer::client::socketTcp::client_context&& client_ctx , 
                    int M_handshake_err , SSL* M_retry_tls , SSL_CTX* M_retry_ctx
                ) {
                    if (client_ctx.M_socketfd < 0) {
                     throw std::runtime_error("Invalid socket!");
                    }
                
                    client_ctx.M_connect_rety_count = 0;
                    while (client_ctx.M_connect_rety_count < client_ctx.M_max_rety_count) {
                        client_ctx.M_connect_rety_count ++;
                        SLEEP(1000 * client_ctx.M_connect_rety_count);
                        if (M_handshake_err == SSL_ERROR_WANT_READ || M_handshake_err == SSL_ERROR_WANT_WRITE) {
                            int M_ssl_handshake_retry = SSL_connect(M_retry_tls);
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
                        //UPDATE NEW TLS PTR
                        M_retry_tls = M_creater_new_retry_tls(M_retry_tls, 
                            M_retry_ctx, client_ctx.M_socketfd , client_ctx.M_addrs_host);
                            switch (M_handshake_err) {
                                case SSL_ERROR_SYSCALL: {
                                    if (errno == ECONNRESET || errno == ETIMEDOUT) {
                                        //CONNECT REST OR TIMEOUT
                                        int M_tls_retry_connect_ret = SSL_connect(M_retry_tls);
                                            if (M_tls_retry_connect_ret != 1) {
                                                //RETY FAILED
                                                if (client_ctx.M_connect_rety_count > client_ctx.M_max_rety_count) {
                                                    client_ctx.M_is_retry_success = false;
                                                    return client_ctx;
                                                }
                                                int M_ssl_err = SSL_get_error(M_openssl_configure.M_openssl_ptr, M_tls_retry_connect_ret);
                                                print_tls_ERR(M_ssl_err);
                                                continue;
                                            }
                                        //RETRY SUCCESS
                                        client_ctx.M_is_retry_success = true;
                                        //UPDATE CONGIGURE TLS PTR
                                        M_openssl_configure.M_openssl_ptr = M_retry_tls;
                                        return client_ctx;
                                    }
                                    client_ctx.M_is_retry_success = false;
                                    return client_ctx;
                                }
                                //ERROR CALL FAILED END
                            
                                case SSL_ERROR_SSL: {
                                    unsigned long err = ERR_get_error();
                                    std::string M_ssl_err_message = M_get_tls_err();
                                    throw std::runtime_error(M_ssl_err_message);
                                }
                                //SSL ERROR
                            };
                        }
                        client_ctx.M_is_retry_success = false;
                        return client_ctx;
                    }
        };
        using socketTlsSocket = std::shared_ptr<stagdeer::client::socketSSL>;
    }
}

#endif