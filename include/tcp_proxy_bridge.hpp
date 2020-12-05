#ifndef PROXY_TCP_PROXY_BRIDGE_HPP
#define PROXY_TCP_PROXY_BRIDGE_HPP

#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>

namespace proxy {
    namespace tcp_proxy {
        namespace ip = boost::asio::ip;

        class bridge : public boost::enable_shared_from_this<bridge> {
        public:

            typedef ip::tcp::socket socket_type;

            typedef boost::shared_ptr<bridge> ptr_type;

            bridge(boost::asio::io_service &ios);

            socket_type &downstream_socket() {
                // client socket
                return downstream_socket_;
            }

            socket_type &upstream_socket() {
                // up server socket
                return upstream_socket_;
            }

            void start(const std::string &upstream_host, unsigned short upstream_port);

            void handle_upstream_connect(const boost::system::error_code &error);

        private:

            void handle_upstream_read(const boost::system::error_code &error,
                                      const size_t &bytes_transferred);

            void handle_downstream_write(const boost::system::error_code &error);

            void handle_downstream_read(const boost::system::error_code &error,
                                        const size_t &bytes_transferred);

            void handle_upstream_write(const boost::system::error_code &error);

            void close();

            socket_type downstream_socket_;

            socket_type upstream_socket_;

            enum {
                //暂时设置 1500
                max_data_length = 1500
            };
            unsigned char downstream_data_[max_data_length];
            unsigned char upstream_data_[max_data_length];
            boost::mutex mutex_;

        };
    }
}
#endif