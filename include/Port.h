//
// Created by golas on 1.3.23.
//

#ifndef USE_PORT_H
#define USE_PORT_H

#include <memory>
#include "Connector.h"

class Port {

protected:
    std::weak_ptr<Connector> m_connector;

public:
    Port() = default;
    virtual ~Port() = default;

    virtual void connect(std::weak_ptr<Connector> connector) = 0;
    virtual void disconnect() final;

    [[nodiscard]] virtual bool empty() const final;
    virtual explicit operator bool() const final;
};

class DataPort : public Port {

public:
    DataPort() = default;
    ~DataPort() override = default;

    void connect(std::weak_ptr<Connector> connector) override;
    uint32_t read(uint32_t address);
    void write(uint32_t address, uint32_t data);
};

class SignalPort : public Port {

public:
    SignalPort() = default;
    ~SignalPort() override = default;

    void connect(std::weak_ptr<Connector> connector) override;
    void send();
};

#endif //USE_PORT_H
