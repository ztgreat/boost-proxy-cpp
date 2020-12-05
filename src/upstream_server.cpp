#include <string>
#include "upstream_server.hpp"

namespace proxy {

    proxy::upstream_server::upstream_server(const std::string &server, int port) {
        this->server = server;
        this->port = port;
    }

}