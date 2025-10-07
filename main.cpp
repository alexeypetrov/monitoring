#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <regex>
#include <string>
#include "curl.h"
#include "http_server.h"
#include "sqlite_db.h"

namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

int main(const int argc, char* argv[]) {
    boost::program_options::options_description desc("Options");
    desc.add_options()
    ("help,h", "show help")
    ("max-threads,m", boost::program_options::value<std::size_t>()->default_value(1), "max threads")
    ("database-path,d", boost::program_options::value<std::string>()->default_value("monitoring.db"), "database path")
    ("timeout,t", boost::program_options::value<std::size_t>()->default_value(10), "timeout in seconds")
    ("port,p", boost::program_options::value<unsigned short>()->default_value(8080), "HTTP server port");

    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
    boost::program_options::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }

    const auto maxThreads = vm["max-threads"].as<std::size_t>();
    const auto databasePath = vm["database-path"].as<std::string>();
    const auto timeout = vm["timeout"].as<std::size_t>();
    const auto port = vm["port"].as<unsigned short>();

    try {
        boost::asio::io_context ioContext;
        auto database = std::make_shared<SqliteDb>(databasePath);
        auto httpClientFactory = [](const std::string& url, size_t timeout) -> std::unique_ptr<HttpClientInterface> {
            return std::make_unique<Curl>(url, timeout);
        };
        HttpServer server(ioContext, port, maxThreads, database, timeout, httpClientFactory);

        ioContext.run();
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
