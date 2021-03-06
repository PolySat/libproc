#include "zhelpers.hpp"
#include <iostream>

int main(int argc, char **argv)
{
   char buff[1024];

   if (argc < 2) {
      printf("Usage: %s <port number>\n", argv[0]);
      return 0;
   }

    //  Create context
    zmq::context_t context(1);

    // Create client socket
    zmq::socket_t client (context, ZMQ_PAIR);
    sprintf(buff, "tcp://localhost:%s", argv[1]);

    client.connect(buff);
    // s_send (client, "{\"command\":\"run\"}");
    s_send (client, "{\"command\":\"set_fd_breakpoint\",\"fd\":8}");

    while (1) {
       std::string msg = s_recv (client);
       std::cout << "Received: " << msg << std::endl;
       sleep(3);
       s_send (client, "{\"command\":\"next\"}");
    }

    client.close();
    context.close();

    return 0;
}
