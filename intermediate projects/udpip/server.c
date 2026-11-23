#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define PORT 5000
#define MAXLIN 1000

int main() 
{
  char buffer[100];
  char *message = "Hello Client";
  int listenfd, len;
  struct sockaddr_in servaddr, cliaddr;
  bzero(&servaddr, sizeof(servaddr));

  // Create a UDP socket
  listenfd = socket(AF_INET, SOCK_DGRAM, 0);

  //we are filling the sock address details 
  //to the sockaddr_in struct we delared above
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(PORT);
  servaddr.sin_family = AF_INET;

  // bind server address to socket descriptor
  bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
  
  //receive the datagram
  len = sizeof(cliaddr);

  //we receive on this socket and save the client message and address 
  int n = recvfrom(listenfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&cliaddr, &len);
  if (n < 0){
    perror("recvfrom");
    exit(1);
  }
  
  //receive message from server
  buffer[n] = '\0';

  //Print the recvd message
  puts(buffer);

  //send the response
  sendto(listenfd, message, strlen(message), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
  return 0;
}
