#pragma once

#include <memory>         // std::unique_ptr, std::shared_ptr, std::enable_shared_from_this
#include <string>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>
// ---------------------------------------------------


// (File ICommandExecutor.h includes ListProcess, StartProcess, v.v.)
#include "../Module_2/ICommandExecutor.h" 

// --- namespace shortcuts ---
namespace net = boost::asio;
namespace beast = boost::beast;
using tcp = net::ip::tcp;
namespace websocket = beast::websocket;
// ------------------------------------


// Forward declaration: CommandServer
class CommandServer;

/**
 * @brief Represents a WebSocket session for a single connected client.
 *
 * This class manages all read/write operations on the WebSocket stream
 * and contains the COMMAND PARSING LOGIC for incoming text commands.
 */
class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    /**
     * @brief Constructor
     * 
     * @param socket  The TCP socket transferred from CommandServer after accepting a client.
     * @param executor A shared pointer to Module 2 (ICommandExecutor), providing the actual system-level functionalities.
     */
    ClientSession(tcp::socket&& socket, std::shared_ptr<ICommandExecutor> executor);

    // Start a client session: WebSocket handshake and begins waiting for messages.
    void start();

private:
    // WebSocket handshake
    void on_accept(beast::error_code ec);

    // Start a read loop to receive commands from the client.
    void do_read();

    /**
     * @brief Callback executed when a read operation completes.
     * 
     * @param ec Error code indicating success or failure.
     * @param bytes_transferred Number of bytes read into the buffer.
     */ 
    void on_read(beast::error_code ec, std::size_t bytes_transferred);

    // Callback executed when an asynchronous write completes.
    void on_write(beast::error_code ec, std::size_t bytes_transferred);

    /**
     * @brief Parse a command string received from the client and dispatch it
     *        to the appropriate function in Module 2.
     *
     * This contains the main `if-else` logic for command interpretation.
     *
     * @param raw_command The full text command received (e.g., "start notepad.exe").
     */
    void ParseAndExecute(const std::string& raw_command);

    // Send a text response back to client
    void send_text_response(const std::string& message);

    // Send a binary response back to client (BYTES)
    void send_binary_response(const std::vector<uint8_t>& data);

private:
    websocket::stream<tcp::socket> m_ws;          // WebSocket stream for this client
    std::shared_ptr<ICommandExecutor> m_executor; // A "bridge" to Module 2 implementation
    beast::flat_buffer m_buffer;                  // Buffer for incoming/outgoing data
};


/**
 * @brief Main server class (Module 1 - Listener).
 *
 * This class is responsible **only** for accepting new client connections
 * and spawning a new ClientSession for each connection.
 */
class CommandServer {
public:
     /**
     * @brief Constructor
     * 
     * @param ioc      The Asio I/O context (usually created in main.cpp).
     * @param port     TCP port to listen on.
     * @param executor A unique_ptr to Module 2 (ICommandExecutor),
     *                 created by main.cpp and injected into the server.
     */
    CommandServer(net::io_context& ioc, unsigned short port, std::shared_ptr<ICommandExecutor> executor);

    // Start the server and begin accepting connections.
    void start();

private:
    // Begin an asynchronous accept loop for new clients.
    void do_accept();

    // Handle new connection
    void on_accept(beast::error_code ec, tcp::socket socket);

    net::io_context& m_ioc;                       // Reference to the shared I/O context
    tcp::acceptor m_acceptor;                     // TCP acceptor listening for connections
    std::shared_ptr<ICommandExecutor> m_executor_shared;  // Shared Module 2 instance
};