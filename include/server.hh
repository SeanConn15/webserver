#ifndef  INCLUDE_SERVER_HH_
#define INCLUDE_SERVER_HH_

#include "socket.hh"
#include "http_messages.hh"

class Server {
 private:
    SocketAcceptor const& _acceptor;
    std::string evaluate_request(HttpResponse* response, std::string uri) const;
    void parse_request(const Socket_t& sock, HttpRequest* const request) const;

 public:
    explicit Server(SocketAcceptor const& acceptor);
    void run_linear() const;
    void run_fork() const;
    void run_thread_pool(const int num_threads) const;
    void run_thread() const;

    void handle(const Socket_t& sock) const;
};

#endif  // INCLUDE_SERVER_HH_
