/**
 * This file contains your implementation of a TLS socket and socket acceptor. The TLS socket uses
 * the OpenSSL library to handle all socket communication, so you need to configure OpenSSL and use the
 * OpenSSL functions to read/write to the socket. src/tcp.cc is provided for your reference on 
 * Sockets and SocketAdaptors and examples/simple_tls_server.c is provided for your reference on OpenSSL.
 */

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <iostream>
#include <sstream>
#include <cstring>
#include <memory>

#include "tls.hh"
#include "errors.hh"


TLSSocket::TLSSocket(int port_no, struct sockaddr_in addr, SSL* ssl) :
  _socket(port_no), _addr(addr), _ssl(ssl) {
    // TODO: Task 2.1
    char inet_pres[INET_ADDRSTRLEN];
    if (inet_ntop(addr.sin_family, &(addr.sin_addr), inet_pres, INET_ADDRSTRLEN)) {
        std::cout << "Received a connection from " << inet_pres << std::endl;
    }
    setenv("MYHTTPD_IP", inet_pres, 1);
}
TLSSocket::~TLSSocket() noexcept {
    // TODO: Task 2.1
    SSL_free(_ssl);
    EVP_cleanup();
    close(_socket);
}

char TLSSocket::getc() {
    char c = '\0';
    read(&c, 1);
    return c;
}

ssize_t TLSSocket::read(char *buf, size_t buf_len) {
    // TODO: Task 2.1
    ssize_t r = SSL_read(_ssl, buf, buf_len);
    return r;
}

std::string TLSSocket::readline() {
    std::string str;
    char c;
    while ((c = getc()) != '\n' && c != EOF) {
        str.append(1, c);
    }
    if (c == '\n') {
        str.append(1, '\n');
    }
    return str;
}

void TLSSocket::write(std::string const &str) {
        SSL_write(_ssl, str.c_str(), str.length());
}

void TLSSocket::write(char const *const buf, const size_t buf_len) {
    if (buf == NULL)
        return;
    SSL_write(_ssl, buf, strlen(buf));
}

TLSSocketAcceptor::TLSSocketAcceptor(const int portno) {
    // initializes things as normal, plus some ssl startup stuff

    // init_ssl
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    // create context

    const SSL_METHOD *method;

    method = SSLv23_server_method();

    _ssl_ctx = SSL_CTX_new(method);
    if (!_ssl_ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }


    // configure context
    SSL_CTX_set_ecdh_auto(_ssl_ctx, 1);

    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(_ssl_ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(_ssl_ctx, "key.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }


    _addr.sin_family = AF_INET;
    _addr.sin_port = htons(portno);
    _addr.sin_addr.s_addr = htonl(INADDR_ANY);

    _master_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (_master_socket < 0) {
        perror("Unable to create socket");
        exit(EXIT_FAILURE);
    }

    if (bind(_master_socket, (struct sockaddr*)&_addr, sizeof(_addr)) < 0) {
        perror("Unable to bind");
        exit(EXIT_FAILURE);
    }

    if (listen(_master_socket, 1) < 0) {
        perror("Unable to listen");
        exit(EXIT_FAILURE);
    }
}

Socket_t TLSSocketAcceptor::accept_connection() const {
        struct sockaddr_in addr;
        uint len = sizeof(addr);
        SSL *ssl;

        int client = accept(_master_socket, (struct sockaddr*)&addr, &len);
        if (client < 0) {
            perror("Unable to accept");
            exit(EXIT_FAILURE);
        }

        ssl = SSL_new(_ssl_ctx);
        SSL_set_fd(ssl, client);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        }
    return std::make_unique<TLSSocket>(client, addr, ssl);
}

TLSSocketAcceptor::~TLSSocketAcceptor() noexcept {
    SSL_CTX_free(_ssl_ctx);
}
