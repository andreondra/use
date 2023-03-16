/**
 * @file Port.h
 * Port abstract interface and specializations.
 * */

#ifndef USE_PORT_H
#define USE_PORT_H

#include <memory>
#include "Connector.h"

/**
 * @brief Component's port abstraction class.
 * The Port means to be used as a local Component's variable, which allows to interface with another Component.
 * The controlled Component exposes a Connector, which shall be stored in the Port.
 * */
class Port {

protected:
    /// A connector which Port interface with.
    std::weak_ptr<Connector> m_connector;

public:
    Port() = default;
    virtual ~Port() = default;

    /**
     * Attach a Connector to the port.
     * @param connector Connector to be connected.
     * */
    virtual void connect(std::weak_ptr<Connector> connector) = 0;

    /**
     * Detach a Connector.
     * */
    virtual void disconnect() final;

    /**
     * Checks if there is any Connector attached to the Port.
     * @return true If the Connector is present.
     * */
    [[nodiscard]] virtual bool empty() const final;

    /**
     * A shorthand for empty() function.
     * */
    virtual explicit operator bool() const final;
};

/**
 * @brief A specialized Port to store data Connectors.
 * */
class DataPort : public Port {

public:
    DataPort() = default;
    ~DataPort() override = default;

    /**
     * Connect a data Connector.
     * @param connector A connector to attach.
     * @throw std::invalid_argument If the provided connector is empty or invalid.
     * */
    void connect(std::weak_ptr<Connector> connector) override;

    /**
     * Read data at a specified address. Default value returned is 0x0.
     * @param address Address to read from.
     * @return Read value.
     * */
    uint32_t read(uint32_t address);

    /**
     * Read data at a specified address with confirmation that anybody actually responded.
     * Used for e.g. buses, when there does not have to be a device responding to the specified address.
     *
     * @param address An address to read from.
     * @param buffer A buffer to store the value to.
     * @return true If any Component responded to the address.
     * */
    bool readConfirmed(uint32_t address, uint32_t & buffer);

    /**
     * Write data to the address specified.
     * @param address An address to write the data to.
     * @param data Data to write.
     * */
    void write(uint32_t address, uint32_t data);
};

/**
 * @brief A specialized Port to store signal Connectors.
 * */
class SignalPort : public Port {

public:
    SignalPort() = default;
    ~SignalPort() override = default;

    /**
     * Connect a signal Connector.
     * @param connector A connector to attach.
     * @throw std::invalid_argument If the provided connector is empty or invalid.
     * */
    void connect(std::weak_ptr<Connector> connector) override;

    /// Send a signal to the controlled component.
    void send();
};

#endif //USE_PORT_H
