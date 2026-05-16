#pragma once

#include "mc/world/actor/player/Player.h"

namespace my_mod {

/**
 * @brief Sends an IP verification form to the player.
 * 
 * This function creates and sends a simple form to the player for IP verification.
 * If the player hasn't passed verification yet, the form will be resent.
 * 
 * @param player The player to send the form to.
 * @param token Optional JWT token for verification. If empty, a new token will be generated.
 * @param warn Optional flag to indicate if this is a warning retry.
 */
void SendForm(Player& player, std::string token = "", bool warn = false);

} // namespace my_mod
