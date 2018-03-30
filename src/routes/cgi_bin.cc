#include "http_messages.hh"
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

// You could implement your logic for handling /cgi-bin requests here

std::string handle_cgi_bin(const HttpRequest& request) {


	//execute the script and save it in response
	std::string result = "HTTP/1.1 200 OK\r\n";
	int pipe_fd[2];
	if (pipe(pipe_fd) == -1) {
		perror("cgi_bin pipe error");
		exit(-1);
	}
	int pid = fork();

	if (pid == -1) {
		perror("cgi_bin fork error");
		exit(-1);
	}
	if (pid == 0) {
		//in the child
		close(pipe_fd[0]);  // close read end
		dup2(pipe_fd[1], STDOUT_FILENO);
		dup2(pipe_fd[1], STDOUT_FILENO);
		close(pipe_fd[1]);

		//get the script from the uri
		//script name cannot contain "?"
		//remove "/cgi-bin/"
		std::string full = request.request_uri.substr(9, request.request_uri.length() - 8);
		
		//get environment variables in this form:
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
		execl(full.c_str(), exe.c_str(), NULL);
		perror("cgi_bin execl error");
		exit(-1);
	} else {
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
	    result.clear();
	}
	}
	//this function writes directly to the client
	return result; 
}
