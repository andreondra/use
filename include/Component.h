//
// Created by golas on 21.2.23.
//

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

class Component{
protected:
    std::string m_deviceName;
    std::map<std::string, std::shared_ptr<Connector>> m_connectors;
    std::map<std::string, Port *> m_ports;

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

    virtual void connect(const std::string & toPort, std::weak_ptr<Connector> connector);
    virtual void disconnect(const std::string & fromPort);
    virtual std::weak_ptr<Connector> getConnector(const std::string & name);

    [[nodiscard]] virtual std::vector<std::string> listConnectors() const;
    [[nodiscard]] virtual std::vector<std::string> listPorts() const;

    virtual std::vector<EmulatorWindow> getGUIs() = 0;

    // maybe provide default implementation using listConnectors...?
    //virtual void renderNode();

    // virtual bool supportsDynamicDataPorts();
    // virtual void addDataPort(const std::string & name);

    // virtual void saveState() = 0;
    // virtual void loadState() = 0;
};

#endif //USE_COMPONENT_H
