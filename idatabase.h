#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include "url.h"

class IDatabase {
   public:
    virtual ~IDatabase() = default;

    virtual size_t GetRequestId(const std::string& content) const = 0;
    virtual bool Insert(const Url& url) = 0;
    virtual std::vector<Url> GetUrls(const int requestId) = 0;
};
