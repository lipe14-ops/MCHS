#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MB_SIZE 1024

#ifndef PORT
#define PORT 8080
#endif

typedef struct { 
  char * name;
  char * address;
  int port;
  struct sockaddr_in core;
  int fd;
} HTTPServer;

HTTPServer openHTTPServer(char * name, char * address, int port) {
  HTTPServer server;

  struct sockaddr_in serverCore;
  serverCore.sin_family = AF_INET;
  serverCore.sin_port = htons(port);
  serverCore.sin_addr.s_addr = inet_addr(address);

  server.name = &name[0];
  server.address = &address[0];
  server.port = port;
  server.core = serverCore;
  server.fd = socket(AF_INET, SOCK_STREAM, 0);

  int isBinded = bind(server.fd, (struct sockaddr *) &(server.core), sizeof(server.core));

  if (isBinded != 0) {
    fprintf(stderr, "ERROR: the port: %d is already open.\n", PORT);
    exit(-1);
  }

  listen(server.fd, 0);

  return server;
}

typedef struct {
  int fd;
  struct sockaddr_in core;
} HTTPClient;

typedef struct {
  char * method;
  char * path;
  char * data; 
} HTTPRequestHeader;

HTTPRequestHeader serializeHTTPRequestHeader(char * header) {
  return (HTTPRequestHeader) {
    .method = strtok(header, " "),
    .path = strtok(NULL, " "),
    .data = header,
  };
}
  
typedef struct {
  bool isValid;
  HTTPRequestHeader header;
  char * body;
} HTTPRequest; 

HTTPRequest serializeHTTPRequest(char * buffer) {
  char * delimiter_ptr = strstr(buffer, "\r\n\r\n");

  if (delimiter_ptr == NULL) 
    return (HTTPRequest) { 0 };
  
  int header_size = (int) (delimiter_ptr - buffer);

  HTTPRequest request = {
    .isValid = true,
    .body = delimiter_ptr + strlen(delimiter_ptr) + 1
  };

  if (request.isValid) {
    char * header_data = (char *) calloc(header_size, sizeof(char));
    memcpy(header_data, buffer, header_size);
    request.header = serializeHTTPRequestHeader(header_data);
  }

  return request;
}

typedef struct {
  char * header;
  char * body;
} HTTPResponse; 

void sendHTTPResponse(HTTPClient client, HTTPResponse response) {
  char message[strlen(response.header) + strlen(response.body) + 4];
  sprintf(message, "%s\r\n\r\n%s", response.header, response.body);
  send(client.fd, message, sizeof(message), MSG_OOB);
}

typedef struct {
  int size;
  char * content;
} FileData;

FileData getFileData(char * filepath) {
  char * real_filepath = realpath(filepath, 0);
  FILE * fd = fopen(real_filepath, "r");

  if (fd == NULL) {
    fprintf(stderr, "ERROR: filepath is not valid.\n");
    exit(-1);
  }

  fseek(fd, 0, SEEK_END);
  int contentLength = ftell(fd);
  rewind(fd);

  FileData file = {
    .size = contentLength,
    .content = (char *) calloc(contentLength, sizeof(char))
  };

  fread(file.content, contentLength, 1, fd);
  fclose(fd);
  return file;
}

typedef struct {
  char * path;
  char * method;
  void (*controller)(HTTPRequest, HTTPClient);
} HTTPRoute;

typedef struct {
  HTTPRoute * routes;
  size_t routesCount;
} HTTPServerRouter;

HTTPServerRouter newHTTPServerRouter() {
  return (HTTPServerRouter) {
    .routes = (HTTPRoute *) malloc(sizeof(HTTPRoute)),
    .routesCount = 0,
  };
}

void addHTTPServerRoute(HTTPServerRouter * router, HTTPRoute route) {
  router->routes =  (HTTPRoute *) realloc(router->routes, sizeof(HTTPRoute) * (++router->routesCount + 1));
  router->routes[router->routesCount - 1] = route;
}

HTTPRoute * HandlerHTTPServerRoutes(HTTPServerRouter router, HTTPRequest request) {
  for (size_t i = 0; i < router.routesCount; ++i) {
    HTTPRoute route = router.routes[i];
    if (strcmp(request.header.method, route.method) == 0 && strcmp(request.header.path, route.path) == 0) {
      return &router.routes[i];
    }
  }
  return NULL;
}

void indexPageController(HTTPRequest request, HTTPClient client) {
  FileData file = getFileData("./templates/index.html");

  HTTPResponse response = {
    .header = "HTTP/1.1 200 OK\nContent-Type: text/html\nConnection: close",
    .body = file.content
  };
  sendHTTPResponse(client, response);
}
  
int main(void) {
  HTTPServer server = openHTTPServer("MCHS", "127.0.0.1", PORT);
  HTTPServerRouter router = newHTTPServerRouter();
  HTTPClient client;

  addHTTPServerRoute(&router, (HTTPRoute) {
    .method = "GET",
    .path = "/",
    .controller = indexPageController
  });

  printf("> %s is listening on port: %d.\n", server.name, server.port);

  while (true) {
    int client_size = sizeof(client.core);
    client.fd = accept(server.fd, (struct sockaddr *) &client.core, (socklen_t*)&client_size);

    char request_buffer[3 * MB_SIZE];
    int receivedDataSize = recv(client.fd, request_buffer, 3 * MB_SIZE, 0);

    HTTPRequest request = serializeHTTPRequest(request_buffer);

    FileData file = getFileData("./templates/404.html");

    HTTPResponse notFoundResponse = {
      .header = "HTTP/1.1 404 Not found\nContent-Type: text/html\nConnection: close",
      .body = file.content
    };

    HTTPRoute * route = HandlerHTTPServerRoutes(router, request);

    if (request.isValid && route != NULL) {
      route->controller(request, client);
      printf("size: %d method: %s path: %s\n", receivedDataSize, request.header.method, request.header.path);
    } else if (request.isValid) {
      sendHTTPResponse(client, notFoundResponse);
    }

    free(request.header.data);
    close(client.fd);
  }

  shutdown(server.fd, 0);
  return 0;
}
