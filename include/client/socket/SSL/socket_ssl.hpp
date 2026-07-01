#ifndef STAGDEER_CLIENT_SSL_SOCKET
#define STAGDEER_CLIENT_SSL_SOCKET

#include <bits/types/struct_timeval.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <openssl/bio.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "../../util/type_util.hpp"
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
            inline typename stagdeer::util::lamdba_trais::constraint<
                stagdeer::util::lamdba_trais::M_is_retTp<
                    typename stagdeer::util::lamdba_trais::M_get_lamdba_ret_Tp<
                        Tp, const std::error_code& , 
                            struct stagdeer::client::socketSSL::openssl_options&&,
                            struct stagdeer::client::socketTcp::client_context&&
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
                        std::declval<stagdeer::client::socketSSL::openssl_options&&>(),
                        std::declval<stagdeer::client::socketTcp::client_context&&>()
                    )
                )
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
                                        //RETRY CONNECT
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
                                            throw std::runtime_error("Retry stl hanshake to " +
                                                 std::string(M_client_context.M_addrs_host) + " falied"); 
                                        }
                            } else {
                                std::error_code ec = std::make_error_code(std::errc::connection_reset);
                                self -> M_thread_manager.getThreadManager()
                                    .asyncTaskvoid(std::move(callback_token),
                                        ec , std::move(M_client_context)
                                    );
                                throw std::runtime_error("STL ERROR: " + self->M_get_stl_err());
                            }
                        return;
                    }
                );
                return -1;
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
                }
                M_openssl_ctx_ptr = SSL_CTX_new(TLS_client_method());
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
            
            struct stagdeer::client::socketTcp::client_context M_ssl_handshake_retry
                (struct stagdeer::client::socketTcp::client_context client_ctx , 
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
    }
}

#endif