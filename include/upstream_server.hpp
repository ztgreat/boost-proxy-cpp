#ifndef PROXY_UPSTREAM_SERVER_HPP
#define PROXY_UPSTREAM_SERVER_HPP

#include <string>

namespace proxy {

    class upstream_server {
    public:
        std::string server;
        int port;
    public:
        upstream_server() = delete;

        upstream_server(const std::string &, int port);

        virtual ~upstream_server() = default;
    };

}

#endif