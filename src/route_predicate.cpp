#include <string>
#include <vector>
#include <memory>
#include "request.hpp"
#include "route_predicate.hpp"

namespace proxy {


    proxy::route_predicate::route_predicate(const std::string &type, const std::string &rule) {
        this->rule = rule;
        this->type = type;
    }

    bool proxy::route_predicate::match(const proxy::http::request &request, std::vector<std::string> &param) {
        //return proxy::regex_find(*this->re2_engine, request.uri, param);
        return true;
    }


}
