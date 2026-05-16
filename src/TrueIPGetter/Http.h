#pragma once

#include "ll/api/thread/ThreadPoolExecutor.h"
#include "httplib.h"

namespace TrueIPGetter {

/**
 * @brief Manages the HTTP server for IP reporting.
 * 
 * This class handles starting, stopping, and managing the HTTP server
 * that receives IP reports from clients.
 */
class HttpServerManager {
public:
    /**
     * @brief Initializes and starts the HTTP server.
     * 
     * @param port The port to listen on (default: 8080).
     * @return true if the server started successfully, false otherwise.
     */
    static bool init(int port = 8080);

    /**
     * @brief Stops the HTTP server and cleans up resources.
     */
    static void stop();

    /**
     * @brief Checks if the server is currently running.
     * 
     * @return true if the server is running, false otherwise.
     */
    static bool isRunning();

    /**
     * @brief Gets the HTTP server instance.
     * 
     * @return httplib::Server& Reference to the server instance.
     */
    static httplib::Server& getServer();

private:
    static httplib::Server m_server;
    static std::atomic<bool> m_running;
    static std::optional<ll::thread::ThreadPoolExecutor> m_threadPool;
};

} // namespace TrueIPGetter
