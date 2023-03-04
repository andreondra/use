/**
 * @file TestBus.cpp Test Bus class.
 * */

#include <array>
#include "gtest/gtest.h"
#include "components/Bus.h"

/// Common test fixture to avoid copying.
class TestBus : public ::testing::Test {
protected:
    static constexpr int deviceCount = 5;
    static constexpr uint32_t addrWidth = 16;
    static constexpr uint32_t dataWidth = 8;
    static constexpr int memorySize = 255;
    static constexpr uint8_t defaultValue = 0xFF;

    typedef std::array<uint8_t, memorySize> MockMemory;
    Bus bus{deviceCount, addrWidth, dataWidth};
    std::weak_ptr<Connector> busConnector;
    DataInterface masterIf;

    // Represent each device on the bus as a simple memory device.
    std::array<std::shared_ptr<Connector>, deviceCount> devices;
    std::array<MockMemory, deviceCount> deviceMemories{};

    void SetUp() override {
        busConnector = bus.getConnector("master");
        masterIf = busConnector.lock()->getDataInterface();

        for(auto & mem : deviceMemories){
            mem.fill(defaultValue);
        }

        // Bind a dummy interface to each device.
        for(int i = 0; i < deviceCount; i++) {

            MockMemory & currentMemory = deviceMemories[i];
            AddressRange currentRange{
                    (uint32_t)i * memorySize, (uint32_t)i * memorySize + memorySize - 1
            };

            devices[i] = std::make_shared<Connector>(DataInterface{

                    .read = [i, currentRange, this](uint32_t address, uint32_t & buffer) -> bool{
                        if(currentRange.has(address)){
                            buffer = deviceMemories[i][address - currentRange.from];
                            return true;
                        } else {
                            return false;
                        }
                    },
                    .write = [i, currentRange, this](uint32_t address, uint32_t data) {
                        if(currentRange.has(address)) {
                            deviceMemories[i][address - currentRange.from] = data;
                        }
                    }
            });
        }
    }
};

TEST_F(TestBus, Basic) {

    uint32_t buffer;

    // Try to read with no devices connected.
    for(int i = 0; i < deviceCount; i++) {
        for(int j = i * memorySize; j < i * memorySize + memorySize; j++) {
            EXPECT_FALSE(masterIf.read(j, buffer));
        }
    }

    // Connect devices to the bus.
    for(int i = 0; i < deviceCount; i++) {
        bus.connect("slot " + std::to_string(i), devices[i]);
    }

    // Check default values.
    for(int i = 0; i < deviceCount; i++) {
        for(int j = i * memorySize; j < i * memorySize + memorySize; j++) {
            EXPECT_TRUE(masterIf.read(j, buffer));
            EXPECT_EQ(buffer, defaultValue);
        }
    }

    // Write some data to the master interface.
    for(int i = 0; i < deviceCount; i++) {
        for(int j = i * memorySize; j < i * memorySize + memorySize; j++) {
            masterIf.write(j, i);
        }
    }

    // Check the written data.
    for(int i = 0; i < deviceCount; i++) {
        for(int j = i * memorySize; j < i * memorySize + memorySize; j++) {
            EXPECT_TRUE(masterIf.read(j, buffer));
            EXPECT_EQ(buffer, i);
        }
    }
}

/// Check the masking feature.
TEST_F(TestBus, Masking) {

    uint32_t buffer;
    const uint32_t addrMask = 0xFFFF0000;
    const uint32_t dataMask = 0xFFFFFF00;

    // Connect devices to the bus.
    for(int i = 0; i < deviceCount; i++) {
        bus.connect("slot " + std::to_string(i), devices[i]);
    }

    // Check default values, address masking test.
    for(int i = 0; i < deviceCount; i++) {
        for(int j = i * memorySize; j < i * memorySize + memorySize; j++) {
            EXPECT_TRUE(masterIf.read(addrMask | j, buffer));
            EXPECT_EQ(buffer, defaultValue);
        }
    }

    // Try to write and read values larger than the dataWidth, data masking test.
    for(int i = 0; i < deviceCount; i++) {
        for(int j = i * memorySize; j < i * memorySize + memorySize; j++) {
            masterIf.write(j, dataMask | i);
            EXPECT_TRUE(masterIf.read(j, buffer));
            EXPECT_EQ(buffer, i);
        }
    }
}