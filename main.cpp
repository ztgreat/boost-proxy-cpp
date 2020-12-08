
#include <cstdlib>
#include <iostream>
#include <string>
#include <route_locator.hpp>
#include <wait.h>
#include "tcp_proxy_server.hpp"
#include "include/yaml-cpp/yaml.h"
#include "route_predicate.hpp"
#include "proxy_type.h"
#include "util.hpp"
#include "multi_process.hpp"

int main(int argc, char *argv[]) {

    YAML::Node config = YAML::LoadFile("../config.yaml");
    const std::string host = config["server.host"].as<std::string>();
    int port = config["server.port"].as<std::int32_t>();
    int backlog = config["server.backlog"].as<std::int32_t>();
    const std::string mode = config["server.mode"].as<std::string>();

    //暂时没用
    proxy::proxy_type proxy_type;
    if (strcmp(mode.c_str(), "TCP") == 0 || strcmp(mode.c_str(), "tcp") == 0) {
        proxy_type = proxy::proxy_type::TCP;
    } else {
        proxy_type = proxy::proxy_type::HTTP;
    }

    const std::string worker_processes_str = config["server.worker.processes"].as<std::string>();
    unsigned int worker_processes = std::thread::hardware_concurrency();
    if (strcmp(worker_processes_str.c_str(), "auto") != 0) {
        worker_processes = std::stoi(worker_processes_str);
    }

    const YAML::Node &routes = config["routes"];
    auto route_locators = new std::vector<proxy::route_locator *>();
    for (auto it = routes.begin(); it != routes.end(); ++it) {
        const YAML::Node &route = *it;
        auto routeLocator = new proxy::route_locator;
        auto *loadBalance = new proxy::load_balance;
        const YAML::Node &upServers = route["uri"];
        for (auto it2 = upServers.begin(); it2 != upServers.end(); ++it2) {
            const YAML::Node &upServer = *it2;
            std::string url = upServer.as<std::string>();

            std::string::size_type first_pos = url.find_first_of(':');

            // upstream
            if (first_pos == -1) {
                auto temp = new proxy::upstream_server(url, 80);
                loadBalance->add_upstream_server(temp);
            } else {
                auto temp = new proxy::upstream_server(url.substr(0, first_pos), std::stoi(
                        url.substr(first_pos + 1, url.length() - first_pos - 1)));
                loadBalance->add_upstream_server(temp);
            }
        }

        const YAML::Node &predicates = route["predicates"];
        for (auto it2 = predicates.begin(); it2 != predicates.end(); ++it2) {
            const YAML::Node &node_predicate = *it2;
            // predicate
            std::string predicate = node_predicate.as<std::string>();
            std::vector<std::string> temp = proxy::util::split(predicate, '=');

            if (temp.size() != 2) {
                continue;
            }
            if (std::strcmp(temp[0].c_str(), "path") == 0) {
                auto path_predicate = new proxy::route_predicate("path", temp[1]);
                routeLocator->addPredicate(path_predicate);
            }
        }
        routeLocator->setLoadBalance(loadBalance);
        route_locators->insert(route_locators->end(), routeLocator);
    }

    try {
        boost::asio::io_service ios;
        proxy::tcp_proxy::server server(ios,
                                        host, port, backlog);
        server.set_route_locators(*route_locators);
        server.run();
    } catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    std::function<void(pthread_mutex_t *, size_t *)> ff = [&](pthread_mutex_t *mtx, size_t *data) {
        try {
            boost::asio::io_service ios;
            proxy::tcp_proxy::server server(ios,
                                            host, port, backlog);
            server.set_route_locators(*route_locators);
            server.run();
        } catch (std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    };
    std::function<bool(int)> g = [&](int status) {
        std::cout << strsignal(WTERMSIG(status)) << std::endl;
        return false;
    };
    proxy::process::multi_process main_process;
    main_process.run(ff, g, worker_processes);
    return 0;
}
