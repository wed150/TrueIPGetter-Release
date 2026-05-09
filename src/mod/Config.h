#pragma once
struct Config {
    int  version          = 1;
    bool RamdomSecret = true;
    std::string ModifiedSecret = "";
    std::string ServerIP = "";
    int ServerPort = 8080;
    int NatPort = 8080;
    std::string IPQueryServer = "https://ip-zone-3popaahzhphu-1328425533.eo-edgefunctions.com";

};