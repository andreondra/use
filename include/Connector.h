/**
 * @file Connector.h
 * Connector classes and specializations.
 * */

#ifndef USE_CONNECTOR_H
#define USE_CONNECTOR_H

#include <variant>
#include <memory>
#include "Types.h"

/**
 * An inter-component connection abstraction mechanism. Connector is exposed by a controlled Component
 * and passed to a controlling Component's Port.
 *
 * @note It is meant to be passed around using a smart observing pointer, such as std::weak_ptr
 * to keep track of "connections".
 * */
class Connector {

private:
    /// An interface which can be used by a controlling Component.
    std::variant<std::monostate, DataInterface, SignalInterface> m_interface;

public:
    Connector() = default;
    explicit Connector(DataInterface interface);
    explicit Connector(SignalInterface interface);

    /**
     * Check whether the Connector contains a data interface.
     * @return true If there is a data interface.
     * */
    [[nodiscard]] bool hasDataInterface() const;
    /**
     * Check whether the Connector contains a signal interface.
     * @return true If there is a signal interface.
     * */
    [[nodiscard]] bool hasSignalInterface()  const;

    /**
     * Get data interface reference.
     * @return Data interface.
     * @throw std::logic_error There is not data interface stored.
     * */
    const DataInterface & getDataInterface();

    /**
     * Get signal interface reference.
     * @return Signal interface.
     * @throw std::logic_error There is not signal interface stored.
     * */
    const SignalInterface  & getSignalInterface();

    /**
     * Store a data interface.
     * @param interface Data interface to store.
     * */
    void setInterface(DataInterface interface);

    /**
     * Store a signal interface.
     * @param interface Signal interface to store.
     * */
    void setInterface(SignalInterface interface);
};

#endif //USE_CONNECTOR_H
