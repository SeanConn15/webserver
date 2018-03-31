#include "http_messages.hh"
#include <unistd.h>
#include <dlfcn.h>
#include <link.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdlib.h>
#include "socket.hh"


//all dynamic libararies have to conform to this fucntion standard
typedef void (*httprunfunc)(int ssock, const char* querystring);

void handle_cgi_bin(const HttpRequest& request,const Socket_t& sock ) {

	//get the script from the uri
	//script name cannot contain "?"
	//remove "/cgi-bin/"
	std::string full = request.request_uri.substr(9, request.request_uri.length() - 8);

	//set up pipes
	int pipe_fd[2];
	if (pipe(pipe_fd) == -1) {
		perror("cgi_bin pipe error");
		exit(-1);
	}

	//split the string up and get environment variables in this form:
	// ?<var>=<val>&<var>=<val>&...<var>=<val>
	
	//seperates executable
	int i = 0;
	while(full[i] != '?' && i < full.length())
	{
		i++;
	}
	std::string exe = full.substr(0, i);
	i++;

	std::string name= "";
	std::string val = "";
	//gets all the environment variables
	//TODO: protections for malformed strings
	//lab handout said to to set this
	setenv("REQUEST_METHOD","GET", 1);

	int j;
	while(i < full.length())
	{
		j = i;
		while(full[i] != '=' )
			i++;
		name = full.substr(j, i-j);
		i++;		//skip over =
		j = i;
		while(full[i] != '&' && i < full.length())
			i++;
		val = full.substr(j, i-j);
		i++; 		//skip over & for next assignment

		//TODO:set environment variable here
		setenv(name.c_str(), val.c_str(), 1);
	}


	//turn exe into an aboslute path
	full = "/homes/connell7/cs252/lab5-src/http-root-dir/cgi-bin/" + exe;

	//keeps track of the type of script
	bool library = false;
	//if the scipt is just a regular script, just run it and put it's output through the pipe.
	//if it ends in .so, dynamically load it and give it a direct connection to the client
	if (exe.substr(exe.length() - 3, exe.length() - 1).compare(".so") == 0)
	{
		library = true;	
		void * lib = dlopen( "/homes/connell7/cs252/lab5-src/http-root-dir/cgi-bin/hello.so", 0 );

		if ( lib == NULL ) {
			fprintf( stderr, "./hello.so not found\n");
			perror( "dlopen");
			exit(1);
		}
	}

	//used if the script is not dynamic
	std::string result = "HTTP/1.1 200 OK\r\n";
	
	int pid = fork();

	if (pid == -1) {
		perror("cgi_bin fork error");
		exit(-1);
	}
	if (pid == 0) {
		//in the child
		dup2(pipe_fd[1], STDOUT_FILENO);
		//dup2(pipe_fd[1], STDOUT_FILENO);
		close(pipe_fd[1]);

		//prepare to talk to client directly
		if(library)
		{

		}
		else//execute script and exit
		{
			close(pipe_fd[0]);  // close read end
			execl(full.c_str(), exe.c_str(), NULL);
		}
		perror("cgi_bin child error");
		exit(-1);
	} else {
		

		if(!library)//if it is not a library just read the output and write it to the pipe
		{
			close(pipe_fd[1]);  // close write end

			char buf;
			while (read(pipe_fd[0], &buf, 1) > 0) {
				result += buf;
			}
			close(pipe_fd[0]);  // close read end

			int status;
			if (waitpid(pid, &status, 0) == -1) {
				perror("cgi_bin waitpid error");
				exit(-1);
			}
			if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
			    std::cerr << "cgi_bin returned nonzero status for " << request.request_uri << std::endl;
			    result = request.request_uri + " Not Found\r\n\r\n";

			}
		}
		else//allow the child which is a dynamically loaded library to write and read
		{
			//output is from the pipe, write that to the client
			//input is from the client's socket
			//write and read at the same time?


		}
	}
	//this function writes directly to the client
	sock->write(result); 
}
