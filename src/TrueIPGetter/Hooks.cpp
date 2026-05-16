#include "Hooks.h"

#include "Config.h"
#include "Entry.h"

#include "ll/api/service/Bedrock.h"
#include "mc/network/ServerNetworkHandler.h"
#include "mc/common/SubClientId.h"
#include "mc/server/ServerPlayer.h"
#include "ll/api/memory/Hook.h"

#include "ll/api/data/KeyValueDB.h"
#include "ll/api/io/Logger.h"

namespace TrueIPGetter {

// External global variables from Entry.cpp
extern std::set<mce::UUID> passedPlayers;
extern std::unique_ptr<ll::data::KeyValueDB> ipDb;
extern Config config;
extern ll::io::Logger* logger;

// Hook: Player::getIPAndPort
LL_AUTO_TYPE_INSTANCE_HOOK(
    playerIpGetHook,
    HookPriority::Normal,
    Player,
    &Player::getIPAndPort,
    std::string
) {
    if (this) {
        // Get player UUID
        const auto uuid = this->getUuid().asString();
        
        // Check if we have a custom IP for this player
        if (ipDb && ipDb->has(uuid)) {
            auto ipAndPort = origin();
            const auto colonPos = ipAndPort.find(":");
            std::string port = ":14514";
            
            // Extract port from original IP:Port string
            if (colonPos != std::string::npos) {
                port = ipAndPort.substr(colonPos);
            }
            
            const auto newIP = ipDb->get(uuid).value();
            
            // Return modified IP if it's not the default placeholder
            if (newIP != "0.0.0.0") {
                return newIP + port;
            }
        }
    }
    
    return origin();
}

// Hook: NetworkIdentifier::getIPAndPort
LL_AUTO_TYPE_INSTANCE_HOOK(
    networkIpGetHook,
    HookPriority::Normal,
    NetworkIdentifier,
    &NetworkIdentifier::getIPAndPort,
    std::string,
) {
    if (const auto networkHandlerOpt = ll::service::getServerNetworkHandler()) {
        // Get ServerPlayer and validate
        if (const auto* serverPlayer = networkHandlerOpt.value()._getServerPlayer(*this, SubClientId::PrimaryClient)) {
            // Get player UUID
            const auto uuid = serverPlayer->getUuid().asString();
            
            // Check if we have a custom IP for this player
            if (ipDb && ipDb->has(uuid)) {
                auto ipAndPort = origin();
                const auto colonPos = ipAndPort.find(":");
                std::string port = ":14514";
                
                // Extract port from original IP:Port string
                if (colonPos != std::string::npos) {
                    port = ipAndPort.substr(colonPos);
                }
                
                const auto newIP = ipDb->get(uuid).value();
                
                // Return modified IP if it's not the default placeholder
                if (newIP != "1.1.1.1") {
                    return newIP + port;
                }
            }
        }
    }
    
    return origin();
}

// Hook: NetworkIdentifier::getAddress
LL_AUTO_TYPE_INSTANCE_HOOK(
    networkAddressGetHook,
    HookPriority::Normal,
    NetworkIdentifier,
    &NetworkIdentifier::getAddress,
    std::string,
) {
    // Get player UUID from the network identifier
    const auto uuid = ll::service::getServerNetworkHandler().value()
                    ._getServerPlayer(*this, SubClientId::PrimaryClient)
                    ->getUuid()
                    .asString();
    
    // Check if we have a custom IP for this player
    if (ipDb && ipDb->has(uuid)) {
        return ipDb->get(uuid).value();
    }
    
    return origin();
}

void installHooks() {
    // Hooks are automatically installed by the LL_AUTO_TYPE_INSTANCE_HOOK macro
    logger->debug("IP钩子已安装");
}

void uninstallHooks() {
    playerIpGetHook::unhook();
    networkIpGetHook::unhook();
    networkAddressGetHook::unhook();
    logger->debug("IP钩子已卸载");
}

} // namespace TrueIPGetter
