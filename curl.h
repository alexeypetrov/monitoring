#pragma once

#include <curl/curl.h>
#include <memory>
#include <string>
#include "http_client_interface.h"

class Curl : public HttpClientInterface {
   public:
    Curl(const std::string& url, const size_t timeout);
    ~Curl() override = default;
    [[nodiscard]] size_t getHttpStatus() const override;

    [[nodiscard]] long long getRequestTime() const override;

   private:
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl;
};
