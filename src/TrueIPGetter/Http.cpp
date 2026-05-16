#include "Http.h"

#include "Config.h"
#include "Entry.h"
#include "Jwt.h"

#include "ll/api/service/Bedrock.h"
#include "mc/network/packet/ClientboundCloseFormPacket.h"
#include "mc/platform/UUID.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"

#include <nlohmann/json.hpp>
#include "ll/api/data/KeyValueDB.h"
#include "ll/api/io/Logger.h"

namespace TrueIPGetter {

// Static member initialization
httplib::Server HttpServerManager::m_server;
std::atomic<bool> HttpServerManager::m_running(false);
std::optional<ll::thread::ThreadPoolExecutor> HttpServerManager::m_threadPool;

// External global variables from Entry.cpp
extern std::set<mce::UUID> passedPlayers;
extern std::unique_ptr<ll::data::KeyValueDB> ipDb;
extern Config config;
extern ll::io::Logger* logger;

bool HttpServerManager::init(int port) {
    try {
        // Register the IP report endpoint
        m_server.Post("/report_ip", [](const httplib::Request& req, httplib::Response& res) {
            try {
                // Parse JSON body
                nlohmann::json j = nlohmann::json::parse(req.body);
                const std::string token = j.at("token");
                
                logger->debug("IP上报: " + req.body);
                
                // Verify the JWT token
                if (JwtUtils::verifyToken(token)) {
                    const auto decoded = jwt::decode(token);
                    const auto strUuid = decoded.get_subject();

                    // Only process if player hasn't already passed verification
                    if (const mce::UUID uuid = mce::UUID::fromString(strUuid); !passedPlayers.contains(uuid)) {
                        // Store the reported IP in the database
                        ipDb->set(decoded.get_subject(), j.at("ip"));
                        passedPlayers.insert(uuid);
                        
                        // Close the form on the client side

                        if (const auto player = ll::service::getLevel()->getPlayer(uuid)) {
                            ClientboundCloseFormPacket closeFormPacket;
                            player->sendNetworkPacket(closeFormPacket);
                            logger->info(player->getRealName() + " - IP: " + j.at("ip").get<std::string>());
                        }
                    }
                } else {
                    logger->error("收到无效的JWT令牌");
                }
                
                // Send success response
                nlohmann::json response;
                response["status"] = "ok";
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_content(response.dump(), "application/json");
                
            } catch (const std::exception& e) {
                logger->error("JSON解析错误: " + std::string(e.what()));
                res.status = 400;
                res.set_content("无效的JSON", "text/plain");
            }
        });
        
        // Configure thread pool for the server
        m_server.new_task_queue = []() {
            return new httplib::ThreadPool(2); // Use 2 threads
        };
        
        // Start the server in a separate thread
        m_running = true;
        m_threadPool.emplace("HttpServerPool", 2);
        m_threadPool->execute([port]() {
            logger->info("正在启动HTTP服务器，端口: " + std::to_string(port));
            if (!m_server.listen("0.0.0.0", port)) {
                logger->error("无法在端口 " + std::to_string(port) + " 上启动HTTP服务器");
                m_running = false;
            }
        });
        
        logger->info("HTTP服务器已启动，端口: " + std::to_string(port));
        return true;
        
    } catch (const std::exception& e) {
        logger->error("无法启动HTTP服务器: " + std::string(e.what()));
        return false;
    }
}

void HttpServerManager::stop() {
    if (m_running) {
        m_server.stop();
        
        // Destroy thread pool
        if (m_threadPool.has_value()) {
            try {
                m_threadPool->destroy();
                m_threadPool.reset();
                logger->info("HTTP线程池已销毁");
            } catch (const std::exception& e) {
                logger->error("销毁线程池时出错: " + std::string(e.what()));
            }
        }
        
        m_running = false;
        logger->info("HTTP服务器已停止");
    }
}

bool HttpServerManager::isRunning() {
    return m_running;
}

httplib::Server& HttpServerManager::getServer() {
    return m_server;
}

} // namespace TrueIPGetter
