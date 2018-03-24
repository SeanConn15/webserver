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

void Server::handle(const Socket_t& sock) const {
	////get request and fill in the structure
	HttpRequest request;
	parse_request(sock, &request);
	//print out the struct
	request.print();



	////form the response	
	HttpResponse resp;
	resp.http_version = "HTTP/1.1";

	//determe the validity of the request
	bool valid = false; //defaults to invalid
	//check username and password combo

	if(request.headers["Authorization"] != "Basic c2Vhbjp5ZXM=") //sean:yes in base 64
	{
		resp.status_code = 401;
		std::string name = "WWW-Authenticate";
		std::string val  = "Basic realm=\"myhttpd-cs252\"";
		resp.headers[name] = val;
	}
	else
	{
		resp.status_code = 200;
		valid = true;
	}

	//generate the content to be returned

	if(valid)
	{
		if (request.request_uri.compare("/hello") == 0)
			resp.message_body = "Hello CS252!";
		else
		{
			//evaluate the request made in the uri
			resp.message_body = evaluate_request(request.request_uri);
		}



		//insert additional information into the headers
		std::string name;
		std::string val;
		//connection type
		name = "Connection";
		val  = "close";
		resp.headers[name] = val;

		//content type
		name = "Content-Type";
		val  = "text/text";
		resp.headers[name] = val;

		//finding content length
		name = "Content-Length";
		val  = std::to_string(resp.message_body.length());
		resp.headers[name] = val;
	}	

	std::cout << resp.to_string() << std::endl;
	sock->write(resp.to_string());
}

void Server::parse_request(const Socket_t& sock, HttpRequest* const request) const
{
	
	//first get the string written from the socket
	std::vector<std::string> lines;
	std::string line = sock->readline();
	while(line.compare("\r\n") != 0)
	{
		lines.push_back(line.substr(0, line.length() - 2));	//remove /r/n and add to vector
		line = sock->readline();				//read new line
	}
	
	//std::cout << buff << std::endl;	
	
	//now parse it and get all the component parts together
	//<Method> <SP> <Request-URI> <SP> <HTTP-Version> <CRLF>	


	//default is GET
	//TODO: actually read the request
	request->method = "GET";

	//forming uri
	request->request_uri = "";
	if (lines[0].length() < 4)
	{
		return;
	}
	int i = 4; //place after "GET "
	while(lines[0][i] != ' ')
	{
		i++;	
		if(i == lines[0].length())
		{
			return;
		}
	}
	request->request_uri = lines[0].substr(4, i - 4);

	//forming the HTTP-VERSION
	request->http_version = lines[0].substr(i + 1, lines[0].length() - 1);
	
	//now read headers
	for (int i = 1; i < lines.size(); i++)
	{
		//TODO:check for <text>: <text>
		size_t seperator = lines[i].find(':');
		if(seperator == std::string::npos)
		{
			//TODO: handle bad headers
		}
		std::string begin = lines[i].substr(0, seperator);
		std::string end   = lines[i].substr(seperator + 2, lines[i].length() - 1);
		//std::cout << "'" << begin << "' '" << end << "'";
		request->headers[begin] = end;
	}

}
std::string Server::evaluate_request(std::string uri) const
{

}
