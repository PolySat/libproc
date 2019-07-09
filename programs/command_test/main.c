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

#include <time.h>
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
    int tmp;
   int sfd;
    char word[10] = "hello\n";

    //os picks port
   sfd = socket_init(0);
    if(sfd == -1){
        printf("init failed!\n");

    }

   addr.sin_family = AF_INET;
   inet_pton(AF_INET, "127.0.0.1",&addr.sin_addr );
    //server specific port
   addr.sin_port = htons(5001);

   while (1)
   {
    sleep(1);
      buf[BUF_LEN] = 0;
    
        if(strlen(word) != (tmp = socket_write(sfd, word, strlen(word), &addr))){
            printf("size was wrong, attempted to write %zd bytes, returned %d\n",strlen(word),tmp); 
        }
      printf("wrote %s to socket\n",word);
   }

   socket_close(sfd);

   return 0;
}
#ifdef __cplusplus
}
#endif
