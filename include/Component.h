//
// Created by golas on 21.2.23.
//

#ifndef USE_COMPONENT_H
#define USE_COMPONENT_H

#include <cstdint>
#include <string>
#include <map>
#include <set>
#include "Types.h"

class Component{

protected:
    static std::string m_deviceName;
    std::map<std::string, DataInterface> m_dataConnectors;
    std::map<std::string, DataInterface> m_dataPorts;
    std::map<std::string, std::function<void(void)>> m_pinConnectors;
    std::map<std::string, std::function<void(void)>> m_pinPorts;

public:

    Component() = default;
    virtual ~Component() = 0;
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

    // ========================================================================
    // Data bus handling.
    // ========================================================================
    [[nodiscard]] virtual std::vector<std::string> listDataPorts() const;
    virtual void connectData(const std::string & portName, DataInterface connector);
    virtual void disconnectData(const std::string & portName);
    [[nodiscard]] virtual std::vector<std::string> listDataConnectors() const;
    virtual DataInterface getDataConnector(const std::string & connectorName);

    // ========================================================================
    // Pin I/O handling.
    // ========================================================================
    [[nodiscard]] virtual std::vector<std::string> listPinPorts() const;
    virtual void connectPin(const std::string & portName, std::function<void(void)> connector);
    virtual void disconnectPin(const std::string & portName);
    [[nodiscard]] virtual std::vector<std::string> listPinConnectors() const;
    virtual std::function<void(void)> getPinConnector(const std::string & pinName);

    virtual void renderGUI() = 0;

    // virtual bool supportsDynamicDataPorts();
    // virtual void addDataPort(const std::string & name);

    // virtual void saveState() = 0;
    // virtual void loadState() = 0;
};

#endif //USE_COMPONENT_H
