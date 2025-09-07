#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include "idatabase.h"
#include "ihttp_client.h"
#include "url_parser.h"

using boost::asio::ip::tcp;
using json = nlohmann::json;

class HttpSession : public std::enable_shared_from_this<HttpSession> {
   public:
    HttpSession(tcp::socket socket, const std::shared_ptr<UrlParser>& urlParser, const std::shared_ptr<IDatabase>& db)
        : socket_(std::move(socket)), url_parser(urlParser), db(db) {}

    void start() { read_request(); }

   private:
    void read_request() {
        auto self(shared_from_this());
        boost::asio::async_read_until(
            socket_, buffer_, "\r\n\r\n",
            [this, self](boost::system::error_code ec, std::size_t) {
                if (!ec) {
                    std::istream request_stream(&buffer_);
                    request_stream >> method_ >> uri_ >> version_;

                    std::string line;
                    std::getline(request_stream, line);

                    std::string header;
                    while (std::getline(request_stream, header)) {
                        if (!header.empty() && header.back() == '\r') {
                            header.pop_back();
                        }
                        if (header.empty()) {
                            break;
                        }
                        if (header.find("Content-Length:") == 0) {
                            content_length_ = std::stoi(header.substr(15));
                        }
                        if (header.find("Content-Type:") == 0) {
                            content_type_ = header.substr(13);
                            while (!content_type_.empty() && isspace(content_type_.front())) {
                                content_type_.erase(0, 1);
                            }
                            while (!content_type_.empty() && isspace(content_type_.back())) {
                                content_type_.pop_back();
                            }
                        }
                    }

                    if (method_ == "POST" && content_length_ > 0) {
                        read_body();
                    } else {
                        handle_request();
                    }
                }
            });
    }

    void read_body() {
        auto self(shared_from_this());
        std::size_t bytes_already_in_buffer = buffer_.size();
        std::size_t bytes_to_read = (content_length_ > bytes_already_in_buffer) ? (content_length_ - bytes_already_in_buffer) : 0;

        if (bytes_to_read > 0) {
            boost::asio::async_read(
                socket_, buffer_, boost::asio::transfer_exactly(bytes_to_read),
                [this, self](boost::system::error_code ec, std::size_t) {
                    if (!ec) {
                        std::istream body_stream(&buffer_);
                        body_.assign(std::istreambuf_iterator<char>(body_stream), {});
                        handle_request();
                    }
                });
        } else {
            std::istream body_stream(&buffer_);
            body_.assign(std::istreambuf_iterator<char>(body_stream), {});
            handle_request();
        }
    }

    void handle_request() {
        std::string response_body;
        std::string status = "200 OK";

        if (method_ == "GET") {
            std::regex get_results_pattern("^/get_results/(.+)$");
            std::smatch matches;

            if (std::regex_match(uri_, matches, get_results_pattern)) {
                std::string id = matches[1].str();
                json response_json = {{"request_id", id}};
                std::vector<Url> urls = db->GetUrls(std::stoi(id));
                for (const auto& url : urls) {
                    response_json["urls"].push_back(
                        {{"url", url.url},
                         {"http_status", url.http_status},
                         {"response_time", url.response_time},
                         {"created_at", url.created_at}});
                }
                response_body = response_json.dump();
            } else {
                status = "404 Not Found";
                response_body = R"({"error": "Not Found"})";
            }
        } else if (method_ == "POST") {
            if (content_type_.find("application/json") != std::string::npos) {
                if (uri_ == "/check_urls") {
                    try {
                        json parsed = json::parse(body_);
                        int requestId = db->GetRequestId(body_);
                        std::vector<std::string> urls;
                        for (const auto& url_obj : parsed["urls"]) {
                            urls.push_back(url_obj["url"].get<std::string>());
                        }
                        url_parser->AddUrls(requestId, urls);
                        response_body = R"({"status": "OK", "request_id": )" + std::to_string(requestId) + R"(, "count_urls": )" + std::to_string(urls.size()) + "}";
                    } catch (const std::exception& e) {
                        status = "400 Bad Request";
                        response_body = R"({"error": "Bad Request"})";
                    }
                } else {
                    status = "404 Not Found";
                    response_body = R"({"error": "Not Found"})";
                }
            } else {
                status = "415 Unsupported Media Type";
                response_body = R"({"error": "Unsupported Media Type"})";
            }
        } else {
            status = "405 Method Not Allowed";
            response_body = R"({"error": "Method Not Allowed"})";
        }
        std::string content_type = "text/html; charset=UTF-8";
        std::regex get_results_pattern("^/get_results/(.+)$");
        if (method_ == "GET" && std::regex_match(uri_, get_results_pattern)) {
            content_type = "application/json; charset=UTF-8";
        }

        std::string response = "HTTP/1.1 " + status + "\r\n"
            "Content-Type: " + content_type + "\r\n"
            "Content-Length: " + std::to_string(response_body.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            response_body;

        auto self(shared_from_this());
        boost::asio::async_write(
            socket_, boost::asio::buffer(response),
            [this, self](boost::system::error_code, std::size_t) {
                socket_.close();
            });
    }

    tcp::socket socket_;
    boost::asio::streambuf buffer_;
    std::string method_, uri_, version_, body_, content_type_;
    size_t content_length_ = 0;
    std::shared_ptr<UrlParser> url_parser;
    std::shared_ptr<IDatabase> db;
};

class HttpServer {
   public:
    HttpServer(
        boost::asio::io_context& io_context, unsigned short port,
        size_t maxThreads, const std::shared_ptr<IDatabase>& database,
        const size_t timeout,
        std::function<std::unique_ptr<IHttpClient>(const std::string&, size_t)> httpClientFactory)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), max_threads(maxThreads), timeout(timeout) {

        db = database;
        url_parser = std::make_shared<UrlParser>(max_threads, timeout, db,  httpClientFactory);
        accept();
    }

   private:
    void accept() {
        acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<HttpSession>(std::move(socket), url_parser, db)->start();
            }
            accept();
        });
    }

    tcp::acceptor acceptor_;
    size_t max_threads;
    std::shared_ptr<UrlParser> url_parser;
    std::shared_ptr<IDatabase> db;
    size_t timeout;
};
