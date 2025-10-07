#pragma once

#include <cstddef>

class HttpClientInterface {
   public:
    virtual ~HttpClientInterface() = default;

    [[nodiscard]] virtual size_t getHttpStatus() const = 0;
    [[nodiscard]] virtual long long getRequestTime() const = 0;
};
