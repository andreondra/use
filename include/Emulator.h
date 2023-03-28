//
// Created by golas on 21.2.23.
//

#ifndef USE_EMULATOR_H
#define USE_EMULATOR_H

#include <memory>
#include <map>
#include "System.h"

class Emulator{

private:
    enum class SYSTEMS{ NONE, BARE6502, NES } m_systemID = SYSTEMS::NONE;
    std::unique_ptr<System> m_system;

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

    // ===========================================
    // System handling
    // ===========================================
    /**
     * Load a System to the emulator, initialize and load the debugging GUI of the system.
     * @param system A system to load.
     * */
    bool m_runEnabled = false;
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
