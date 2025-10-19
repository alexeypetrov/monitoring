#include "sqlite_db.h"

SqliteDb::SqliteDb(const std::string& databasePath)
    : db(nullptr, &sqlite3_close), db_path(databasePath) {
    initDb();
    createTable();
}

[[nodiscard]] size_t SqliteDb::getRequestId(const std::string& content) const {
    const char* insert_sql = R"(
        INSERT INTO requests (content)
        VALUES (?);
    )";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db.get(), insert_sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return 0;
    }
    sqlite3_bind_text(stmt, 1, content.c_str(), -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(db.get());
}

bool SqliteDb::insert(const Url& url) {
    const char* insert_sql = R"(
        INSERT INTO urls (request_id, url, http_status, response_time)
        VALUES (?, ?, ?, ?);
    )";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db.get(), insert_sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, url.request_id);
    sqlite3_bind_text(stmt, 2, url.url.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, url.http_status);
    sqlite3_bind_int(stmt, 4, url.response_time);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

std::vector<Url> SqliteDb::find(const int requestId) {
    std::vector<Url> urls;
    const char* select_sql = R"(
        SELECT request_id, url, http_status, response_time, created_at
        FROM urls
        WHERE request_id = ?;
    )";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db.get(), select_sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return urls;
    }

    sqlite3_bind_int(stmt, 1, requestId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Url url;
        url.request_id = sqlite3_column_int(stmt, 0);
        url.url = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        url.http_status = sqlite3_column_int(stmt, 2);
        url.response_time = sqlite3_column_int(stmt, 3);
        url.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        urls.push_back(url);
    }

    sqlite3_finalize(stmt);
    return urls;
}

bool SqliteDb::requestIdExists(const int requestId) {
    const char* select_sql = R"(
        SELECT COUNT(*) FROM requests WHERE id = ?;
    )";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db.get(), select_sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, requestId);

    bool isExists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        isExists = sqlite3_column_int(stmt, 0) > 0;
    }

    sqlite3_finalize(stmt);
    return isExists;
}

void SqliteDb::initDb() {
    sqlite3* temp_db = nullptr;
    int rc = sqlite3_open(db_path.c_str(), &temp_db);
    if (rc != SQLITE_OK) {
        std::string errorMsg = "Can't open database: " + std::string(sqlite3_errmsg(temp_db));
        sqlite3_close(temp_db);
        throw std::runtime_error(errorMsg);
    }
    db.reset(temp_db);
}

void SqliteDb::createTable() {
    const char* create_table_sql = R"(
        CREATE TABLE IF NOT EXISTS requests (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            content TEXT NOT NULL,
            created_at DATETIME DEFAULT (datetime('now','localtime'))
        );
        CREATE TABLE IF NOT EXISTS urls (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            request_id INTEGER NOT NULL,
            url TEXT NOT NULL,
            http_status INTEGER NOT NULL,
            response_time INTEGER NOT NULL,
            created_at DATETIME DEFAULT (datetime('now','localtime')),
            FOREIGN KEY (request_id) REFERENCES requests (id)
        );)";
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db.get(), create_table_sql, nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK) {
        std::string error_msg = "SQL error: " + std::string(err_msg);
        sqlite3_free(err_msg);
        throw std::runtime_error(error_msg);
    }
}
