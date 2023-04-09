#include <cstdint>
#include "components/NESPeripherals.h"
#include "ImInputBinder.h"

NESPeripherals::NESPeripherals() {

    m_deviceName = "NES Peripherals";

    m_connectors["cpuBus"] = std::make_shared<Connector>(DataInterface{
            .read = [&](uint32_t address, uint32_t & buffer) {
                if(address == 0x4016) {
                    buffer = m_controller1.DATA();
                    return true;
                } else if(address == 0x4017) {
                    buffer = m_controller2.DATA();
                    return true;
                } else {
                    return false;
                }
            },
            .write = [&](uint32_t address, uint32_t data) {
                if(address == 0x4016){
                    // Controller 1 and 2 strobe bit.
                    m_controller1.OUT(data & 0x1);
                    m_controller2.OUT(data & 0x1);
                }
            }
    });
}

void NESPeripherals::init() {
    m_controller1.pressedButtons = 0;
    m_controller1.dataShifter    = 0;
    m_controller1.mic            = false;
    m_controller1.strobeLatch    = false;

    m_controller2.pressedButtons = 0;
    m_controller2.dataShifter    = 0;
    m_controller2.mic            = false;
    m_controller2.strobeLatch    = false;
}

std::vector<EmulatorWindow> NESPeripherals::getGUIs() {

    std::function<void(void)> debugger = [this](){

        // Window contents
        // ===================================================================
        int playerId = 1;
        ImGui::SeparatorText("Pressed buttons");
        for(unsigned int *shifter : {&m_controller1.pressedButtons, &m_controller2.pressedButtons}) {

            ImGui::Text("Player %d", playerId);
            ImGui::CheckboxFlags("A", shifter, 0x1 << (int)Controller::inputButtons::A);
            ImGui::CheckboxFlags("B", shifter, 0x1 << (int)Controller::inputButtons::B);
            ImGui::CheckboxFlags("Select", shifter, 0x1 << (int)Controller::inputButtons::SELECT);
            ImGui::CheckboxFlags("Start", shifter, 0x1 << (int)Controller::inputButtons::START);
            ImGui::CheckboxFlags("Up", shifter, 0x1 << (int)Controller::inputButtons::UP);
            ImGui::CheckboxFlags("Down", shifter, 0x1 << (int)Controller::inputButtons::DOWN);
            ImGui::CheckboxFlags("Left", shifter, 0x1 << (int)Controller::inputButtons::LEFT);
            ImGui::CheckboxFlags("Right", shifter, 0x1 << (int)Controller::inputButtons::RIGHT);

            playerId++;
        }

        ImGui::SeparatorText("State shifters");
        ImGui::Text("Player 1: 0x%u", m_controller1.dataShifter);
        ImGui::Text("Player 2: 0x%u", m_controller2.dataShifter);

        ImGui::SeparatorText("Microphone (P2)");
        ImGui::Checkbox("Sound detected", &m_controller2.mic);

        ImGui::SeparatorText("Strobe latches");
        ImGui::Checkbox("Player 1", &m_controller1.strobeLatch);
        ImGui::Checkbox("Player 2", &m_controller2.strobeLatch);
    };

    return {
            EmulatorWindow{
                    .category = m_deviceName,
                    .title = "Debugger",
                    .id    = getDeviceID(),
                    .dock  = DockSpace::LEFT,
                    .guiFunction = debugger
            }
    };
}

std::vector<ImInputBinder::action_t> NESPeripherals::getInputs() {

    return {
            ImInputBinder::action_t {
                .name_id = "[P1] A",
                .key     = ImGuiKey_Semicolon,
                .pressCallback = [&](){ m_controller1.setState(Controller::inputButtons::A, true); },
                .releaseCallback = [&](){ m_controller1.setState(Controller::inputButtons::A, false); },
            },
            ImInputBinder::action_t {
                .name_id = "[P1] B",
                .key     = ImGuiKey_K,
                .pressCallback = [&](){ m_controller1.setState(Controller::inputButtons::B, true); },
                .releaseCallback = [&](){ m_controller1.setState(Controller::inputButtons::B, false); },
            },
            ImInputBinder::action_t {
                    .name_id = "[P1] Start",
                    .key     = ImGuiKey_L,
                    .pressCallback = [&](){ m_controller1.setState(Controller::inputButtons::START, true); },
                    .releaseCallback = [&](){ m_controller1.setState(Controller::inputButtons::START, false); },
            },
            ImInputBinder::action_t {
                    .name_id = "[P1] Select",
                    .key     = ImGuiKey_O,
                    .pressCallback = [&](){ m_controller1.setState(Controller::inputButtons::SELECT, true); },
                    .releaseCallback = [&](){ m_controller1.setState(Controller::inputButtons::SELECT, false); },
            },
            ImInputBinder::action_t {
                    .name_id = "[P1] Left",
                    .key     = ImGuiKey_LeftArrow,
                    .pressCallback = [&](){ m_controller1.setState(Controller::inputButtons::LEFT, true); },
                    .releaseCallback = [&](){ m_controller1.setState(Controller::inputButtons::LEFT, false); },
            },
            ImInputBinder::action_t {
                    .name_id = "[P1] Right",
                    .key     = ImGuiKey_RightArrow,
                    .pressCallback = [&](){ m_controller1.setState(Controller::inputButtons::RIGHT, true); },
                    .releaseCallback = [&](){ m_controller1.setState(Controller::inputButtons::RIGHT, false); },
            },
            ImInputBinder::action_t {
                    .name_id = "[P1] Up",
                    .key     = ImGuiKey_UpArrow,
                    .pressCallback = [&](){ m_controller1.setState(Controller::inputButtons::UP, true); },
                    .releaseCallback = [&](){ m_controller1.setState(Controller::inputButtons::UP, false); },
            },
            ImInputBinder::action_t {
                    .name_id = "[P1] Down",
                    .key     = ImGuiKey_DownArrow,
                    .pressCallback = [&](){ m_controller1.setState(Controller::inputButtons::DOWN, true); },
                    .releaseCallback = [&](){ m_controller1.setState(Controller::inputButtons::DOWN, false); },
            },
            ImInputBinder::action_t {
                    .name_id = "[P2] A",
                    .key     = ImGuiKey_D,
                    .pressCallback = [&](){ m_controller2.setState(Controller::inputButtons::A, true); },
                    .releaseCallback = [&](){ m_controller2.setState(Controller::inputButtons::A, false); },
            },
            ImInputBinder::action_t {
                    .name_id = "[P2] B",
                    .key     = ImGuiKey_A,
                    .pressCallback = [&](){ m_controller2.setState(Controller::inputButtons::B, true); },
                    .releaseCallback = [&](){ m_controller2.setState(Controller::inputButtons::B, false); },
            },
            ImInputBinder::action_t {
                    .name_id = "[P2] Start",
                    .key     = ImGuiKey_S,
                    .pressCallback = [&](){ m_controller2.setState(Controller::inputButtons::START, true); },
                    .releaseCallback = [&](){ m_controller2.setState(Controller::inputButtons::START, false); },
            },
            ImInputBinder::action_t {
                    .name_id = "[P2] Select",
                    .key     = ImGuiKey_W,
                    .pressCallback = [&](){ m_controller2.setState(Controller::inputButtons::SELECT, true); },
                    .releaseCallback = [&](){ m_controller2.setState(Controller::inputButtons::SELECT, false); },
            },
            ImInputBinder::action_t {
                    .name_id = "[P2] Left",
                    .key     = ImGuiKey_F,
                    .pressCallback = [&](){ m_controller2.setState(Controller::inputButtons::LEFT, true); },
                    .releaseCallback = [&](){ m_controller2.setState(Controller::inputButtons::LEFT, false); },
            },
            ImInputBinder::action_t {
                    .name_id = "[P2] Right",
                    .key     = ImGuiKey_H,
                    .pressCallback = [&](){ m_controller2.setState(Controller::inputButtons::RIGHT, true); },
                    .releaseCallback = [&](){ m_controller2.setState(Controller::inputButtons::RIGHT, false); },
            },
            ImInputBinder::action_t {
                    .name_id = "[P2] Up",
                    .key     = ImGuiKey_T,
                    .pressCallback = [&](){ m_controller2.setState(Controller::inputButtons::UP, true); },
                    .releaseCallback = [&](){ m_controller2.setState(Controller::inputButtons::UP, false); },
            },
            ImInputBinder::action_t {
                    .name_id = "[P2] Down",
                    .key     = ImGuiKey_G,
                    .pressCallback = [&](){ m_controller2.setState(Controller::inputButtons::DOWN, true); },
                    .releaseCallback = [&](){ m_controller2.setState(Controller::inputButtons::DOWN, false); },
            },
            ImInputBinder::action_t {
                    .name_id = "[P2] Mic",
                    .key     = ImGuiKey_P,
                    .pressCallback = [&](){ m_controller2.setState(Controller::inputButtons::MIC, true); },
                    .releaseCallback = [&](){ m_controller2.setState(Controller::inputButtons::MIC, false); },
            }
    };

}

uint8_t NESPeripherals::Controller::DATA() {

    uint8_t data;

    if(strobeLatch)
        dataShifter = pressedButtons;

    data = (dataShifter & 0x1) | (mic << 2);

    if(!strobeLatch){
        dataShifter >>= 1;
    }

    return data;
}

void NESPeripherals::Controller::OUT(bool value) {

    if(value){
        strobeLatch = true;
        dataShifter = pressedButtons;
    } else {
        strobeLatch = false;
    }
}

void NESPeripherals::Controller::setState(NESPeripherals::Controller::inputButtons button, bool value) {

    if(button == inputButtons::MIC){
        mic = value;
    } else {
        putBit((uint8_t)button, value);
    }
}
