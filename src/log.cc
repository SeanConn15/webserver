// Copyright 2018 hello
#include <string>
#include "server.hh"

void Log::write() {
}

void Log::addReqTime(time_t length) {
    if (length > max_request) {
        max_request = length;
    }
    if (min_request == 0 || length < min_request) {
        min_request = length;
    }
}
std::string Log::generate_logs()  {
    std::string hello;
    for (std::string s : request_list) {
        hello += s.c_str();
        hello += '\n';
    }
    return hello;
}
std::string Log::generate_stats() {
    std::stringstream stat;
    stat << "Christopher Sean Connelly\n";
    stat << "Uptime (seconds): " << (time(NULL) - start_time) << "\n";
    stat << "Requests: " << request_list.size() << "\n";
    stat << "Max request time (ms): " << max_request << "\n";
    stat << "Min request time (ms): " << min_request << "\n";

    return stat.str();
}
