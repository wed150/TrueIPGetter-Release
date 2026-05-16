#pragma once


namespace my_mod {

/**
 * @brief Installs all memory hooks for IP retrieval.
 * 
 * This function installs hooks on Player::getIPAndPort,
 * NetworkIdentifier::getIPAndPort, and NetworkIdentifier::getAddress
 * to intercept and modify IP address retrieval.
 */
void installHooks();

/**
 * @brief Removes all installed memory hooks.
 * 
 * This function uninstalls all previously installed hooks to restore
 * original behavior.
 */
void uninstallHooks();

} // namespace my_mod
