//
// Created by golas on 1.3.23.
//

#ifndef USE_CONNECTOR_H
#define USE_CONNECTOR_H

#include <variant>
#include <memory>
#include "Types.h"

/**
 * An inter-component connection abstraction mechanism.
 *
 * @note It is meant to be passed around using a smart observing pointer, such as std::weak_ptr
 * to keep track of "connections".
 * */
class Connector {

private:
    std::variant<DataInterface, SignalInterface> m_interface;

public:
    Connector(Connector & src) = delete;
    Connector & operator=(const Connector & src) = delete;

    Connector() = default;
    explicit Connector(DataInterface interface);
    explicit Connector(SignalInterface interface);

    [[nodiscard]] bool hasDataInterface() const;
    [[nodiscard]] bool hasSignalInterface()  const;

    const DataInterface & getDataInterface();
    const SignalInterface  & getSignalInterface();
    void setInterface(DataInterface interface);
    void setInterface(SignalInterface interface);
};

/*
 * mainBus.read()
 * mainBus.write()
 * mainBus.set()
 *
 * DataPort mainBus;
 * SignalPort REQ;
 * SignalPort HALT;
 * std::map<std::string name, Port & port> m_ports; nebo shared ptr?
 * m_ports({}, {}, {})
 *
 * listPorts, connect(...) - port.connect(...)
 *
 * Connector NMI;
 * Connector IRQ;
 * std::map<std::string name, Connector & connector> m_connectors;
 * std::map<std::string name, weak_ptr<Connector> connector> m_connectors;
 *  getConnector(name)
 *
 * mainBus.read()
 * mainBus.write()
 * REQ.set()
 *
 * mainBus.signal()
 *
 * */

#endif //USE_CONNECTOR_H
