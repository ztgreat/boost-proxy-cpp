#ifndef PROXY_ROUTE_PREDICATE_HPP
#define PROXY_ROUTE_PREDICATE_HPP

#include <string>
#include "request.hpp"

namespace proxy {
    class route_predicate {
    public:
        std::string type;
        std::string rule;

        route_predicate() = delete;

        route_predicate(const std::string &type, const std::string &rule);

        virtual bool match(const proxy::http::request &request, std::vector<std::string> &param);
    };
}
#endif
