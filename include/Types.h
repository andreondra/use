/**
 * @file Types.h
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief Emulator helper types.
 * @copyright Copyright (c) 2023 Ondrej Golasowski
 *
 */

#ifndef USE_TYPES_H
#define USE_TYPES_H

#include <cstdint>
#include <functional>
#include <string>

/**
 * Abstract R/W data interface, which can read and send data at specified address.
 * Used in Components to interface with ports.
 * */
struct DataInterface {

    /**
     * Read data from the device.
     * @param address Address to read from.
     * @return Data read from the address.
     * */
    std::function<bool(uint32_t address, uint32_t & buffer)> read;

    /**
     * Write data to the device.
     * @param address Address to write to.
     * @param data Data to store.
     * */
    std::function<void(uint32_t address, uint32_t data)> write;
};

/**
 * Abstract signal interface, which can trigger a remote action (usually used to trigger a signal
 * in a different component).
 * */
struct SignalInterface {

    /**
     * Send a signal pulse to connected device.
     * */
    std::function<void(void)> send;

    /**
     * Set a signal state. This function does not care if active is low or high.
     * When the active is true, the signal should be considered in its active state.
     */
    std::function<void(bool active)> set;
};

/**
 * Universal address range type.
 * */
struct AddressRange {

    /// Start address.
    uint32_t from;
    /// End address.
    uint32_t to;

    /**
     * Check if the specified value is in the address range.
     * @param value Value to be checked.
     * @return true If the value is in the range, otherwise false.
     * */
    [[nodiscard]] bool has(const uint32_t & value) const {
        return (from <= value) && (value <= to);
    }
};

/**
 * List of available dock spaces.
 * */
enum class DockSpace{
    MAIN,
    LEFT,
    BOTTOM,
    RIGHT
};

/**
 * Helper type to construct dockable window.
 * */
struct EmulatorWindow{

    std::string category;
    /**
     * Window title
     * @warning This should be emulator-wide unique, otherwise the GUI elements will be merged to an existing window
     * of a same name. */
    std::string title = "Default Window";

    /**
     * Unique window ID.
     * */
    uintptr_t id = 0;

    /**
     * Dock space to use. Non-docked windows are not allowed to maintain clear UI.
     * */
    DockSpace dock    = DockSpace::MAIN;

    /**
     * GUI rendering function.
     * */
    std::function<void(void)> guiFunction = [](){};
};

/**
 * A single pixel.
 * */
struct RGBPixel{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

/**
 * A stereo audio frame (two samples - left and right).
 * */
struct SoundStereoFrame {
    float left;
    float right;
};

/**
 * A vector of sound getters provided by the Component.
 * */
typedef std::vector<std::function<SoundStereoFrame(void)>> SoundSampleSources;

#endif //USE_TYPES_H
