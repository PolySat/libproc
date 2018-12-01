/*
 * Copyright PolySat, California Polytechnic State University, San Luis Obispo. cubesat@calpoly.edu
 * This file is part of libproc, a PolySat library.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file ipc.c inter-process communication source file.
 *
 * Source for IPC component of the polysat library.
 *
 * @author Greg Manyak
 */
#include "ipc.h"
#include "debug.h"
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/file.h>
#include <strings.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "proclib.h"

#define WAIT_MS (5 * 1000)

// List of custom services for use if /etc/services lookup fails
static struct ServiceNames {
   char *name;
   int port;
   const char *multicast_ip;
   struct in_addr multicast_addr;
   int multicast_port;
} serverNameList[] = {
   { "beacon", 50000, "234.192.101.1", { 0 }, 51000 },
   { "sys_manager", 50001, "234.192.101.2", { 0 }, 51001 },
   { "watchdog", 50002, "234.192.101.3", { 0 }, 51002 },
   { "satcomm", 50003, "234.192.101.4", { 0 }, 51003 },
   { "filemgr", 50004, "234.192.101.5", { 0 }, 51004 },
   { "telemetry", 50005, "234.192.101.6", { 0 }, 51005 },
   { "datalogger", 50006, "234.192.101.7", { 0 }, 51006 },
   { "ethcomm", 50007, "234.192.101.8", { 0 }, 51007 },
   { "comm_server", 50008, "234.192.101.9", { 0 }, 51008 },
   { "clksync", 50009, "234.192.101.10", { 0 }, 51009 },
   { "payload", 50010, "234.192.101.11", { 0 }, 51010 },
   { "adcs", 50011, "234.192.101.12", { 0 }, 51011 },
   { "pscam", 50012, "234.192.101.13", { 0 }, 51012 },
   { "camera", 50012, "234.192.101.13", { 0 }, 51012 },
   { "gps", 50013, "234.192.101.14", { 0 }, 51013 },
   { "log_cleaner", 50014, "234.192.101.15", { 0 }, 51014 },
   { "test1", 2003, "224.0.0.1", { 0 }, 52003 },
   { "test2", 2004, "234.192.101.16", { 0 }, 52004 },
   { NULL, 0 },
};

// gets socket by service name
int socket_named_init(const char * service)
{
   // look up the port number for the service
   uint32_t portNum = 0;
   int fd = -1;

   if (service)
      portNum = socket_get_addr_by_name(service);
   // Acquire the socket file descriptor if the lookup succeeds
   if (portNum != -1) {
      DBG_print(DBG_LEVEL_INFO, "Binding socket on port %u\n", portNum);
      fd = socket_init(portNum);
   } else {
      DBG_print(DBG_LEVEL_WARN, "Failed to look up %s port number\n", service);
   }

   return fd;
}

// gets socket by port number
int socket_init(int port)
{
   struct sockaddr_in addr;
   int fd;

   // Make sure we can create the socket file desciptor before configuring
   if ((fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
      ERRNO_WARN("Failed to open socket\n");
   } else {
      // Configure socket to be non-blocking
      ERR_WARN(fcntl(fd, F_SETFL, O_NONBLOCK),
            "Failed to configure socket to be non-blocking\n");

      // Set up the socket address structure
      memset((char *) &addr, 0, sizeof(addr));
      // Select IPv4 Socket type
      addr.sin_family = AF_INET;
      // Convert the port from host ("readable") order to network
      addr.sin_port = htons(port);
      // Allow socket to receive messages from any IP address
      addr.sin_addr.s_addr = htonl(INADDR_ANY);

      // Set option to share the port if it's already in use
      int option = 1;
      ERR_WARN(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)), "set socket options");

      ERR_WARN(bind(fd, (const struct sockaddr *)&addr, sizeof(addr)),
            "Failed to bind socket on port %d\n", port);
   }

   return fd;
}

// reads data from a socket and gives src address and data len
int socket_read(int fd, void * buf, size_t bufSize,
      struct sockaddr_in * src)
{
   size_t size;
   socklen_t sockLen;
   src->sin_family = AF_INET;
   sockLen = sizeof(*src);

   ERR_WARN(size = recvfrom(fd, buf, bufSize, 0, (struct sockaddr *)src, &sockLen),
        "socket_read - recvfrom\n");

   return size;
}


// writes data on a socket to service name
int socket_named_write(int fd, void * buf, size_t bufSize, const char * name)
{
   struct sockaddr_in dest;

   // configure the socket address with the looked up port
   memset((char*)&dest, 0, sizeof(dest));
   // Select IPv4 Socket type
   dest.sin_family = AF_INET;
   // Look up the port corresponding to the name
   dest.sin_port = htons(socket_get_addr_by_name(name));
   // Use localhost ip as source of message
   dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

   return socket_write(fd, buf, bufSize, &dest);
}

// writes data on a socket to destination socket address
int socket_write(int fd, void * buf, size_t bufSize, struct sockaddr_in * dest)
{
   size_t size;

   ERR_WARN(size = sendto(fd, buf, bufSize, 0, (const struct sockaddr *)dest,
      sizeof(struct sockaddr_in)), "socket_write - sendto\n");

   return size;
}

// closes a socket
int socket_close(int fd)
{
   return close(fd);
}

// returns the multicast address associated with a system service
uint16_t socket_multicast_port_by_name(const char * service)
{
   // Look up in the /etc/services file for the port number
   struct ServiceNames *curr;

   for (curr = serverNameList; curr->name; curr++) {
      // look for a matching service name
      if (!strcmp(curr->name, service)) {
         return curr->multicast_port;
      }
   }

   // convert port number to host order
   return 0;
}

// returns the multicast address associated with a system service
struct in_addr socket_multicast_addr_by_name(const char * service)
{
   // Look up in the /etc/services file for the port number
   struct ServiceNames *curr;
   struct in_addr res = { 0 };

   for (curr = serverNameList; curr->name; curr++) {
      // look for a matching service name
      if (!strcmp(curr->name, service)) {
         if (!curr->multicast_addr.s_addr)
            inet_aton(curr->multicast_ip, &curr->multicast_addr);

         return curr->multicast_addr;
      }
   }

   // convert port number to host order
   return res;
}

// creates socket address by name
int socket_get_addr_by_name(const char * service)
{
   // Look up in the /etc/services file for the port number
   struct servent * serviceEntry = getservbyname(service, "udp");
   struct ServiceNames *curr;
   int port;

   // if lookup failed, use the internal list
   if (serviceEntry == 0) {
      for (curr = serverNameList; curr->name; curr++) {
         // look for a matching service name
         if (!strcmp(curr->name, service)) {
            return curr->port;
         }
      }

      port = atol(service);
      if (port > 0)
         return port;

      DBG_print(DBG_LEVEL_WARN,
         "socket_get_addr_by_name - service '%s' lookup failed\n", service);

      return -1;
   }

   // convert port number to host order
   return ntohs(serviceEntry->s_port);
}

// gets service name by socket address
int socket_get_name_by_addr(struct sockaddr_in * addr, char * buf, size_t bufSize)
{
   // Look up in the /etc/services file for the name corresponding to the port
   struct servent *serviceEntry = getservbyport(addr->sin_port, "udp");
   struct ServiceNames *curr;
   char *name = NULL;
   int nameLen = 0;
   int portNum = 0;

   // If lookup in /etc/services failed, try the internal list
   if (serviceEntry == NULL)
   {
      // convert the port number to host (human-readable) order for comparison
      portNum = ntohs(addr->sin_port);

      for (curr = serverNameList; curr->name; curr++) {
         // look for a matching port number
         if (curr->port == portNum) {
            name = curr->name;
            break;
         }
      }
   } else {
      name = serviceEntry->s_name;
   }

   // Check if lookup failed altogether
   if (!name) {
      DBG_print(DBG_LEVEL_WARN, "service on port %d lookup failed\n", ntohs(addr->sin_port));
      return -1;
   }

   // Ensure that buffer is large enough for name + null byte
   if ((nameLen = strlen(name)) >= bufSize) {
      DBG_print(DBG_LEVEL_WARN,
         "service lookup buffer too small for storing %s\n", name);
      return -1;
   } else {
      strcpy(buf, name);
   }

   return nameLen;
}


/// Uses select to wait for packet reception or timeout
static int wait_for_packet(int sock, int ms_dur)
{
   struct timeval end_time, now;
   struct timeval tmp_tv;
   fd_set read_set;
   int result;

   gettimeofday(&now, NULL);
   tmp_tv.tv_sec = ms_dur / 1000;
   tmp_tv.tv_usec = (ms_dur - tmp_tv.tv_sec * 1000) * 1000;

   timeradd(&tmp_tv, &now, &end_time);

   while (timercmp(&now, &end_time, <)) {
      timersub(&end_time, &now, &tmp_tv);

      FD_ZERO(&read_set);
      FD_SET(sock, &read_set);

      result = select(sock+1, &read_set, NULL, NULL, &tmp_tv);
      // Socket is ready to read
      if (result == 1)
         return 1;

      if (result < 0 && errno != EINTR)
         return -1;

      gettimeofday(&now, NULL);
   }

   return 0;
}
// Blocking read of a single UDP packet and sanity check the return value
static int read_response(int sock, void *rxResp, size_t rxRespLen)
{
   int len;
   struct sockaddr_in src;

   len = socket_read(sock, rxResp, rxRespLen, &src);
   if (len < 0) {
      perror("Error reading response packet");
      return -4;
   }
   else if (len != rxRespLen) {
      printf("Response packet not fully read\n");
      return -5;
   }

   return len;
}

// Uses blocking system calls to setup an IPC port, transmit the given
//  UDP data, and receive a single response.
int socket_send_packet_and_read_response(const char *dstAddr,
      const char *dstProc, void *txCmd, size_t txCmdLen,
      void *rxResp, size_t rxRespLen, int responseTimeoutMS)
{
   int sock;
   int result = 0;
   struct sockaddr_in addr;

   sock = socket_init(0);
   if (sock < 0)
      return -1;

   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = 0;

   if (dstAddr) {
      if (0 == socket_resolve_host(dstAddr, &addr.sin_addr))
         return -1;
   }

   if (0 == addr.sin_addr.s_addr)
      inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

   addr.sin_port = htons(socket_get_addr_by_name(dstProc));

   socket_write(sock, txCmd, txCmdLen, &addr);

   if (rxResp && rxRespLen > 0) {
      result = wait_for_packet(sock, responseTimeoutMS);
      if (result == 0) {
         printf("No response received in %d ms\n", responseTimeoutMS);
         result = -2;
      }
      else if (result != 1) {
         perror("Error waiting for reponse packet");
         result = -3;
      }
      else {
         result = read_response(sock, rxResp, rxRespLen);
      }
   }

   socket_close(sock);

   return result;
}

int socket_resolve_host(const char *host, struct in_addr *addr)
{
   struct hostent *hp;

   if (inet_aton(host, addr)!=0)
      return 1;

   if ((hp = gethostbyname(host)) == NULL) {
      perror("gethostbyname");
      return 0;
   }

   *addr = *(struct in_addr*)(hp->h_addr);

   return 1;
}
