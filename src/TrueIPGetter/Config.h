#pragma once

#include <string>

/**
 * @brief Configuration structure for the TrueIPGetter mod.
 * 
 * This structure holds all configurable parameters that can be set
 * via the config.json file.
 */
struct Config {
    int version = 1;  ///< Configuration version for compatibility checking
    
    bool RamdomSecret = true;  ///< Whether to generate a random JWT secret on each startup
    
    std::string ModifiedSecret = "";  ///< Custom JWT secret (used when RamdomSecret is false)
    
    std::string ServerIP = "";  ///< The server's public IP address for IP verification
    
    int ServerPort = 8080;  ///< The server's port number
    
    int NatPort = 8080;  ///< Port for the NAT traversal HTTP server
    
    /// URL of the IP query server used for client-side IP detection
    std::string IPQueryServer = "https://ip-zone-3popaahzhphu-1328425533.eo-edgefunctions.com";
};