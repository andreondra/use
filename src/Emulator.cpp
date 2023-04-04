//
// Created by golas on 21.2.23.
//

#include <memory>
#include <thread>
#include "immapp/immapp.h"
#include "imgui.h"
#include "Emulator.h"
#include "systems/Bare6502.h"
#include "systems/NES.h"
#include "Types.h"

std::string Emulator::dockSpaceToString(DockSpace dockSpace) {

    switch(dockSpace) {
        case DockSpace::MAIN:   return "MainDockSpace";
        case DockSpace::LEFT:   return "LeftSpace";
        case DockSpace::RIGHT:  return "RightSpace";
        case DockSpace::BOTTOM: return "BottomSpace";
        default:                return "";
    }
}

void Emulator::setIdling(bool enabled) {
    HelloImGui::GetRunnerParams()->fpsIdling.fpsIdle = enabled ? 9 : 0;
}

void Emulator::loadSystem(std::unique_ptr<System> system) {

    m_runState = STATE::STOPPED;

    // Get app state.
    HelloImGui::RunnerParams *params;
    params = HelloImGui::GetRunnerParams();

    // Remove existing debugging windows.
    params->dockingParams.dockableWindows.clear();

    // Change and init system.
    m_system.swap(system);
    m_system->init();

    // Configure sound.
    m_sound = std::make_unique<Sound>(m_system->soundOutputCount());

    // Add debugging windows from the new System.
    for(auto & windowConfig : m_system->getGUIs()) {

        // Add Window category to label.
        std::string newLabel = (!windowConfig.category.empty()) ? "[" + windowConfig.category + "] " : "";
        // Add Window title.
        newLabel += windowConfig.title;
        // Add unique ID.
        newLabel += "###" + std::to_string(windowConfig.id);

        HelloImGui::DockableWindow window;
        window.label = newLabel;
        window.dockSpaceName = dockSpaceToString(windowConfig.dock);
        window.GuiFunction = windowConfig.guiFunction;

        // Add new windows.
        params->dockingParams.dockableWindows.push_back(window);
        // Reset the layout, so the windows are properly docked.
        params->dockingParams.layoutReset = true;
    }
}

void Emulator::runSystem() {

    if(m_runState == STATE::RUNNING && m_system) {

        unsigned long remainingClocks = m_system->getClockRate() / DEFAULT_IMGUI_REFRESH_HZ;

        while(remainingClocks) {

            if(m_clockCounter % m_sound->getSampleRate() == 0) {
                // Flush all frames to all outputs.
                m_sound->writeFrames(m_system->getSampleSources());
            }

            m_system->doClocks(1);
            remainingClocks--;

            m_clockCounter++;
        }

    }
}

void Emulator::guiStatusBar() {

    if(m_system) {
        switch(m_runState) {
            case STATE::RUNNING:
                ImGui::Text("Running...");
                break;
            case STATE::STOPPED:
                ImGui::Text("Stopped.");
                break;
        }
    } else {
        ImGui::Text("Ready to load a system");
    }
}

void Emulator::guiToolbar() {

    if(ImGui::BeginMenu("Run")) {

        if(m_system) {

            switch(m_runState) {
                case STATE::STOPPED:
                    if (ImGui::MenuItem("Clock")) m_system->doClocks(1);
                    if (ImGui::MenuItem("Step")) m_system->doSteps(1);
                    if (ImGui::MenuItem("Frame")) m_system->doFrames(1);
                    ImGui::Separator();
                    if (ImGui::MenuItem("Run...")) {
                        setIdling(false);
                        m_sound->start();
                        m_runState = STATE::RUNNING;
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Hard reset")) m_system->init();
                    break;
                case STATE::RUNNING:
                    if (ImGui::MenuItem("Stop")) {
                        setIdling(true);
                        m_clockCounter = 0;
                        m_sound->stop();
                        m_runState = STATE::STOPPED;
                    }
                    break;
            }

        } else {
            ImGui::Text("Please select a system");
        }

        ImGui::EndMenu();
    }
}

void Emulator::guiMenuItems() {

    if(ImGui::BeginMenu("Select System")) {

        if(ImGui::MenuItem("None", nullptr, m_systemID == SYSTEMS::NONE)) {

            // Get app state.
            HelloImGui::RunnerParams *params;
            params = HelloImGui::GetRunnerParams();

            // Remove existing debugging windows.
            params->dockingParams.dockableWindows.clear();

            m_system.reset();
            m_systemID = SYSTEMS::NONE;
        } else if(ImGui::MenuItem("Bare 6502", nullptr, m_systemID == SYSTEMS::BARE6502)) {

            loadSystem(std::make_unique<Bare6502>());
            m_systemID = SYSTEMS::BARE6502;
        } else if(ImGui::MenuItem("NES", nullptr, m_systemID == SYSTEMS::NES)) {

            loadSystem(std::make_unique<NES>());
            m_systemID = SYSTEMS::NES;
        }

        ImGui::EndMenu();
    }
}

int Emulator::run() {

    // ===============================================
    // Basic parameters
    // ===============================================
    HelloImGui::RunnerParams par;
    par.appWindowParams.windowTitle = "USE: Universal System Emulator";
    par.appWindowParams.windowGeometry.size = {1280, 720};
    par.appWindowParams.restorePreviousGeometry = false;

    // ===============================================
    // Status bar
    // ===============================================
    par.imGuiWindowParams.showStatusBar = true;
    par.imGuiWindowParams.showStatus_Fps = true;
    par.callbacks.ShowStatus = [&]{guiStatusBar();};

    // ===============================================
    // Tool bar
    // ===============================================
    par.imGuiWindowParams.showMenuBar = true;
    par.callbacks.ShowAppMenuItems = [&](){guiMenuItems();};
    par.callbacks.ShowMenus = [&](){guiToolbar();};

    // ===============================================
    // Docking areas
    // ===============================================
    par.imGuiWindowParams.defaultImGuiWindowType = HelloImGui::DefaultImGuiWindowType::ProvideFullScreenDockSpace;
    par.imGuiWindowParams.enableViewports = false;
    par.dockingParams.layoutCondition = HelloImGui::DockingLayoutCondition::ApplicationStart;

    HelloImGui::DockingSplit splitLeft;
    splitLeft.initialDock = dockSpaceToString(DockSpace::MAIN);
    splitLeft.newDock = dockSpaceToString(DockSpace::LEFT);
    splitLeft.direction = ImGuiDir_Left;
    splitLeft.ratio = 0.25f;
    par.dockingParams.dockingSplits.push_back(splitLeft);

    HelloImGui::DockingSplit splitBottom;
    splitBottom.initialDock = dockSpaceToString(DockSpace::MAIN);
    splitBottom.newDock = dockSpaceToString(DockSpace::BOTTOM);
    splitBottom.direction = ImGuiDir_Down;
    splitBottom.ratio = 0.25f;
    par.dockingParams.dockingSplits.push_back(splitBottom);

    HelloImGui::DockingSplit splitRight;
    splitRight.initialDock = dockSpaceToString(DockSpace::MAIN);
    splitRight.newDock = dockSpaceToString(DockSpace::RIGHT);
    splitRight.direction = ImGuiDir_Right;
    splitRight.ratio = 0.25f;
    par.dockingParams.dockingSplits.push_back(splitRight);

    // Run emulation if enabled.
    par.callbacks.PreNewFrame = [this](){
        runSystem();
        if(m_system) m_system->onRefresh();
    };

    // Note: debugger dockable windows are set up during System change.
    ImmApp::Run(par);

    return 0;
}
