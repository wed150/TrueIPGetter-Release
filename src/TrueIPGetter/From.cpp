#include "From.h"
#include "Config.h"
#include "Entry.h"
#include "Jwt.h"

#include "ll/api/form/SimpleForm.h"

#include "ll/api/data/KeyValueDB.h"
#include "ll/api/io/Logger.h"

namespace my_mod {

// External global variables from Entry.cpp
extern std::set<mce::UUID> passedPlayers;
extern std::unique_ptr<ll::data::KeyValueDB> ipDb;
extern Config config;
extern ll::io::Logger* logger;

void SendForm(Player& player, std::string token, const bool warn) {
    // Skip if player is already marked to skip IP check
    if (ipDb->get(player.getUuid().asString()) == "1.1.1.1") {
        return;
    }

    // Generate a new JWT token if not provided
    if (token.empty()) {
        token = JwtUtils::generateToken(player.getUuid().asString());
        logger->debug("生成令牌: " + token);
    }

    // Build the form content message
    std::string content = "我们正在验证您的真实IP地址\n"
                         "请不要着急或主动关闭此表单，这只会重新请求并浪费您的时间\n"
                         "收集完成后页面将自动关闭\n"
                         "如果您多次失败，请联系管理员。管理员可以使用 /skipipcheck <玩家> true 来配置跳过检测";

    if (warn) {
        content += "\n§4您的验证尚未通过，请勿关闭";
    }

    // Create and configure the simple form
    ll::form::SimpleForm form("IP验证", content);
    
    // Construct the URL for IP query server
    const std::string url = config.IPQueryServer + "/?token=" + token + "&url="
                   + config.ServerIP + ":" + std::to_string(config.ServerPort);
    
    form.appendButton("请等待", url, "url")
        .appendButton("卡住了？点此重新收集")
        .sendTo(player, [token](Player& player, const int selected, const ll::form::FormCancelReason reason) {
            // Handle user busy case by resending the form
            if (reason == ModalFormCancelReason::UserBusy) {
                SendForm(player, token);
                return;
            }
            
            switch (selected) {
                case 0: {
                    // Check if player has passed verification
                    if (const bool isPassed = passedPlayers.contains(player.getUuid()); !isPassed) {
                        // Resend form if not passed
                        SendForm(player, token);
                    } else {
                        player.sendMessage("IP验证已通过");
                    }
                    break;
                }
                case 1: {
                    // Recollect button clicked, resend form
                    SendForm(player, token);
                    break;
                }
                case -1: {
                    // Form cancelled, check if player has passed verification
                    if (const bool isPassed = passedPlayers.contains(player.getUuid()); !isPassed) {
                        // Resend form if not passed
                        SendForm(player, token);
                    } else {
                        player.sendMessage("IP验证已通过");
                    }
                    break;
                }
            }
        });
}

} // namespace my_mod
