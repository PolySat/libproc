#include "zmqlite.h"
#include "events.h"
#include "ipc.h"
#include <errno.h>
#include "debug.h"
#include <unistd.h>

static int zmql_client_raw_write(struct ZMQLClient *client, const void *data,
      int dlen);

enum ZMQLState { ZMQL_SIGNATURE, ZMQL_VERS1, ZMQL_AUTH, ZMQL_DATA };

static char ZMQ_SIGNATURE_DATA[] = { 0xFF, 0, 0, 0, 0, 0, 0, 0, 1, 0x7F };
#define ZMQ_SIGNATURE_LEN 10
static char ZMQ_VERS_DATA[] = { 0x03 };
#define ZMQ_VERS_LEN 1
static char ZMQ_AUTH_DATA[] = { 0, 'N', 'U', 'L', 'L', 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0 };
#define ZMQ_AUTH_LEN 53
static char ZMQ_HANDSHAKE_DATA[] = { 0x04, 0x1A, 0x05, 'R', 'E', 'A', 'D', 'Y',
                                     0x0B, 'S', 'o', 'c', 'k', 'e', 't', '-',
                                     'T', 'y', 'p', 'e', 0, 0, 0, 0x04,
                                     'P', 'A', 'I', 'R' };
#define ZMQ_HANDSHAKE_LEN 28

#define ZMQ_MORE_DATA 0x1
#define ZMQ_LONG_SIZE 0x2
#define ZMQ_COMMAND 0x4

#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))


struct ZMQLClient {
   int socket;
   enum ZMQLState state;
   struct sockaddr_in addr;
   struct IPCBuffer *data;
   struct ZMQLServer *server;
   struct ZMQLClient *next;
   int closeFlag;
   zmql_client_message_cb msgCb;
   zmql_client_status_cb connectCb, disconnectCb;
   void *msgArg;
};

struct ZMQLServer {
   int socket;
   int clientCount;
   struct ZMQLClient *clients;
   zmql_client_message_cb msgCb;
   zmql_client_status_cb connectCb, disconnectCb;
   void *msgArg;
   EVTHandler *evt;
};

int zmql_server_socket(struct ZMQLServer *server)
{
   if (server)
      return server->socket;

   return -1;
}

static size_t zmql_client_process(const char *data, size_t dataLen, void *arg)
{
   struct ZMQLClient *client = (struct ZMQLClient*)arg;
   unsigned char flags;
   uint64_t payloadLen = 0;
   int dataOffset = 0;

   if (client->state == ZMQL_SIGNATURE) {
      if (dataLen < ZMQ_SIGNATURE_LEN)
         return 0;
      if (0 == memcmp(data, ZMQ_SIGNATURE_DATA, ZMQ_SIGNATURE_LEN)) {
         client->state = ZMQL_VERS1;
         zmql_client_raw_write(client, ZMQ_VERS_DATA, ZMQ_VERS_LEN);
         return ZMQ_SIGNATURE_LEN;
      }
      else {
         DBG_print(DBG_LEVEL_WARN, "Bad ZMQ signature, closing");
         client->closeFlag = 1;
         return 0;
      }
   }
   else if (client->state == ZMQL_VERS1) {
      if (dataLen < ZMQ_VERS_LEN)
         return 0;
      if (0 == memcmp(data, ZMQ_VERS_DATA, ZMQ_VERS_LEN)) {
         client->state = ZMQL_AUTH;
         zmql_client_raw_write(client, ZMQ_AUTH_DATA, ZMQ_AUTH_LEN);
         return ZMQ_VERS_LEN;
      }
      else {
         DBG_print(DBG_LEVEL_WARN, "Bad ZMQ version, closing");
         client->closeFlag = 1;
         return 0;
      }
   }
   else if (client->state == ZMQL_AUTH) {
      if (dataLen < ZMQ_AUTH_LEN)
         return 0;
      if (0 == memcmp(data, ZMQ_AUTH_DATA, ZMQ_AUTH_LEN)) {
         zmql_client_raw_write(client, ZMQ_HANDSHAKE_DATA, ZMQ_HANDSHAKE_LEN);
         client->state = ZMQL_DATA;
         if (client->connectCb)
            client->connectCb(client, client->msgArg);
         return ZMQ_AUTH_LEN;
      }
      else {
         DBG_print(DBG_LEVEL_WARN, "Bad ZMQ authentication, closing");
         client->closeFlag = 1;
         return 0;
      }
   }
   else if (client->state == ZMQL_DATA) {
      if (dataLen < 1)
         return 0;

      // Parse flags to determine message length
      flags = data[0];
      if (flags & ZMQ_LONG_SIZE) {
         dataOffset = 9;
         if (dataLen < dataOffset)
            return 0;
         memcpy(&payloadLen, &data[1], sizeof(payloadLen));
         payloadLen = ntohll(payloadLen);
      }
      else {
         dataOffset = 2;
         if (dataLen < dataOffset)
            return 0;
         payloadLen = data[1];
      }

      // Make sure we have the entire message buffered
      if (dataLen < (dataOffset + payloadLen))
         return 0;

      if (!(flags & ZMQ_COMMAND)) {
         if (client->msgCb)
            client->msgCb(client, &data[dataOffset], payloadLen,
                          client->msgArg);
      }

      return dataOffset + payloadLen;
   }

   return 0;
}

int zmql_client_read_cb(int fd, char type, void *arg)
{
   struct ZMQLClient *client = (struct ZMQLClient*)arg;
   int len;
   char buff[2048];

   client->closeFlag = 0;
   len = read(fd, buff, sizeof(buff));
   if (len == -1) {
      if (errno == EAGAIN)
         return EVENT_KEEP;
      else
         ERR_REPORT(DBG_LEVEL_WARN, "Error while reading zmql client socket.  Closing.");
   }
   if (len <= 0) {
      DBG_print(DBG_LEVEL_INFO, "Destroying zmql client %p.", client);
      zmql_destroy_client(&client);
      return EVENT_REMOVE;
   }

   ipc_append_buffer(client->data, buff, len);
   ipc_process_buffer(client->data, zmql_client_process, client);

   if (client->closeFlag) {
      zmql_destroy_client(&client);
      return EVENT_REMOVE;
   }

   return EVENT_KEEP;
}

int zmql_accept_cb(int fd, char type, void *arg)
{
   struct ZMQLServer *server = (struct ZMQLServer*)arg;
   struct ZMQLClient *client;
   socklen_t len = sizeof(client->addr);

   client = (struct ZMQLClient*)malloc(sizeof(struct ZMQLClient));
   if (!client)
      return EVENT_KEEP;
   memset(client, 0, sizeof(*client));

   client->socket = accept(fd, &client->addr, &len);
   if (client->socket == -1) {
      DBG_print(DBG_LEVEL_WARN, "Failed to accept zmql client.");
      free(client);
      return EVENT_KEEP;
   }

   server->clientCount++;
   client->server = server;
   client->next = server->clients;
   server->clients = client;
   client->data = ipc_alloc_buffer();
   client->state = ZMQL_SIGNATURE;
   client->msgCb = server->msgCb;
   client->msgArg = server->msgArg;
   client->disconnectCb = server->disconnectCb;
   client->connectCb = server->connectCb;

   EVT_fd_add(server->evt, client->socket, EVENT_FD_READ,
         zmql_client_read_cb, client);
   EVT_fd_set_name(server->evt, client->socket, "ZMQ Client %s:%u",
         inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port));

   zmql_client_raw_write(client, ZMQ_SIGNATURE_DATA, ZMQ_SIGNATURE_LEN);

   return EVENT_KEEP;
}

struct ZMQLServer *zmql_create_tcp_server(EVTHandler *evt, int port,
      zmql_client_message_cb cb, zmql_client_status_cb con_cb,
      zmql_client_status_cb discon_cb, void *arg)
{
   struct ZMQLServer *server;

   server = (struct ZMQLServer*) malloc(sizeof(*server));
   if (!server)
      return NULL;

   memset(server, 0, sizeof(*server));

   server->socket = socket_tcp_init(port);
   if (server->socket <= 0) {
      free(server);
      return NULL;
   }
   server->evt = evt;
   server->msgCb = cb;
   server->msgArg = arg;
   server->connectCb = con_cb;
   server->disconnectCb = discon_cb;

   EVT_fd_add(evt, server->socket, EVENT_FD_READ, zmql_accept_cb, server);
   EVT_fd_set_name(evt, server->socket, "ZMQ Server port %d", port);

   return server;
}

void zmql_destroy_tcp_server(struct ZMQLServer **goner)
{
   struct ZMQLServer *server;
   struct ZMQLClient *client;

   if (!goner)
      return;
   server = *goner;
   *goner = NULL;
   if (!server)
      return;

   if (server->socket > 0) {
      EVT_fd_remove(server->evt, server->socket, EVENT_FD_READ);
      close(server->socket);
   }

   while ((client = server->clients))
      zmql_destroy_client(&client);

   free(server);
}

void zmql_destroy_client(struct ZMQLClient **goner)
{
   struct ZMQLClient **itr, *client;

   if (!goner)
      return;
   client = *goner;
   *goner = NULL;
   if (!client)
      return;

   for (itr = &client->server->clients; itr && *itr; itr = &((*itr)->next)) {
      if (*itr == client) {
         *itr = client->next;
         break;
      }
   }

   if (!client->closeFlag)
      EVT_fd_remove(client->server->evt, client->socket, EVENT_FD_READ);
   if (client->state == ZMQL_DATA && client->disconnectCb)
      client->disconnectCb(client, client->msgArg);

   if (client->server)
      client->server->clientCount--;

   close(client->socket);
   if (client->data)
      ipc_destroy_buffer(&client->data);

   free(client);
}

static int zmql_client_raw_write(struct ZMQLClient *client, const void *data,
      int dlen)
{
   int len;

   len = write(client->socket, data, dlen);

   if (len != dlen)
      DBG_print(DBG_LEVEL_WARN, "Short ZMQ write");

   return len;
}

int zmql_write_buffer(struct ZMQLClient *client, struct IPCBuffer *msg)
{
   uint64_t len;
   char header[10];
   int hlen = 2;

   if (client->state != ZMQL_DATA)
      return 0;

   len = ipc_buffer_size(msg);

   if (len <= 255) {
      header[0] = 0;
      header[1] = len;
   }
   else {
      header[0] = ZMQ_LONG_SIZE;
      len = htonll(len);
      memcpy(&header[1], &len, sizeof(len));
      hlen = 9;
   }

   zmql_client_raw_write(client, header, hlen);
   ipc_write_buffer_sync(client->socket, msg);

   return 0;
}

void zmql_broadcast_buffer(struct ZMQLServer *server, struct IPCBuffer *msg)
{
   struct ZMQLClient *client;

   for (client = server->clients; client; client = client->next)
      zmql_write_buffer(client, msg);
}

int zmql_client_count(struct ZMQLServer *server)
{
   if (!server)
      return 0;
   return server->clientCount;
}

struct ZMQLServer *zmql_server_for_client(struct ZMQLClient *client)
{
   if (client)
      return client->server;

   return NULL;
}
