/**
 * This file parses the command line arguments and correctly
 * starts your server. You should not need to edit this file
 */

#include <unistd.h>
#include <sys/resource.h>

#include <csignal>
#include <cstdio>
#include <iostream>

#include "server.hh"
#include "socket.hh"
#include "tcp.hh"
#include "tls.hh"


int verbosity = 1;
// 1 is least verbose, 4 is most.

// 1: new connections, closing connections
// 2: page requests
// 3: headers from/to client
// 4: everything

extern "C" void signal_handler(int signal) {
    exit(0);
}

int main(int argc, char** argv) {
//    struct rlimit mem_limit = { .rlim_cur = 40960000, .rlim_max = 91280000 };
    struct rlimit cpu_limit = { .rlim_cur = 300, .rlim_max = 600 };
//    if (setrlimit(RLIMIT_AS, &mem_limit)) {
//        perror("Couldn't set memory limit\n");
//    }
    if (setrlimit(RLIMIT_CPU, &cpu_limit)) {
        perror("Couldn't set CPU limit\n");
    }

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    char use_https = 0;
    int port_no = 0;
    int num_threads = 0;  // for use when running in pool of threads mode

    char usage[] = "USAGE: myhttpd -pNUM_THREADS [-vVERBOSTIY] [-s] [-h] PORT_NO \n";
	char help[] = 	"This is a webserver than serves files in the directory http-root-dir/, and does some basic logging.\n\n"
			"GENERAL OPTIONS:\n\n"
			"-h:	display this help screen\n"
			"-s:	use https to make connections\n"
			"-p <num>	make a pool of threads for every connection, specifiying the number of threads in the pool\n"
			"-v:	set the verbosity level of the output\n"
			"	2: IP's of incoming connections and page requests\n"
			"	3: headers sent to/gotten from the client\n"
			"	4: Debug (everything)\n\n"
			"\n"
			"PORT_NO	the port the server will accept conenctions from\n\n\n";

    if (argc == 1) {
        fputs(usage, stdout);
        return 0;
    }

    int c;
    while ((c = getopt(argc, argv, "p:v:sh")) != -1) {
        switch (c) {
            case 'h':
                fputs(help, stdout);
                return 0;
            case 'p':
                num_threads = stoi(std::string(optarg));
                break;
            case 's':
                use_https = 1;
                break;
	    case 'v':
                verbosity = stoi(std::string(optarg));
		if (verbosity < 1 || verbosity > 4)
                {
                    std::cerr << "verbosity must be between 1 and 4" << std::endl;
                    return 1;
		}
                break;
            case '?':
                if (isprint(optopt)) {
                    std::cerr << "Unknown option: -" << static_cast<char>(optopt)
                              << std::endl;
                } else {
                    std::cerr << "Unknown option " << optopt <<  std::endl;
                }
                // Fall through
            default:
                fputs(usage, stderr);
                return 1;
        }
    }

    if (optind > argc) {
        std::cerr << "Extra arguments were specified" << std::endl;
        fputs(usage, stderr);
        return 1;
    } else if (optind == argc) {
        std::cerr << "Port number must be specified" << std::endl;
        return 1;
    }

    port_no = atoi(argv[optind]);
    std::cout << "Running on port " << port_no << " with " << num_threads << " threads";
    if(use_https)
        std::cout << ", using https." << std::endl;
    else
        std::cout << ", using http." << std::endl;
        
    SocketAcceptor* acceptor;
    if (use_https) {
        acceptor = new TLSSocketAcceptor(port_no);
    } else {
        acceptor = new TCPSocketAcceptor(port_no);
    }
    Server server(*acceptor, verbosity);
    server.run_thread_pool(num_threads);
    delete acceptor;
}
