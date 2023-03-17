/**
 * @file Types.h
 * Emulator helper types.
 * */

#ifndef USE_TYPES_H
#define USE_TYPES_H

#include <cstdint>
#include <functional>
#include <string>

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

struct SignalInterface {
    std::function<void(void)> send;
};

struct AddressRange {

    uint32_t from;
    uint32_t to;

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
#endif //USE_TYPES_H
