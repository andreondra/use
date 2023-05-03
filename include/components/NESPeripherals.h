/**
 * @file NESPeripherals.h
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief NES controllers emulation.
 * @copyright Copyright (c) 2023 Ondrej Golasowski
 */
#ifndef USE_NESPERIPHERALS_H
#define USE_NESPERIPHERALS_H

#include "Component.h"

/**
 * NES Peripherals emulation. For now, there is only emulation of two standard controllers implemented.
 *
 * Connectors: data cpuBus to interface with the main NES bus.
 * */
class NESPeripherals : public Component {
private:
    /// Standard NES controller.
    class Controller{
    public:
        /// Status of physical buttons (which are pressed -- in emulator this corresponds to configured keys on keyboard or gamepad). Using int to be compatible with ImGui::CheckboxFlags.

        union {
            unsigned int debug = 0;
            uint8_t data;
        } pressedButtons;

        /// Status is copied to this shifter when the strobe latch is on. Using int to be compatible with ImGui::CheckboxFlags.
        uint8_t dataShifter = 0;
        int shiftedCount = 0;
        /// Microphone status.
        bool mic = false;
        /// Latch which enables the status update of dataShifter.
        bool strobeLatch = false;
        /// Helper function to put bit to corresponding position.
        void putBit(uint8_t pos, bool value){
            pressedButtons.data &= ~(0x1 << pos);
            pressedButtons.data |= value << pos;
        }

        /// Bit positions in shifter.
        enum class inputButtons{
            A       = 0,
            B,
            SELECT,
            START,
            UP,
            DOWN,
            LEFT,
            RIGHT,
            MIC
        };

        Controller() = default;
        ~Controller() = default;

        /// Get serial data from the controller.
        uint8_t DATA();
        /// Set strobe latch.
        void OUT(bool value);
        /// Set the state of physical buttons.
        void setState(inputButtons button, bool value);
    } m_controller1, m_controller2;

public:
    NESPeripherals();
    ~NESPeripherals() override = default;

    void init() override;
    std::vector<EmulatorWindow> getGUIs() override;
    std::vector<ImInputBinder::action_t> getInputs() override;
};

#endif //USE_NESPERIPHERALS_H
