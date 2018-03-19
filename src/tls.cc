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
}
TLSSocket::~TLSSocket() noexcept {
    // TODO: Task 2.1
}

char TLSSocket::getc() {
    // TODO: Task 2.1
    char c = '\0';
    return c;
}

ssize_t TLSSocket::read(char *buf, size_t buf_len) {
    // TODO: Task 2.1
    ssize_t r = 0;
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
    write(str.c_str(), str.length());
}

void TLSSocket::write(char const *const buf, const size_t buf_len) {
    if (buf == NULL)
        return;
    // TODO: Task 2.1
}

TLSSocketAcceptor::TLSSocketAcceptor(const int portno) {
    // TODO: Task 2.1
}

Socket_t TLSSocketAcceptor::accept_connection() const {
    // TODO: Task 2.1
}

TLSSocketAcceptor::~TLSSocketAcceptor() noexcept {
    // TODO: Task 2.1
}
