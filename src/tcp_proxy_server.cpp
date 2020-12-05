#include <iostream>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <tcp_proxy_server.hpp>

namespace proxy {
    namespace tcp_proxy {
        namespace ip = boost::asio::ip;

        proxy::tcp_proxy::server::server(boost::asio::io_service &io_service,
                                         const std::string &local_host, unsigned short local_port,
                                         size_t back_log = 4096)
                : io_service_(io_service),
                  localhost_address(boost::asio::ip::address_v4::from_string(local_host)),
                  backlog(back_log),
                  acceptor_(io_service_, ip::tcp::endpoint(localhost_address, local_port)) {
            this->acceptor_.listen(backlog);
            boost::asio::ip::tcp::acceptor::reuse_address reuse_address(true);
            boost::asio::ip::tcp::no_delay no_delay(true);
            acceptor_.set_option(reuse_address);
            acceptor_.set_option(no_delay);
        }

        void proxy::tcp_proxy::server::set_route_locators(std::vector<route_locator *> &route_locator) {
            this->route_locators = &route_locator;
        }

        bool proxy::tcp_proxy::server::accept_connections() {
            try {
                session_ = boost::shared_ptr<tcp_proxy::bridge>(new bridge(io_service_));
                acceptor_.async_accept(session_->downstream_socket(),
                                       boost::bind(&server::handle_accept,
                                                   this,
                                                   boost::asio::placeholders::error));
            }
            catch (std::exception &e) {
                std::cerr << "acceptor exception: " << e.what() << std::endl;
                return false;
            }
            return true;
        }

        void proxy::tcp_proxy::server::handle_accept(const boost::system::error_code &error) {
            if (!error) {
                proxy::upstream_server *upstreamServer = nullptr;
                for (auto route : *(this->route_locators)) {
                    upstreamServer = route->choseServer(nullptr);
                    if (upstreamServer) {
                        break;
                    }
                }
                if (upstreamServer == nullptr) {
                    session_->downstream_socket().close();
                    return;
                }
                session_->start(upstreamServer->server, upstreamServer->port);
                if (!accept_connections()) {
                    std::cerr << "accept fail" << std::endl;
                }
            } else {
                std::cerr << "error: " << error.message() << std::endl;
            }
        }

    }
}