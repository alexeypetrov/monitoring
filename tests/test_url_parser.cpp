#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include <filesystem>
#include "url_parser.h"
#include "test_http_client.h"
#include "sqlite_db.h"

class UrlParserTest : public ::testing::Test {
   protected:
    void SetUp() override {
        test_db_path = "test.db";
        deleteTestDb();
        db = std::make_shared<SqliteDb>(test_db_path);
        auto http_client_factory = [](const std::string&, size_t) -> std::unique_ptr<HttpClientInterface> {
            return std::make_unique<TestHttpClient>();
        };
        parser = std::make_unique<UrlParser>(1, 1, db, http_client_factory);
    }

    void TearDown() override {
        db.reset();
        deleteTestDb();
        parser = nullptr;
    }
    void deleteTestDb() {
        if (std::filesystem::exists(test_db_path)) {
            std::filesystem::remove(test_db_path);
        }
    }

    std::string test_db_path;
    std::shared_ptr<SqliteDb> db;
    std::unique_ptr<UrlParser> parser;
};

TEST_F(UrlParserTest, CheckUrl) {
    const int requestId = 1;
    parser->addUrls(requestId, {
        "http://localhost"
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::vector<Url> urls = db->find(requestId);

    ASSERT_EQ(urls.size(), 1);
    EXPECT_EQ(urls[0].url, "http://localhost");
    EXPECT_EQ(urls[0].http_status, 200);
    EXPECT_EQ(urls[0].response_time, 100);
}
