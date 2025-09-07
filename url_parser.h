#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include "idatabase.h"
#include "ihttp_client.h"
#include "url.h"

class UrlParser {
   public:
    explicit UrlParser(
        const size_t numThreads, size_t timeout,
        const std::shared_ptr<IDatabase>& dB,
        std::function<std::unique_ptr<IHttpClient>(const std::string&, size_t)> httpClientFactory)
        : m_num_threads(numThreads), m_timeout(timeout), db(dB), http_client_factory(std::move(httpClientFactory)) {
        for (size_t i = 0; i < m_num_threads; i++) {
            std::thread t(&UrlParser::worker, this);
            t.detach();
        }
    }
    void AddUrls(const int requestId, const std::vector<std::string>& url) {
        std::lock_guard<std::mutex> lock(mtx);
        for (size_t i = 0; i < url.size(); i++) {
            m_queue.push(Url{requestId, url.at(i)});
        }
        cv.notify_one();
    }

   private:
    void worker() {
        while (true) {
            Url url;
            {
                std::unique_lock lock(mtx);
                cv.wait(lock, [this] { return !m_queue.empty(); });
                if (m_queue.empty()) {
                    break;
                }
                url = m_queue.front();
                m_queue.pop();
            }
            auto httpClient = http_client_factory(url.url, m_timeout);
            url.http_status = httpClient->getHttpStatus();
            url.response_time = httpClient->getRequestTime();
            db->Insert(url);
        }
    }
    size_t m_num_threads;
    size_t m_timeout;
    std::queue<Url> m_queue;
    std::mutex mtx;
    std::condition_variable cv;
    std::shared_ptr<IDatabase> db;
    std::function<std::unique_ptr<IHttpClient>(const std::string&, size_t)> http_client_factory;
};
