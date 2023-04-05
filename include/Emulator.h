//
// Created by golas on 21.2.23.
//

#ifndef USE_EMULATOR_H
#define USE_EMULATOR_H

#include <memory>
#include <map>
#include "Sound.h"
#include "System.h"

class Emulator{

private:
    unsigned long long m_clockCounter = 0;
    static const int DEFAULT_IMGUI_REFRESH_HZ = 60;

    enum class SYSTEMS{ NONE, BARE6502, NES } m_systemID = SYSTEMS::NONE;
    std::unique_ptr<System> m_system;
    std::unique_ptr<Sound> m_sound;

    enum class STATE{ STOPPED, RUNNING } m_runState = STATE::STOPPED;

    // ===========================================
    // Helpers
    // ===========================================
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

    void setIdling(bool enabled);

    // ===========================================
    // System handling
    // ===========================================
    /**
     * Load a System to the emulator, initialize and load the debugging GUI of the system.
     * @param system A system to load.
     * */
    void loadSystem(std::unique_ptr<System> system);
    void runSystem();

    // ===========================================
    // GUI callbacks
    // ===========================================
    void guiStatusBar();
    void guiToolbar();
    void guiMenuItems();

public:
    Emulator() = default;
    int run();
};

#endif //USE_EMULATOR_H
