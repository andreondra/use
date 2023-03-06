/**
 * @file TestMemory.cpp Test Memory class.
 * */

#include <fstream>
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

        uint32_t buffer = 0x0;
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

        uint32_t buffer = 0x0;
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

    uint32_t buffer = 0x0;
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

TEST(TestMemory, Load) {

    Memory memory(5, {0x00, 0x0F}, 0x00);

    std::weak_ptr<Connector> mc = memory.getConnector("data");
    DataInterface di = mc.lock()->getDataInterface();

    std::ifstream fileSmall("testfiles/dummySmall.bin", std::ios_base::binary);
    std::ifstream fileBig("testfiles/dummyBig.bin", std::ios_base::binary);
    if(!fileSmall) throw std::runtime_error("Can't open file dummySmall.bin.");
    if(!fileBig) throw std::runtime_error("Can't open file dummyBig.bin.");

    // Try to specify invalid offsets.
    for(uint32_t i = 0x6; i < 0xA; i++)
        EXPECT_THROW(memory.load(i, fileSmall), std::invalid_argument);

    memory.load(0x0, fileSmall);

    uint32_t buffer = 0x0;
    // Compare memory contents with the known contents of the dummySmall (sequence 0x1 - 0x5).
    uint32_t expected = 0x1;
    for(uint32_t i = 0x0; i < 0x5; i++) {
        di.read(i, buffer);
        EXPECT_EQ(buffer, expected++);
    }

    // Load bigger file, check.
    memory.load(0x0, fileBig);
    expected = 0xA;
    for(uint32_t i = 0x0; i < 0x5; i++) {
        di.read(i, buffer);
        EXPECT_EQ(buffer, expected--);
    }
    // Test mirroring.
    expected = 0xA;
    for(uint32_t i = 0x5; i < 0xA; i++) {
        di.read(i, buffer);
        EXPECT_EQ(buffer, expected--);
    }
}

TEST(TestMemory, LoadOffset) {

    Memory memory(5, {0x00, 0x0F}, 0x00);

    std::weak_ptr<Connector> mc = memory.getConnector("data");
    DataInterface di = mc.lock()->getDataInterface();

    std::ifstream fileSmall("testfiles/dummySmall.bin", std::ios_base::binary);

    for(uint32_t offset = 0x0; offset < 0x5; offset++) {

        memory.init();
        memory.load(offset, fileSmall);
        uint32_t expected = 0x0;
        uint32_t buffer = 0x0;
        for(uint32_t adr = 0x0; adr < 0x5; adr++) {
            di.read(adr, buffer);
            expected = (adr >= offset) ? expected + 1 : 0x0;
            EXPECT_EQ(buffer, expected);
        }

        fileSmall.seekg(std::ios_base::beg);
    }
}