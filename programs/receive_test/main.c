/**
 * Test code for process command handling.
 * 
 * Don't input any lines over BUF_LEN or a buffer overrun will occur.
 *
 * @author Greg Eddington
 */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <polysat/polysat.h>
/*#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>*/


#define BUF_LEN 2048
/*
int socket_close(int fd);
int socket_write(int fd, void * buf, size_t bufSize, struct sockaddr_in * dest);
int socket_init(int port);
*/

int main(int argc, char *argv[])
{
   struct sockaddr_in addr;
   char buf[BUF_LEN+1];
   int sfd;

   sfd = socket_init(0);

   addr.sin_family = AF_INET;
   inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
   addr.sin_port = htons(5001);

   while (scanf("%s",buf) == 1)
   {
      buf[BUF_LEN] = 0;
      socket_read(sfd, buf, strlen(buf), &addr);
      printf("socket output:%s\n ",buf);
   }

   socket_close(sfd);

   return 0;
}
#ifdef __cplusplus
}
#endif
