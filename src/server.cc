/**
 * This file contains the primary logic for your server. It is responsible for
 * handling socket communication - parsing HTTP requests and sending HTTP responses
 * to the client. 
 */

#define MAXLENGTH 128
#include <functional>
#include <iostream>
#include <sstream>
#include <vector>
#include <tuple>
#include "server.hh"

#include "http_messages.hh"
#include "errors.hh"
#include "misc.hh"
#include "routes.hh"

Server::Server(SocketAcceptor const& acceptor) : _acceptor(acceptor) { }

void Server::run_linear() const {
  while (1) {
    Socket_t sock = _acceptor.accept_connection();
    handle(sock);
  }
}

void Server::run_fork() const {
  // TODO: Task 1.4
}

void Server::run_thread() const {
  // TODO: Task 1.4
}

void Server::run_thread_pool(const int num_threads) const {
  // TODO: Task 1.4
}

// example route map. you could loop through these routes and find the first route which
// matches the prefix and call the corresponding handler. You are free to implement
// the different routes however you please
/*
std::vector<Route_t> route_map = {
  std::make_pair("/cgi-bin", handle_cgi_bin),
  std::make_pair("/", handle_htdocs),
  std::make_pair("", handle_default)
};
*/

void parse_request(const Socket_t& sock, HttpRequest* const request)
{
	
  	//first get the string written from the socket

	std::string buff = "";
	char ch;
	char lastc = 0;
	while ( buff.length() < MAXLENGTH && ( ch = sock->getc() ) > 0 ) 
	{
    		if ( lastc == '\015' && ch == '\012' ) //if you reach the end of the message
		{
			printf("HELLO FREN\n");
			buff = buff.substr(0, buff.length() - 1); //remove the trailing newline
			break;
		}
		lastc = ch;
		buff += ch;
	}
	//std::cout << buff << std::endl;	
	
	//now parse it and get all the component parts together
	//<Method> <SP> <Request-URI> <SP> <HTTP-Version> <CRLF>	


	//default is GET
	request->method = "GET";

	//forming uri
	request->request_uri = "";
	if (buff.length() < 4)
	{
		return;
	}
	int i = 4; //place after "GET "
	while(buff[i] != ' ')
	{
		i++;	
		if(i == buff.length())
		{
			return;
		}
	}
	request->request_uri = buff.substr(4, i - 4);

	//forming the HTTP-VERSION
	request->http_version = buff.substr(i + 1, buff.length() - 1);

}
void Server::handle(const Socket_t& sock) const {
  HttpRequest request;
  // TODO: implement parsing HTTP requests
  // recommendation:
  parse_request(sock, &request);
  request.print();

  HttpResponse resp;
  // TODO: Make a response for the HTTP request
  resp.http_version = "HTTP/1.1";
  std::cout << resp.to_string() << std::endl;
  sock->write(resp.to_string());
}


