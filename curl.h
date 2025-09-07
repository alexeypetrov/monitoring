#pragma once

#include <curl/curl.h>
#include <string>
#include <memory>
#include "ihttp_client.h"

class Curl : public IHttpClient {
   public:
    Curl(const std::string& url, const size_t timeout) : curl(curl_easy_init(), &curl_easy_cleanup) {
        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, timeout);
    };
    ~Curl() override = default;
    [[nodiscard]] size_t getHttpStatus() const override {
        const CURLcode res = curl_easy_perform(curl.get());
        size_t httpCode = 0;
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &httpCode);
        }

        return httpCode;
    }

    [[nodiscard]] long long getRequestTime() const override {
        double totalTime = 0.0;
        curl_easy_getinfo(curl.get(), CURLINFO_TOTAL_TIME, &totalTime);
        return static_cast<long long>(totalTime * 1000.0);
    }

   private:
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl;
};
