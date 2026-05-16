#include "Command.h"
#include "Entry.h"

#include "ll/api/data/KeyValueDB.h"
#include "mc/world/actor/player/Player.h"
namespace TrueIPGetter {

extern std::unique_ptr<ll::data::KeyValueDB> ipDb;

void registerCommands() {
    auto& command = ll::command::CommandRegistrar::getInstance(false)
                        .getOrCreateCommand("skipipcheck", "跳过IP验证", CommandPermissionLevel::GameDirectors);
    
    command.overload<SkipIpCheckOverload>()
        .required("player")
        .required("action")
        .execute([](CommandOrigin const& origin, CommandOutput& output, SkipIpCheckOverload const& overload) {
            if (auto entity = overload.player.results(origin).data) {
                try {
                    for (auto en : *entity) {
                        if (en) {
                            auto uuid = en->getUuid().asString();
                            
                            if (overload.action) {
                                // Set player to skip IP verification
                                ipDb->set(uuid, "1.1.1.1");
                                output.success("成功设置玩家跳过IP验证");
                            } else {
                                // Remove skip flag if it exists
                                if (ipDb->get(uuid) == "1.1.1.1") {
                                    ipDb->del(uuid);
                                }
                                output.success("成功取消玩家跳过IP验证");
                            }
                        }
                    }
                } catch (...) {
                    // Silently handle any exceptions
                }
            }
        });
}

} // namespace TrueIPGetter
