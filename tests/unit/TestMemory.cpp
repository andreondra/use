/**
 * @file TestMemory.cpp Test Memory class.
 * */

#include "gtest/gtest.h"
#include "components/Memory.h"

/// Test whether the memory sets default values correctly inside bounds.
TEST(TestMemory, DefaultValues) {

    uint8_t valuesToTest[] = {0x00, 0x55, 0xFF};

    // Reading inside the boundaries.
    for(auto & val : valuesToTest) {

        Memory memory(0x10, {0x00, 0x0F}, val);

        std::weak_ptr<Connector> mc = memory.getConnector("data");
        DataInterface di = mc.lock()->getDataInterface();

        uint32_t buffer;
        for(int i = 0x00; i < 0x10; i++) {
            EXPECT_TRUE(di.read(i, buffer));
            EXPECT_EQ(buffer, val);
        }
    }
}

/// Test default values outside the boundaries.
TEST(TestMemory, DefaultValuesMirroring) {

    uint8_t valuesToTest[] = {0x00, 0x55, 0xFF};

    // Reading inside the boundaries.
    for(auto & val : valuesToTest) {

        Memory memory(0x10, {0x10, 0x30}, val);

        std::weak_ptr<Connector> mc = memory.getConnector("data");
        DataInterface di = mc.lock()->getDataInterface();

        uint32_t buffer = 0;
        for(int i = 0x0; i < 0x10; i++) {
            EXPECT_FALSE(di.read(i, buffer));
        }

        for(int i = 0x10; i < 0x30; i++) {
            EXPECT_TRUE(di.read(i, buffer));
            EXPECT_EQ(buffer, val);
        }

        for(int i = 0x31; i < 0x40; i++) {
            EXPECT_FALSE(di.read(i, buffer));
        }
    }
}

/// Test R/W access within boundaries.
TEST(TestMemory, ReadWrite) {

    const uint32_t coef = 0xAA;

    Memory memory(0x10, {0x00, 0x0F}, 0x00);

    std::weak_ptr<Connector> mc = memory.getConnector("data");
    DataInterface di = mc.lock()->getDataInterface();

    uint32_t buffer;
    for(int i = 0x00; i < 0x10; i++) {

        EXPECT_TRUE(di.read(i, buffer));
        EXPECT_EQ(buffer, 0x00);

        di.write(i, i + coef);

        EXPECT_TRUE(di.read(i, buffer));
        EXPECT_EQ(buffer, i + coef);
    }
}

/// Test R/W access outside the boundaries (mirroring).
TEST(TestMemory, ReadWriteMirroring) {

    const uint32_t coef = 0xAA;

    Memory memory(0x10, {0x10, 0x2F}, 0x00);

    std::weak_ptr<Connector> mc = memory.getConnector("data");
    DataInterface di = mc.lock()->getDataInterface();

    uint32_t buffer = 0x00;
    for(int i = 0x00; i < 0x10; i++) {
        di.write(i, i + coef);
        EXPECT_FALSE(di.read(i, buffer));
        EXPECT_EQ(buffer, 0x00);
    }

    for(int i = 0x10; i < 0x30; i++) {
        di.write(i, i + coef);
        EXPECT_TRUE(di.read(i, buffer));
        EXPECT_EQ(buffer, i + coef);
    }

    for(int i = 0x30; i < 0x40; i++) {
        di.write(i, i + coef);
        EXPECT_FALSE(di.read(i, buffer));
    }
}

/// Test init function.
TEST(TestMemory, Init) {

    const uint32_t coef = 0xBB;
    const uint32_t defaultValue = 0xFF;

    Memory memory(0x10, {0x00, 0x0F}, defaultValue);

    std::weak_ptr<Connector> mc = memory.getConnector("data");
    DataInterface di = mc.lock()->getDataInterface();

    uint32_t buffer = 0x00;
    for(int i = 0x00; i < 0x10; i++) {
        di.write(i, coef);
    }

    memory.init();

    for(int i = 0x00; i < 0x10; i++) {
        EXPECT_TRUE(di.read(i, buffer));
        EXPECT_EQ(buffer, 0xFF);
    }
}