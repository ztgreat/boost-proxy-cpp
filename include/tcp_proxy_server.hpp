#ifndef PROXY_TCP_PROXY_SERVER_HPP
#define PROXY_TCP_PROXY_SERVER_HPP

#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include "tcp_proxy_bridge.hpp"
#include "route_locator.hpp"

namespace proxy {
    namespace tcp_proxy {
        namespace ip = boost::asio::ip;

        class server {
        public:

            server(boost::asio::io_service &io_service,
                   const std::string &local_host, unsigned short local_port,size_t);

            bool accept_connections();

            void set_route_locators(std::vector<route_locator *> &);

            ~server() {
                for (auto it = this->route_locators->begin(); it != this->route_locators->end(); it++) {
                    delete *it;
                }
                this->route_locators->clear();
                this->route_locators->shrink_to_fit();
                delete this->route_locators;
            }

        private:

            void handle_accept(const boost::system::error_code &error);

            boost::asio::io_service &io_service_;
            ip::address_v4 localhost_address;
            size_t backlog;
            ip::tcp::acceptor acceptor_;
            std::vector<route_locator *> *route_locators;
            proxy::tcp_proxy::bridge::ptr_type session_;
        };

    }
}
#endif
