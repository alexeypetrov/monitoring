#include "url_parser.h"

UrlParser::UrlParser(const size_t numThreads, size_t timeout,
                     const std::shared_ptr<DatabaseInterface>& dB,
                     std::function<std::unique_ptr<HttpClientInterface>(
                         const std::string&, size_t)>
                         httpClientFactory)
    : m_num_threads(numThreads),
      m_timeout(timeout),
      db(dB),
      http_client_factory(std::move(httpClientFactory)) {
    for (size_t i = 0; i < m_num_threads; i++) {
        threads.emplace_back(&UrlParser::worker, this);
    }
}
UrlParser::~UrlParser() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        m_stop = true;
    }
    cv.notify_all();
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}
void UrlParser::addUrls(const int requestId,
                        const std::vector<std::string>& url) {
    std::lock_guard<std::mutex> lock(mtx);
    for (size_t i = 0; i < url.size(); i++) {
        m_queue.push(Url{requestId, url.at(i)});
    }
    cv.notify_all();
}
void UrlParser::worker() {
    while (true) {
        Url url;
        {
            std::unique_lock lock(mtx);
            cv.wait(lock, [this] { return !m_queue.empty() || m_stop; });
            if (m_stop && m_queue.empty()) {
                break;
            }
            if (m_queue.empty()) {
                continue;
            }
            url = m_queue.front();
            m_queue.pop();
        }
        auto httpClient = http_client_factory(url.url, m_timeout);
        url.http_status = httpClient->getHttpStatus();
        url.response_time = httpClient->getRequestTime();
        db->insert(url);
    }
}
