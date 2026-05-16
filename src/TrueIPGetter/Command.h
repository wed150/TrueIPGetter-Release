#pragma once
#include "mc/server/commands/CommandOutput.h"
#include "ll/api/command/CommandHandle.h"


namespace TrueIPGetter {

/**
 * @brief Structure for the skipipcheck command overload parameters.
 */
struct SkipIpCheckOverload {
    CommandSelector<Player> player;  ///< The target player(s)
    bool action{};                                   ///< True to skip, false to unskip
};

/**
 * @brief Registers all commands provided by this mod.
 * 
 * Current registers the 'skipipcheck' command which allows administrators
 * to skip IP verification for specific players.
 */
void registerCommands();

} // namespace TrueIPGetter
