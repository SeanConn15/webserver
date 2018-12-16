#ifndef    INCLUDE_SERVER_HH_
#define INCLUDE_SERVER_HH_

#include <time.h>
#include <vector>
#include <string>
#include "socket.hh"
#include "http_messages.hh"

class Server {
 private:
    SocketAcceptor const& _acceptor;
    std::string evaluate_request(HttpResponse* response, std::string uri) const;
    void parse_request(const Socket_t& sock, HttpRequest* const request) const;
    std::string directory_page(std::string path, std::string dirname) const;

 public:
    explicit Server(SocketAcceptor const& acceptor);
    void run_linear() const;
    void run_fork() const;
    void run_thread_pool(const int num_threads) const;
    void run_thread() const;

    void handle(const Socket_t& sock) const;
};

class Log {
 public:
    //  requests are in the form:
    //   <ip> <request> <response code>
     std::vector<std::string> request_list;
    const time_t start_time = time(NULL);
    time_t max_request;  //  minumum and maximum time taken for a request
    time_t min_request;

    void write();  //  writes all of the needed information into myhttpd.log
    void addReqTime(time_t length);  //  adds a request time to the log file for the max/min
    std::string generate_stats();
    std::string generate_logs();
};
#endif    //   INCLUDE_SERVER_HH_
