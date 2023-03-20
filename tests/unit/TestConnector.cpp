/**
 * @file TestConnector.cpp Connector tests.
 * */

#include "gtest/gtest.h"
#include "Connector.h"
#include <memory>

/// Test default connector state (no interfaces).
TEST(TestConnector, Basic) {

    Connector c;
    EXPECT_FALSE(c.hasDataInterface());
    EXPECT_FALSE(c.hasSignalInterface());
}

/// Test data interface.
TEST(TestConnector, Data) {

    const uint32_t MAX_ADDR = 0x10;
    uint32_t writeBuffer;

    DataInterface d{
            .read = [](uint32_t address, uint32_t & buffer) -> bool{
                if(address <= MAX_ADDR){
                    buffer = address;
                    return true;
                } else {
                    return false;
                }
            },
            .write = [& writeBuffer](uint32_t address, uint32_t data) {
                if(address <= MAX_ADDR) {
                    writeBuffer = data + address;
                }
            }
    };

    Connector c(d);
    EXPECT_TRUE(c.hasDataInterface());
    EXPECT_FALSE(c.hasSignalInterface());
    EXPECT_NO_THROW(c.getDataInterface());
    const DataInterface & dRef = c.getDataInterface();

    uint32_t readBuffer;
    for(uint32_t i = 0; i <= MAX_ADDR; i++) {
        EXPECT_TRUE(dRef.read(i, readBuffer));
        EXPECT_EQ(readBuffer, i);
    }

    EXPECT_FALSE(dRef.read(MAX_ADDR + 0x10, readBuffer));

    for(uint32_t i = 0; i <= MAX_ADDR; i++) {
        dRef.write(i, 0x420);
        EXPECT_EQ(writeBuffer, i + 0x420);
    }
}

/// Test signal interface.
TEST(TestConnector, Signal) {

    bool called = false;
    bool val = false;
    SignalInterface s{
        .send = [& called](){
            called = true;
        },
        .set  = [& val](bool active){
            val = active;
        }
    };

    Connector c(s);
    EXPECT_TRUE(c.hasSignalInterface());
    EXPECT_NO_THROW(c.getSignalInterface());

    c.getSignalInterface().send();
    EXPECT_TRUE(called);

    EXPECT_FALSE(val);
    c.getSignalInterface().set(true);
    EXPECT_TRUE(val);
    c.getSignalInterface().set(false);
    EXPECT_FALSE(val);
}

/// Test re-assignments of interfaces.
TEST(TestConnector, Assignments) {

    SignalInterface s;
    DataInterface d;
    Connector c;

    // 2 passes to be sure the re-assignment works as well.
    for(int i = 0; i < 2; i++) {
        c.setInterface(s);
        EXPECT_TRUE(c.hasSignalInterface());
        EXPECT_FALSE(c.hasDataInterface());
        c.setInterface(d);
        EXPECT_TRUE(c.hasDataInterface());
        EXPECT_FALSE(c.hasSignalInterface());
    }
}