#pragma once

#include "http_client_interface.h"

class TestHttpClient : public HttpClientInterface {
   public:
    [[nodiscard]] size_t getHttpStatus() const override {
        return 200;
    }

   [[nodiscard]] long long getRequestTime() const override {
        return 100;
   };
};
