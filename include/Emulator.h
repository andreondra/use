/**
 * @file Emulator.h
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief Main platform class.
 * @copyright Copyright (c) 2023 Ondrej Golasowski
 *
 */

#ifndef USE_EMULATOR_H
#define USE_EMULATOR_H

#include <memory>
#include <map>
#include "Sound.h"
#include "System.h"
#include "ImInputBinder.h"

/**
 * Main platform class.
 *
 * This class represents the whole application. It handles GUI rendering (both global and System's), System loading,
 * audio data interchange between System and sound management class (Sound) and input actions handling
 * using ImInputBinder.
 *
 * As a developer of a new System, you usually want to add your new system here. As a Component developer,
 * you should not change anything here.
 * */
class Emulator{
private:
    // ===========================================
    // States
    // ===========================================
    enum class SYSTEMS { NONE, BARE6502, NES } m_systemID = SYSTEMS::NONE;
    enum class STATE { STOPPED, RUNNING } m_runState = STATE::STOPPED;

    // ===========================================
    // Widgets, plug-ins and other components
    // ===========================================
    std::unique_ptr<System> m_system;
    std::unique_ptr<Sound> m_sound;
    ImInputBinder m_inputs;
    bool m_showBindingsWindow = false;

    // ===========================================
    // Helpers
    // ===========================================
    /// Used to keep track of elapsed clocks to sync emulation with sound output.
    unsigned long long m_clockCounter = 0;
    /**
     * This function maps DockSpace enum to string, which is used by ImGui docking feature.
     *
     * @param dockSpace DockSpace to map.
     * @return String equivalent of the DockSpace.
     *
     * @note It is implemented this way instead macros, maps or other, because switch which use
     * enum classes are checked to have a case for every value, thus increasing error-proofness of the code.
     * */
    static std::string dockSpaceToString(DockSpace dockSpace);

    /// Set whether to idle on no user interaction.
    void setIdling(bool enabled);

    /// Generate keybindings savefile name.
    std::string getKeybindingsSaveFileName() const;

    // ===========================================
    // System handling
    // ===========================================
    /**
     * Load a System to the emulator, initialize and load the debugging GUI of the system.
     * @param system A system to load.
     * */
    void loadSystem(std::unique_ptr<System> system);

    /**
     * Emulate the System if enabled.
     * */
    void runSystem();

    // ===========================================
    // GUI callbacks
    // ===========================================
    void guiMain();
    void guiStatusBar();
    void guiToolbar();
    void guiMenuItems();

public:
    Emulator() = default;

    /**
     * @brief Run the application.
     *
     * This is the main application loop, return only on exit or error.
     * @return 0 on success, other values on error.
     * */
    int run();
};

#endif //USE_EMULATOR_H
