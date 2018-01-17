/**
 * Test code for process command handling.
 * 
 * Don't input any lines over BUF_LEN or a buffer overrun will occur.
 *
 * @author Greg Eddington
 */

#include "ipc.h"
#include <stdio.h>

#define BUF_LEN 2048

int main(int argc, char *argv[])
{
   struct sockaddr_in addr;
   char buf[BUF_LEN+1];
   int sfd;

   sfd = socket_init(0);

   addr.sin_family = AF_INET;
   inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
   addr.sin_port = htons(5001);

   while (scanf("%s", buf) == 1)
   {
      buf[BUF_LEN] = 0;
      socket_write(sfd, buf, strlen(buf), &addr);
   }

   socket_close(sfd);

   return 0;
}
