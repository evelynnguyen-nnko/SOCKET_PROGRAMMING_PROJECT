#include <iostream>
#include <memory>       // std::make_shared
#include <stdexcept>    // std::exeption
#include <boost/asio.hpp>

#include "Module_1/CommandServer.h"
#include "Module_2/CommandExecutor.h"

int main() {
    unsigned short PORT = 8080;

    try {
        boost::asio::io_context ioc; // io context for Boost.Asio
        auto executor = std::make_shared<CommandExecutor>();

        CommandServer server(ioc, PORT, executor);
        std::cout << "[main] Server is starting on port " << PORT << ".\n";

        server.start(); // Start accepting connections (async)
        ioc.run();      // I/O Asio loop
    } catch (const std::exception& e) {
        std::cerr << "[main] Exeption: " << e.what() << ".\n";
        // return 1;
    }
    return 0;
}