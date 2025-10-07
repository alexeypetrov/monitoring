#pragma once

#include <sqlite3.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "database_interface.h"
#include "url.h"

class SqliteDb : public DatabaseInterface {
   public:
    explicit SqliteDb(const std::string& databasePath);
    ~SqliteDb() override = default;

    [[nodiscard]] size_t getRequestId(const std::string& content) const override;

    bool insert(const Url& url) override;

    std::vector<Url> find(const int requestId) override;

   private:
    void initDb();
    void createTable();

   private:
    std::unique_ptr<sqlite3, decltype(&sqlite3_close)> db;
    std::string db_path;
};
