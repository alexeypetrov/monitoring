#pragma once

#include <cstddef>
#include <string>

struct Url {
    int request_id;
    std::string url;
    int http_status = 0;
    int response_time = 0;
    std::string created_at = "";
};
