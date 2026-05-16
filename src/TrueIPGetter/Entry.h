#pragma once

#include "ll/api/mod/NativeMod.h"

namespace my_mod {

/**
 * @brief Main entry point for the TrueIPGetter mod.
 * 
 * This class manages the mod lifecycle including loading, enabling, and disabling.
 * It coordinates all submodules to provide IP verification functionality.
 */
class Entry {
public:
    /**
     * @brief Gets the singleton instance of Entry.
     * 
     * @return Entry& Reference to the singleton instance.
     */
    static Entry& getInstance();

    Entry() : mSelf(*ll::mod::NativeMod::current()) {}

    /**
     * @brief Gets the native mod instance.
     * 
     * @return ll::mod::NativeMod& Reference to the native mod.
     */
    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    /**
     * @brief Loads the mod and initializes resources.
     * 
     * This method is called when the mod is loaded. It initializes the database
     * and loads configuration from disk.
     * 
     * @return true if the mod loaded successfully.
     * @return false if the mod failed to load.
     */
    bool load();

    /**
     * @brief Enables the mod and starts all services.
     * 
     * This method is called when the mod is enabled. It registers commands,
     * sets up event listeners, installs hooks, and starts the HTTP server.
     * 
     * @return true if the mod was enabled successfully.
     * @return false if the mod failed to enable.
     */
    bool enable();

    /**
     * @brief Disables the mod and stops all services.
     * 
     * This method is called when the mod is disabled. It removes event listeners,
     * uninstalls hooks, and stops the HTTP server.
     * 
     * @return true if the mod was disabled successfully.
     * @return false if the mod failed to disable.
     */
    bool disable();

    // TODO: Implement this method if you need to unload the mod.
    // /// @return True if the mod is unloaded successfully.
    bool unload();

private:
    ll::mod::NativeMod& mSelf;
};

} // namespace my_mod
