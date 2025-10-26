#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <chrono>
#include <memory>
#include <filesystem>
#include <thread>
#include "http_server.h"
#include "test_http_client.h"
#include "sqlite_db.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using boost::asio::ip::tcp;

class HttpServerTest : public ::testing::Test {
   protected:
    void SetUp() override {
        test_db_path = "test_http_server.db";
        deleteTestDb();
        db = std::make_shared<SqliteDb>(test_db_path);
        auto http_client_factory = [](const std::string&, size_t) -> std::unique_ptr<HttpClientInterface> {
            return std::make_unique<TestHttpClient>();
        };
        port = 8888;
        io_context = std::make_unique<boost::asio::io_context>();
        server = std::make_unique<HttpServer>(*io_context, port, 2, db, 1000, http_client_factory);
        server_thread = std::thread([this]() {
            io_context->run();
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        io_context->stop();
        if (server_thread.joinable()) {
            server_thread.join();
        }
        server.reset();
        io_context.reset();
        db.reset();
        deleteTestDb();
    }
    void deleteTestDb() {
        if (std::filesystem::exists(test_db_path)) {
            std::filesystem::remove(test_db_path);
        }
    }
    std::string sendHttpRequest(const std::string& method,
                                const std::string& url,
                                const std::string& body = "",
                                const std::string& content_type = "application/json") {
        boost::asio::io_context io;
        tcp::socket socket(io);
        tcp::resolver resolver(io);
        auto endpoints = resolver.resolve("127.0.0.1", std::to_string(port));
        boost::asio::connect(socket, endpoints);
        std::string request = method + " " + url + " HTTP/1.1\r\n";
        request += "Host: localhost\r\n";
        request += "Content-Type: " + content_type + "\r\n";
        request += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        request += "Connection: close\r\n";
        request += "\r\n";
        request += body;
        boost::asio::write(socket, boost::asio::buffer(request));
        boost::asio::streambuf response_buf;
        boost::system::error_code ec;
        boost::asio::read(socket, response_buf, boost::asio::transfer_all(), ec);
        std::istream response_stream(&response_buf);
        std::string response((std::istreambuf_iterator<char>(response_stream)), std::istreambuf_iterator<char>());

        return response;
    }
    std::string getBody(const std::string& response) {
        size_t body_pos = response.find("\r\n\r\n");
        if (body_pos != std::string::npos) {
            return response.substr(body_pos + 4);
        }
        return "";
    }
    std::string getStatusCode(const std::string& response) {
        size_t start = response.find(' ') + 1;
        size_t end = response.find(' ', start);
        return response.substr(start, end - start);
    }

    std::string test_db_path;
    std::shared_ptr<SqliteDb> db;
    std::unique_ptr<boost::asio::io_context> io_context;
    std::unique_ptr<HttpServer> server;
    std::thread server_thread;
    unsigned short port;
};

TEST_F(HttpServerTest, CheckUrl) {
    json requestBody = {
        {"urls", {
            {{"url", "http://localhost"}}
        }}
    };
    std::string response = sendHttpRequest("POST", "/check_urls", requestBody.dump());
    std::string body = getBody(response);
    std::string status = getStatusCode(response);

    EXPECT_EQ(status, "200");
    json responseJson = json::parse(body);
    EXPECT_EQ(responseJson["status"], "OK");
    EXPECT_TRUE(responseJson.contains("request_id"));
    EXPECT_EQ(responseJson["count_urls"], 1);
    int requestId = responseJson["request_id"];

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string resultResponse = sendHttpRequest("GET", "/get_results/" + std::to_string(requestId));
    std::string resultBody = getBody(resultResponse);
    std::string resultstatus = getStatusCode(resultResponse);

    EXPECT_EQ(status, "200");
    json responseBody = json::parse(resultBody);
    EXPECT_EQ(responseBody["request_id"], std::to_string(requestId));
    EXPECT_TRUE(responseBody.contains("urls"));
    EXPECT_EQ(responseBody["urls"].size(), 1);
    EXPECT_TRUE(responseBody["urls"][0].contains("url"));
    EXPECT_TRUE(responseBody["urls"][0].contains("http_status"));
    EXPECT_TRUE(responseBody["urls"][0].contains("response_time"));
    EXPECT_TRUE(responseBody["urls"][0].contains("created_at"));
    EXPECT_EQ(responseBody["urls"][0]["url"], "http://localhost");
    EXPECT_EQ(responseBody["urls"][0]["http_status"], 200);
    EXPECT_EQ(responseBody["urls"][0]["response_time"], 100);
}

TEST_F(HttpServerTest, CheckMultipleUrls) {
    json requestBody = {
        {"urls", {
            {{"url", "http://localhost/1"}},
            {{"url", "http://localhost/2"}},
            {{"url", "http://localhost/3"}}
        }}
    };
    std::string response = sendHttpRequest("POST", "/check_urls", requestBody.dump());
    std::string body = getBody(response);
    std::string status = getStatusCode(response);

    EXPECT_EQ(status, "200");
    json responseJson = json::parse(body);
    EXPECT_EQ(responseJson["status"], "OK");
    EXPECT_TRUE(responseJson.contains("request_id"));
    EXPECT_EQ(responseJson["count_urls"], 3);
    int requestId = responseJson["request_id"];

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string resultResponse = sendHttpRequest("GET", "/get_results/" + std::to_string(requestId));
    std::string resultBody = getBody(resultResponse);
    std::string resultStatus = getStatusCode(resultResponse);

    EXPECT_EQ(status, "200");
    json resultJson = json::parse(resultBody);
    EXPECT_EQ(resultJson["request_id"], std::to_string(requestId));
    EXPECT_TRUE(resultJson.contains("urls"));
    EXPECT_EQ(resultJson["urls"].size(), 3);

    for (const auto& url : resultJson["urls"]) {
        EXPECT_TRUE(url.contains("url"));
        EXPECT_TRUE(url.contains("http_status"));
        EXPECT_TRUE(url.contains("response_time"));
        EXPECT_TRUE(url.contains("created_at"));
        EXPECT_EQ(url["http_status"], 200);
        EXPECT_EQ(url["response_time"], 100);
    }
}

TEST_F(HttpServerTest, CheckInvalidJson) {
    std::string invalidJson = "{test";

    std::string response = sendHttpRequest("POST", "/check_urls", invalidJson);
    std::string body = getBody(response);
    std::string status = getStatusCode(response);

    EXPECT_EQ(status, "400");
    json responseJson = json::parse(body);
    EXPECT_EQ(responseJson["error"], "Bad Request");
}

TEST_F(HttpServerTest, CheckWrongContentType) {
    json request_body = {
        {"urls", {
            {{"url", "http://localhost"}}
        }}
    };

    std::string response = sendHttpRequest("POST", "/check_urls", request_body.dump(), "text/plain");
    std::string body = getBody(response);
    std::string status = getStatusCode(response);

    EXPECT_EQ(status, "415");
    json responseJson = json::parse(body);
    EXPECT_EQ(responseJson["error"], "Unsupported Media Type");
}

TEST_F(HttpServerTest, CheckRequestIdNotFound) {
    std::string response = sendHttpRequest("GET", "/get_results/9999");
    std::string body = getBody(response);
    std::string status = getStatusCode(response);

    EXPECT_EQ(status, "404");
    json responseJson = json::parse(body);
    EXPECT_EQ(responseJson["error"], "Request ID not found");
}

TEST_F(HttpServerTest, CheckWrongUrl) {
    std::string response = sendHttpRequest("GET", "/test");
    std::string body = getBody(response);
    std::string status = getStatusCode(response);

    EXPECT_EQ(status, "404");
    json responseJson = json::parse(body);
    EXPECT_EQ(responseJson["error"], "Not Found");
}

TEST_F(HttpServerTest, CheckUnsupportedMethod) {
    std::string response = sendHttpRequest("PUT", "/check_urls", "{}");
    std::string body = getBody(response);
    std::string status = getStatusCode(response);

    EXPECT_EQ(status, "405");
    json responseJson = json::parse(body);
    EXPECT_EQ(responseJson["error"], "Method Not Allowed");
}
