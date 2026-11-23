#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>

#define PORT 5000
#define MAXLINE 1000

// Driver code 
int main()
{
  char buffer[100];
  char *message = "Hello Server";
  int sockfd, n;
  struct sockaddr_in servaddr;

  // clear servaddr
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  servaddr.sin_port = htons(PORT);
  servaddr.sin_family = AF_INET;

  // create datagram socket
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  // connect to Server
  if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
  {
    printf("\n Error : Connect Failed \n");
    exit(0);
  }

  // request to send datagram
  // no need to specify server address in sendto
  // connect stores that peers IP and sin_port
if (sendto(sockfd, message, strlen(message), 0, NULL, 0) < 0) {
    perror("sendto");
    exit(1);
}

  // receive from server
  n = recvfrom(sockfd, buffer, sizeof(buffer)-1, 0, NULL, NULL);
  if (n < 0){
    perror("recvfrom");
    exit(1);
  }
  buffer[n] = '\0';
  puts(buffer);

  // close the descriptor
  close(sockfd);
}
