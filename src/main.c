#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#ifndef PORT
#define PORT 8080
#endif

int main(void) {

  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(PORT);
  server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  int is_binded = bind(server_fd, (struct sockaddr *) &server_address, sizeof(server_address));

  if (is_binded != 0) {
    fprintf(stderr, "ERROR: the port: %d is closed.\n", PORT);
    exit(-1);
  }

  listen(server_fd, 0);
  printf("> server is listening on port: %d.\n", PORT);

  struct sockaddr_in client_address;
  while (1) {
    int client_size = sizeof(client_address);
    int client_fd = accept(server_fd, (struct sockaddr *) &client_address, (socklen_t*)&client_size);

    unsigned char data_buffer[1024];
    int receivedDataSize = recv(client_fd, data_buffer, 1024, 0);
    printf("the server got %d bytes.\n", receivedDataSize);
  
    char message[] = "HTTP/1.1 200 OK\nServer: MCHS\nContent-Type: text/html\nConnection: close\n\n<h1>hello world</h1>";
    send(client_fd, message, sizeof(message), MSG_OOB);
    close(client_fd);
  }

  shutdown(server_fd, 0);
  return 0;
}
