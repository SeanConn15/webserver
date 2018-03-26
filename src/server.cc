/**
 * This file contains the primary logic for your server. It is responsible for
 * handling socket communication - parsing HTTP requests and sending HTTP responses
 * to the client. 
 */

#define MAXLENGTH 128
#include <functional>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iterator>
#include <vector>
#include <tuple>
#include "server.hh"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <thread>

#include "errors.hh"
#include "misc.hh"
#include "routes.hh"
#include "unistd.h"

Server::Server(SocketAcceptor const& acceptor) : _acceptor(acceptor) { }

void Server::run_linear() const {
	while (1) {
		Socket_t sock = _acceptor.accept_connection();
		handle(sock);
	}
}
//kills zombie processes
extern "C" void killZombie(int sig)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}
void Server::run_fork() const {
	// TODO: Task 1.4
	
	//set up a zombie killer for child processes
	struct sigaction za;
	za.sa_handler = killZombie;
	sigemptyset(&za.sa_mask);
	za.sa_flags = SA_RESTART;
	if(sigaction(SIGCHLD, &za, NULL)){
		perror("zombie catcher failed");
		exit(2);
	}

	while(1)	//always
	{
		Socket_t sock = _acceptor.accept_connection(); //accept a new connection
		int fid = fork();//spawn a child

		if(fid == 0)	//child: accept the socket connection, exit
		{
			//disable the zombie killer for the child by setting it tothe default
			za.sa_handler = SIG_DFL; //default 
			sigemptyset(&za.sa_mask);//wipe old struct
			sigaction(SIGCHLD, &za, NULL); //Register new sigaction
			handle(sock);
			exit(1);
		}
		//parent:look for an additional connection
	}
}

void Server::run_thread() const {
	// TODO: Task 1.4
	while (1) {
		Socket_t sock = _acceptor.accept_connection();
		std::thread (&Server::handle, this, std::move(sock)).detach();
		//handle(sock);
	}
}

void Server::run_thread_pool(const int num_threads) const {
	// TODO: Task 1.4
	//creates a number of threads that run linearly

	for(int i = 0; i < num_threads; i++)
	{
		std::thread (&Server::run_linear, this).detach();
	}
	run_linear();
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
	int fd = -1; //file requested, if -1 when called will do nothing.
	if(valid)
	{
		//evaluate the request made in the uri
		//also sets content length
		resp.message_body = evaluate_request(&resp, request.request_uri);

		//insert additional information into the headers
		std::string name;
		std::string val;
		//connection type
		name = "Connection";
		val  = "close";
		resp.headers[name] = val;


		//content size	
		name = "Content-Length";  //name and value for message size attribute
		val  = std::to_string(resp.message_body.length()); //write whatever is message is
		resp.headers[name] = val;
	}	

	//print out the respose and send it to the client.
	//std::cout << resp.to_string() << std::endl;
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
std::string Server::evaluate_request(HttpResponse* response, std::string uri) const
{
	//content type
	std::string name = "Content-Type";
	std::string type;

	std::string val;
	if (uri == "/hello")
	{
		val = "Hello CS252!";
		type = "text/text";
		response->headers[name] = val;
		return val;
	}

	//takes the string and searches for the corresponding file in http-root-dir/htdocs
	std::string path = uri;
	path = "/homes/connell7/cs252/lab5-src/http-root-dir/htdocs" + path;


	//see if the file exists and is is readable
	if( access( path.c_str(), R_OK ) != -1 ) // file exists
	{	
    	
		//see what type of file is being refrenced
		type = get_content_type(path);

		if(type.find("directory") != std::string::npos) //if it is a directory
		{
			//open the index.html for that directory
			if(path[path.length() - 1] != '/')
			{
				path += "/index.html";
			}
			else
			{
				//TODO: browse directory
				path += "index.html";
			}
			//now make sure it exists
			if( access( path.c_str(), R_OK ) == -1 ) // index.html does not exist
			{

				//file not found, send a 404
				response->status_code = 404;
				type = "html/html";
				response->headers[name] = val;
				val = path + ": not found";
				return val;

			}
		}
		//now there is a file that was either passed directly, or turned into an index.html

	} 
	else 
	{
		//directory/file not found, send a 404
		response->status_code = 404;
		type = "html/html";
		response->headers[name] = val;
		val = path + ": not found";
		return val;

	}


	//now that we have confirmed the file exists, get it's type
	type = get_content_type(path);
	response->headers[name] = val;
	


	FILE* file = fopen(path.c_str(), "rb");
	if(file == NULL)
	{
		printf("this should not ever happen\n\n\n\n\n");
		//file not found, send a 404
		response->status_code = 404;
		type = "text/text";
		response->headers[name] = val;
		path = "no way hosaye";
		val = "";
		return val;
	}
	else
	{
		//TODO: do something that is not this
		std::ifstream input( path.c_str(), std::ios::binary ); //make file stream

	    	// copies all data into buffer
		std::vector<char> buffer((
			std::istreambuf_iterator<char>(input)), 
			(std::istreambuf_iterator<char>()));
		//convert char vector into string
		std::string s( buffer.begin(), buffer.end() );
		//close the file
		fclose(file);
		return s; //return that string
	}
}
