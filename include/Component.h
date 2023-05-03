/**
* @file Component.h
* @author Ondrej Golasowski (golasowski.o@gmail.com)
* @brief Abstract Component interface.
* @copyright Copyright (c) 2023 Ondrej Golasowski
*
*/

#ifndef USE_COMPONENT_H
#define USE_COMPONENT_H

#include <cstdint>
#include <string>
#include <map>
#include <set>
#include <memory>
#include "Types.h"
#include "Connector.h"
#include "Port.h"
#include "ImInputBinder.h"

/**
 * The Component is an abstraction mechanism for interfacing with generic emulated Component.
 * The class is used to provide an unified interface for node editors and similar tools.
 *
 * The Components interface with each other using Ports and Connectors; no other means shall be used, to
 * make the Component as universal as possible.
 *
 * See documentation for creating a custom component.
 * */
class Component{
protected:
    /// Components' name.
    std::string m_deviceName = "Default Component";

    /// Exposed connectors by name.
    std::map<std::string, std::shared_ptr<Connector>> m_connectors;

    /// Available ports by name.
    std::map<std::string, Port *> m_ports;

    /// Request init of the whole system. Used mainly by ROM on load to properly load reset vectors.
    bool m_initRequested = false;

public:

    Component() = default;
    virtual ~Component() = default;

    /**
     * Initialize a component to a default power-on state (hard reset).
     *
     * @note For Component developer: Implement a correct power-on state here.
     * */
    virtual void init() = 0;

    /**
     * Get a emulator-wide unique component identifier.
     * By default a casted pointer to the component instance.
     * @return Component's unique ID.
     * */
    [[nodiscard]] virtual uintptr_t getDeviceID() const final;

    /**
     * Get a component's name.
     * @return Component's name.
     * */
    [[nodiscard]] virtual std::string getDeviceName() const final;

    /**
     * Set component's name.
     * @param newName A new name to set.
     * */
    virtual void setDeviceName(const std::string & newName) final;

    /**
     * Connect a connector to a specified port.
     *
     * @param toPort Port to connect the connector to.
     * @param connector The connector to connect.
     * */
    virtual void connect(const std::string & toPort, std::weak_ptr<Connector> connector) final;

    /**
     * Disconnect a connector from a specified port.
     *
     * @param fromPort A name of a port from which the connector should be disconnected.
     * */
    virtual void disconnect(const std::string & fromPort) final;

    /**
     * Get a connector IO.
     *
     * @param name Name of the connector to receive.
     * @return Selected connector.
     * */
    virtual std::weak_ptr<Connector> getConnector(const std::string & name) final;

    /**
     * Return names of all connectors in the component.
     * */
    [[nodiscard]] virtual std::vector<std::string> listConnectors() const final;

    /**
     * Return names of all port in the component.
     * */
    [[nodiscard]] virtual std::vector<std::string> listPorts() const final;

    /**
     * Get GUI windows: metadata and rendering functions.
     *
     * @note For Component developer: If the component have a debugger or other GUI, return an arbitrary count of windows,
     * they will be rendered on a load of a system which contains the component.
     *
     * @return A vector of windows to be rendered.
     * */
    virtual std::vector<EmulatorWindow> getGUIs() = 0;

    /**
     * Get audio sources: a list of functions to request a stereo audio sample (frame).
     *
     * @note For Component developer: If the component has any sound outputs, return a functions to get samples from them here.
     * You can return any number of outputs, they will be mixed seamlessly.
     * */
    virtual SoundSampleSources getSoundSampleSources();

    /**
     * Get input key/gamepad mapping requests with corresponding actions.
     *
     * @note For Component developer: If the component wants to interact with keyboard, gamepad or similar, return a vector
     * of required keys and actions when they are pressed or released. Default keybindings can be
     * changed by the user and saved to their config.
     * */
    virtual std::vector<ImInputBinder::action_t> getInputs();

    /**
     * Returns true if the component wants a whole system restarted.
     *
     * @note For Component developer: It can be used for example if the component represents a program ROM
     * and a new ROM was loaded; therefore CPU needs to be restarted to change
     * the program counter to correct location in the new ROM.
     *
     * @return true Restart is requested.
     * @return false No restart needed.
     * */
    [[nodiscard]] virtual bool initRequested();

    // Prepared for future dynamic node editor.
    // Note: Maybe provide default implementation using listConnectors...?
    // virtual void renderNode();
};

#endif //USE_COMPONENT_H
