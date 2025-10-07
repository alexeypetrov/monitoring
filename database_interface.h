#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include "url.h"

class DatabaseInterface {
   public:
    virtual ~DatabaseInterface() = default;

    [[nodiscard]] virtual size_t getRequestId(const std::string& content) const = 0;
    virtual bool insert(const Url& url) = 0;
    virtual std::vector<Url> find(const int requestId) = 0;
};
