
#include <stdio.h>

#define LF 10

extern "C" void
httprun(int ssock, char * query_string)
{
  FILE * fssock = fdopen( ssock, "w");

  if ( fssock == NULL ) {
    perror("fdopen");
  }

  fprintf( fssock, "Content-type: text/html%c%c",LF,LF);
  
  
  fprintf( fssock, "<h1>Hello. Hi from hello.so</h1>\n");

  fclose( fssock );
}
int main(){}
