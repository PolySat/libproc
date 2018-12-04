#ifndef ZMQLITE_H
#define ZMQLITE_H

#include <unistd.h>

struct ZMQLServer;
struct ZMQLClient;
struct EventState;
struct IPCBuffer;

typedef int (*zmql_client_message_cb)(struct ZMQLClient *client,
      const void *data, size_t dataLen, void *arg);
typedef void (*zmql_client_status_cb)(struct ZMQLClient *client, void *arg);

struct ZMQLServer *zmql_create_tcp_server(struct EventState *evt, int port,
      zmql_client_message_cb cb, zmql_client_status_cb connect_cb,
      zmql_client_status_cb disconnect_cb, void *arg);
void zmql_destroy_tcp_server(struct ZMQLServer **goner);
void zmql_destroy_client(struct ZMQLClient **goner);
void zmql_broadcast_buffer(struct ZMQLServer *server, struct IPCBuffer *msg);
int zmql_write_buffer(struct ZMQLClient *client, struct IPCBuffer *msg);
int zmql_client_count(struct ZMQLServer *server);
int zmql_server_socket(struct ZMQLServer *server);
struct ZMQLServer *zmql_server_for_client(struct ZMQLClient *client);

#endif
