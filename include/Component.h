/**
 * @file Component.h
 * Abstract Component interface.
 * */

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
     * */
    virtual void init() = 0;

    /**
     * Get a emulator-wide unique component identifier.
     * By default a casted pointer to the component instance.
     * @return Component's unique ID.
     * */
    [[nodiscard]] virtual uintptr_t getDeviceID() const;

    /**
     * Get a component's name.
     * @return Component's name.
     * */
    [[nodiscard]] virtual std::string getDeviceName() const;

    virtual void setDeviceName(const std::string & newName);

    /**
     * Connect */
    virtual void connect(const std::string & toPort, std::weak_ptr<Connector> connector);
    virtual void disconnect(const std::string & fromPort);
    virtual std::weak_ptr<Connector> getConnector(const std::string & name);

    [[nodiscard]] virtual std::vector<std::string> listConnectors() const;
    [[nodiscard]] virtual std::vector<std::string> listPorts() const;

    virtual std::vector<EmulatorWindow> getGUIs() = 0;
    virtual SoundSampleSources getSoundSampleSources();

    virtual std::vector<ImInputBinder::action_t> getInputs();

    [[nodiscard]] virtual bool initRequested();

    // maybe provide default implementation using listConnectors...?
    //virtual void renderNode();

    // virtual bool supportsDynamicDataPorts();
    // virtual void addDataPort(const std::string & name);

    // virtual void saveState() = 0;
    // virtual void loadState() = 0;
};

#endif //USE_COMPONENT_H
