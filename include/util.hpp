#ifndef PROXY_UTIL_HPP
#define PROXY_UTIL_HPP

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <string>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace proxy {

    namespace util {

        std::vector<std::string> split(const std::string &s, char delimiter);

    }
}
#endif
