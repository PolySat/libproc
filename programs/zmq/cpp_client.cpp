#include "zhelpers.hpp"
#include <iostream>

#define big_message "0123456789abcdef0123456789abcdef" \
                    "0123456789abcdef0123456789abcdef" \
                    "0123456789abcdef0123456789abcdef" \
                    "0123456789abcdef0123456789abcdef" \
                    "0123456789abcdef0123456789abcdef" \
                    "0123456789abcdef0123456789abcdef" \
                    "0123456789abcdef0123456789abcdef" \
                    "0123456789abcdef0123456789abcdef" \
                    "foo!"

int main() {
    //  Create context
    zmq::context_t context(1);

    // Create client socket
    zmq::socket_t client (context, ZMQ_PAIR);
    client.connect("tcp://localhost:12345");

    // Send message from server to client
    // s_send (server, "My Message");
    s_send (client, "My Message");
    std::string msg = s_recv (client);
    std::cout << "Received: " << msg << std::endl;

    s_send (client, big_message);
    msg = s_recv (client);
    std::cout << "Received: " << msg << std::endl;

    client.close();
    context.close();

    return 0;
}
