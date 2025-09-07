#pragma once

#include <cstddef>

class IHttpClient {
   public:
    virtual ~IHttpClient() = default;

    [[nodiscard]] virtual size_t getHttpStatus() const = 0;
    virtual long long getRequestTime() const = 0;
};
