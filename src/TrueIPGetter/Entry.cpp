#include "Entry.h"
#include "Command.h"
#include "Config.h"
#include "From.h"
#include "Hooks.h"
#include "Http.h"
#include "Jwt.h"

#include "ll/api/Config.h"
#include "ll/api/data/KeyValueDB.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerJoinEvent.h"
#include "ll/api/mod/RegisterHelper.h"


namespace my_mod {

    // Global variables for mod state
    std::set<mce::UUID> passedPlayers;  ///< Players who have passed IP verification
    std::unique_ptr<ll::data::KeyValueDB> ipDb;  ///< Database for UUID to IP mapping
    Config config;  ///< Mod configuration
    ll::io::Logger* logger = nullptr;  ///< Logger instance

Entry& Entry::getInstance() {
    static Entry instance;
    return instance;
}

bool Entry::load() {
    logger = &getSelf().getLogger();
    logger->debug("正在加载...");

    // Initialize IP database
    const auto& ipDbPath = getSelf().getDataDir() / "ipMapping";
    ipDb = std::make_unique<ll::data::KeyValueDB>(ipDbPath);

    // Load configuration
    const auto& configFilePath = getSelf().getConfigDir() / "config.json";
    if (!ll::config::loadConfig(config, configFilePath)) {
        logger->warn("无法从 {} 加载配置", configFilePath);
        logger->info("保存默认配置");
        if (!ll::config::saveConfig(config, configFilePath)) {
            logger->error("无法将默认配置保存到 {}", configFilePath);
        }
    }

    return true;
}

bool Entry::enable() {
    logger->debug("正在启用...");

    // Configure JWT secret
    if (config.RamdomSecret) {
        std::string secret = mce::UUID::random().asString();
        JwtUtils::setSecret(secret);
        logger->info("生成随机JWT密钥: {}", secret);
    } else {
        JwtUtils::setSecret(config.ModifiedSecret);
    }

    // Register commands
    registerCommands();

    // Install IP retrieval hooks
    installHooks();

    // Register player join event listener
    auto& eventBus = ll::event::EventBus::getInstance();
    eventBus.emplaceListener<ll::event::PlayerJoinEvent>(
        [this](const ll::event::PlayerJoinEvent& event) {
            try {
                // Remove player from passed set on rejoin
                passedPlayers.erase(event.self().getUuid());
                
                // Send IP verification form
                SendForm(event.self());
            } catch (const std::exception& e) {
                getSelf().getLogger().error("处理玩家加入事件时出错: {}", e.what());
            }
        }
    );

    // Start HTTP server
    HttpServerManager::init(config.NatPort);

    return true;
}

bool Entry::disable() {
    logger->debug("正在禁用...");

    // Uninstall hooks
    uninstallHooks();

    // Stop HTTP server
    HttpServerManager::stop();

    return true;
}

bool Entry::unload() {

    return true;
}

} // namespace my_mod

LL_REGISTER_MOD(my_mod::Entry, my_mod::Entry::getInstance());
