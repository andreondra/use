//
// Created by golas on 21.2.23.
//

#include "immapp/immapp.h"
#include "imgui.h"
#include "Emulator.h"

void Emulator::guiStatusBar() {
    ImGui::Text("Ready");
}

void Emulator::guiToolbar() {

}

void Emulator::guiMenuItems() {

    ImGui::MenuItem("Systems");
}

void Emulator::mainLoop() {

}

void Emulator::setup() {


}

int Emulator::run() {

    // ===============================================
    // Basic parameters
    // ===============================================
    HelloImGui::RunnerParams par;
    par.appWindowParams.windowTitle = "USE: Universal System Emulator";
    par.appWindowParams.windowGeometry.size = {640, 480};
    par.imGuiWindowParams.showMenuBar = false;
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
    splitLeft.initialDock = "MainDockSpace";
    splitLeft.newDock = "LeftSpace";
    splitLeft.direction = ImGuiDir_Left;
    splitLeft.ratio = 0.25f;
    par.dockingParams.dockingSplits.push_back(splitLeft);

    HelloImGui::DockingSplit splitBottom;
    splitBottom.initialDock = "MainDockSpace";
    splitBottom.newDock = "BottomSpace";
    splitBottom.direction = ImGuiDir_Down;
    splitBottom.ratio = 0.25f;
    par.dockingParams.dockingSplits.push_back(splitBottom);

    HelloImGui::DockingSplit splitRight;
    splitRight.initialDock = "MainDockSpace";
    splitRight.newDock = "RightSpace";
    splitRight.direction = ImGuiDir_Right;
    splitRight.ratio = 0.25f;
    par.dockingParams.dockingSplits.push_back(splitRight);

    // ===============================================
    // Dockable windows setup
    // ===============================================

//    for(auto & pkg : mPackages) {
//
//        HelloImGui::DockableWindow window;
//        window.label = pkg.getName();
//        window.dockSpaceName = dockNames[static_cast<int>(pkg.getDefaultDock())];
//        window.GuiFunction = [&](){pkg.render();};
//
//        par.dockingParams.dockableWindows.push_back(window);
//    }

    ImmApp::Run(par);

    return 0;
}
