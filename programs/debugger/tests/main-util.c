/**
 * Test code for process command handling.
 * 
 * @author Greg Eddington
 */
#include <stdint.h>
#include <stdio.h>
#include "proclib.h"

#define WAIT_MS 1500

struct MulticallInfo;

int main(int argc, char **argv)
{
   struct {
      uint8_t cmd;
      char status;
   } __attribute__((packed)) resp;
   int len;
   unsigned char cmd[1];
   const char *ip = "127.0.0.1";
   int opt;

#ifdef __APPLE__
   optind = 1;
#else
   optind = 0;
#endif

   cmd[0] = 1;

   while ((opt = getopt(argc, argv, "h:")) != -1) {
      switch(opt) {
         case 'h':
            ip = optarg;
            break;
         default:
            return 0;
      }
   }

   if (optind < argc) {
      printf("command line parameter error\n");
      return 0;
   }

   len = socket_send_packet_and_read_response(ip, "test1", &cmd, sizeof(cmd),
      &resp, sizeof(resp), WAIT_MS);

   if (len < 1)
      return len;

   if (resp.cmd != 0xF1) {
      printf("response code incorrect.  Got 0x%02X expected 0x%02X\n",
            resp.cmd, 0xF1);
      return 5;
   }

   printf("Beacon response: %d\n", resp.status);

   return 0;
}
