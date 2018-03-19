#include "http_messages.hh"

// You could implement your logic for handling /cgi-bin requests here

HttpResponse handle_cgi_bin(const HttpRequest& request) {
  HttpResponse response;
  response.http_version = request.http_version;
  // TODO: Task 2.2
  return response;
}
