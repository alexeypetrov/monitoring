#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include "database_interface.h"
#include "http_client_interface.h"
#include "url.h"

class UrlParser {
   public:
    explicit UrlParser(const size_t numThreads, size_t timeout,
                       const std::shared_ptr<DatabaseInterface>& dB,
                       std::function<std::unique_ptr<HttpClientInterface>(
                           const std::string&, size_t)>
                           httpClientFactory);
    ~UrlParser();
    void addUrls(const int requestId, const std::vector<std::string>& url);

   private:
    void worker();
    size_t m_num_threads;
    size_t m_timeout;
    std::queue<Url> m_queue;
    std::mutex mtx;
    std::condition_variable cv;
    bool m_stop = false;
    std::shared_ptr<DatabaseInterface> db;
    std::function<std::unique_ptr<HttpClientInterface>(const std::string&, size_t)> http_client_factory;
    std::vector<std::thread> threads;
};
