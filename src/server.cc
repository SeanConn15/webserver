#define MAXLENGTH 128
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>

#include <functional>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iterator>
#include <vector>
#include <tuple>
#include <thread>

#include "server.hh"
#include "errors.hh"
#include "misc.hh"
#include "routes.hh"
#include "/usr/include/unistd.h"

  // TODO: handle broken ssh connections (restart server)

Server::Server(SocketAcceptor const& acceptor, int ver) : _acceptor(acceptor) {verbosity =  ver;}
Log loglist;  // class that holds all the loglist data, writes to its own file when write is called

// kills zombie processes
extern "C" void killZombie(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
}
void Server::run_linear() const {
    while (1) {
        Socket_t sock = _acceptor.accept_connection();
        handle(sock);
    }
}

void Server::run_thread_pool(const int num_threads) const {
      // creates a number of threads that run linearly

    for (int i = 0; i < num_threads; i++) {
        std::thread(&Server::run_linear, this).detach();
    }
    run_linear();
}


void Server::handle(const Socket_t& sock) const {
    time_t starttime = time(NULL);
      // // get request and fill in the structure
    HttpRequest request;
    if (parse_request(sock, &request) == 1)
        return;
      // print out the struct
    request.print(verbosity);



      // // form the response
    HttpResponse resp;
    resp.http_version = "HTTP/1.1";

      // determe the validity of the request
    bool valid = false;   // defaults to invalid
      // check username and password combo

    //password is diabled
    if (0){//(request.headers["Authorization"] != "Basic c2Vhbjp5ZXM=") {  // sean:yes in base 64
    
        resp.status_code = 401;
        std::string name = "WWW-Authenticate";
        std::string val  = "Basic realm=\"myhttpd-cs252\"";
        resp.headers[name] = val;
    } else {
        resp.status_code = 200;
        valid = true;
    }

      // generate the content to be returned
    int fd = -1;   // file requested, if -1 when called will do nothing.
    if (valid) {
          // evaluate the request made in the uri
          // also sets content length

        resp.message_body = evaluate_request(&resp, request.request_uri);
          // insert additional information into the headers
        std::string name;
        std::string val;
          // connection type
        name = "Connection";
        val  = "close";
        resp.headers[name] = val;


          // content size
        name = "Content-Length";    // name and value for message size attribute
        val  = std::to_string(resp.message_body.length());   // write whatever is message is
        resp.headers[name] = val;
    }

      // print out the respose and send it to the client.
      // std::cout << resp.to_string() << std::endl;
    sock->write(resp.to_string());
    loglist.addReqTime(time(NULL) - starttime);
      // for loglistging
    std::stringstream entry;
    entry << getenv("MYHTTPD_IP") << " " << request.request_uri << " "<< resp.status_code;
    loglist.request_list.push_back(entry.str());
}

int Server::parse_request(const Socket_t& sock, HttpRequest* const request) const {
      // first get the string written from the socket
    std::vector<std::string> lines;
    std::string line = sock->readline();
    while (line.compare("\r\n") != 0 && line.compare("") != 0) {
        lines.push_back(line.substr(0, line.length() - 2));      // remove /r/n and add to vector
        line = sock->readline();                  // read new line
    }
    if (lines.size() == 0) //if you get an empty request, ignore it
        return 1;

  // now parse it and get all the component parts together
  // <Method> <SP> <Request-URI> <SP> <HTTP-Version> <CRLF>


    request->method = "GET";

    // forming uri
    request->request_uri = "";
    if (lines[0].length() < 4) {
        return 0;
    }
    int i = 4;    // place after "GET "
    while (lines[0][i] != ' ') {
        i++;
        if (i == lines[0].length()) {
            return 0;
        }
    }
    request->request_uri = lines[0].substr(4, i - 4);

      // forming the HTTP-VERSION
    request->http_version = lines[0].substr(i + 1, lines[0].length() - 1);

      // now read headers
    for (int i = 1; i < lines.size(); i++) {
        size_t seperator = lines[i].find(':');
        if (seperator == std::string::npos) {
              // TODO: handle bad headers
        }
        std::string begin = lines[i].substr(0, seperator);
        std::string end   = lines[i].substr(seperator + 2, lines[i].length() - 1);
          // std::cout << "'" << begin << "' '" << end << "'";
        request->headers[begin] = end;
    }
    return 0;
}
std::string Server::evaluate_request(HttpResponse* response, std::string uri) const {
      // content type
    std::string name = "Content-Type";
    std::string type;

    std::string val;
      // for first test

    if (uri == "/hello") {
        val = "Hello CS252!";
        type = "text/text";
        response->headers[name] = val;
        return val;
    }
    if (uri == "/stats") {
        return loglist.generate_stats();
    }
    if (uri == "/logs") {
        return loglist.generate_logs();
    }


      // takes the string and searches for the corresponding file in http-root-dir/htdocs
    std::string path = uri;
    //getting working directory
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    path = cwd;
    path += "/http-root-dir/htdocs" + uri;


      // see if the file exists and is is readable
    if (access(path.c_str(), R_OK) != -1) {    //  file exists
          // see what type of file is being refrenced
        type = get_content_type(path);

        if (type.find("directory") != std::string::npos) {    // if it is a directory
              // open the index.html for that directory
            if (path[path.length() - 1] == '/' && uri.length() != 1) {
                return directory_page(path, uri);
            } else {
                if (uri.length() != 1)
                    path += '/';
                path += "index.html";
            }
              // now make sure it exists
            if (access(path.c_str(), R_OK) == -1) {    //  index.html does not exist
                  // file not found, send a 404
                response->status_code = 404;
                type = "html/html";
                response->headers[name] = val;
                val = path + ": not found";
                return val;
            }
        }
          // now there is a file that was either passed directly, or turned into an index.html

    } else {
          // directory/file not found, send a 404
        response->status_code = 404;
        type = "html/html";
        response->headers[name] = val;
        val = path + ": not found";
        return val;
    }


      // now that we have confirmed the file exists, get it's type
    type = get_content_type(path);
    response->headers[name] = val;


    FILE* file = fopen(path.c_str(), "rb");
    if (file == NULL) {
        printf("this should not ever happen\n\n\n\n\n");
          // file not found, send a 404
        response->status_code = 404;
        type = "text/text";
        response->headers[name] = val;
        path = "no way hosaye";
        val = "";
        return val;
    } else {
          // TODO: do something that is not this
        std::ifstream input(path.c_str(), std::ios::binary);    // make file stream

              //  copies all data into buffer
        std::vector<char> buffer((
            std::istreambuf_iterator<char>(input)),
            (std::istreambuf_iterator<char>()));
          // convert char vector into string
        std::string s(buffer.begin(), buffer.end() );
          // close the file
        fclose(file);
        return s;    // return that string
    }
}

std::string Server::directory_page(std::string path, std::string dirname) const {
      // html straight up copied from gun's ftp server

    std::string ret = "<!DOCTYPE html PUBLIC \"-  // W3C// DTD HTML 3.2 Final// EN\">";
    ret += "<html><link type=\"text/css\" id=\"dark-mode\" rel=\"stylesheet\" href=\"\"";
    ret += "><style type=\"text/";
    ret += "css\" id=\"dark-mode-custom-style\"></style><head>";
    ret += "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">";
    ret += "<title>Index of ";
    ret += dirname;
    ret += "</title></head>";

      // Table column names
    ret += "<body><table><tbody><tr><th><a href=\"";
    ret += dirname;
    ret += "\">Name</a></th><th><a href=\"https://ftp.gnu.org/gnu/?C=M;O=A\">";
    ret += "Last modified</a></th><th><a href=\"https://ftp.gnu.org/gnu/?C=S;O=A\"";
    ret += ">Size</a></th><th><a href=\"https://ftp.gnu.org/gnu/?C=D;O=A\">Descri";
    ret += "ption</a></th></tr>";

      // opening directory

    DIR * d = opendir(path.c_str());
    if (NULL == d) {
        perror("opendir: ");
        exit(1);
    }
      // printing parent directory

      // printing links to files
    for (dirent * ent = readdir(d); NULL != ent; ent = readdir(d)) {
        if (strcmp(ent->d_name, ".") != 0) {
          // icon
        ret += "<tr><td valign=\"top\"><img src=\"";

        if (ent->d_type == DT_DIR) {
            ret += "/folder.gif\" alt=\"[DIR]\"></td>";
        } else {
            ret += "/file.gif\" alt=\"[FILE]\"></td>";
        }

          // name
        ret += "<td><a href=\"";
          // ret+= dirname;
        ret += ent->d_name;
        if (ent->d_type == DT_DIR)
            ret += "/";
        ret += "\">";
        ret += ent->d_name;
        ret += "/</a></td><td align=\"right\">2013-12-13 14:00";
        ret += "  </td><td align=\"right\">  - </td><td>&nbsp;</td></tr>";
        }
    }
    closedir(d);
    return ret;
}
