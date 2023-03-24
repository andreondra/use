/**
 * @file TestMapper000.cpp Test Mapper 000 class.
 * */

#include <random>
#include "gtest/gtest.h"
#include "components/Gamepak/Mapper000.h"

TEST(TestMapper000, Construction) {

    std::vector<uint8_t> PRGROM;
    std::vector<uint8_t> CHRROM;

    // Empty ROMs.
    EXPECT_THROW(Mapper000(PRGROM, CHRROM, Mapper::mirroringType_t::HORIZONTAL), std::invalid_argument);

    // Empty PRG ROM.
    PRGROM.resize(0x4000);
    EXPECT_NO_THROW(Mapper000(PRGROM, CHRROM, Mapper::mirroringType_t::HORIZONTAL));

    // Invalid sized CHR ROM.
    PRGROM.resize(0x4000);
    CHRROM.resize(0x1000);
    EXPECT_THROW(Mapper000(PRGROM, CHRROM, Mapper::mirroringType_t::HORIZONTAL), std::invalid_argument);

    // Invalid sized PRG ROM.
    PRGROM.clear();
    CHRROM.resize(0x2000);

    // Correctly sized ROMs.
    CHRROM.resize(0x2000);
    PRGROM.resize(0x4000);
    EXPECT_NO_THROW(Mapper000(PRGROM, CHRROM, Mapper::mirroringType_t::HORIZONTAL));

    CHRROM.resize(0x0);
    PRGROM.resize(0x8000);
    EXPECT_NO_THROW(Mapper000(PRGROM, CHRROM, Mapper::mirroringType_t::HORIZONTAL));
}

TEST(TestMapper000, CPUIO) {

    const uint8_t fillValue = 0xBB;
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> randByte(0, 255);

    std::vector<uint8_t> PRGROM(0x4000, 0x00);
    std::vector<uint8_t> PRGExpected;
    std::vector<uint8_t> CHRROM;

    // Fill PRG ROM with random values.
    for(auto & byte : PRGROM) {
        uint8_t ran = randByte(rng);
        byte = ran;
        PRGExpected.push_back(ran);
    }

    Mapper000 m(PRGROM, CHRROM, Mapper::mirroringType_t::HORIZONTAL);
    uint8_t buffer;

    // Non mapped areas test.
    // ================================================
    for(uint32_t i = 0x0; i < 0x6000; i++) {
        ASSERT_FALSE(m.cpuRead(i, buffer)) << "Error at address: " << std::hex << i;
        ASSERT_FALSE(m.cpuWrite(i, buffer)) << "Error at address: " << std::hex << i;
    }

    // PRG ROM test.
    // ================================================
    // Test for expected randomly generated values.
    for(uint32_t i = 0x8000; i < 0xFFFF; i++) {
        ASSERT_TRUE(m.cpuRead(i, buffer));
        ASSERT_EQ(buffer, PRGExpected[(i - 0x8000) & 0x3FFF]) << "Error at address: " << std::hex << i;
    }

    // Write fixed values on even addresses.
    for(uint32_t i = 0x8000; i < 0xFFFF; i += 2) {
        ASSERT_TRUE(m.cpuWrite(i, fillValue));
    }

    // Test whether only correct values were overwritten.
    for(uint32_t i = 0x8000; i < 0xFFFF; i++) {
        ASSERT_TRUE(m.cpuRead(i, buffer));

        if(i % 2)
            ASSERT_EQ(buffer, PRGExpected[(i - 0x8000) & 0x3FFF]) << "Error at address: " << std::hex << i;
        else
            ASSERT_EQ(buffer, fillValue) << "Error at address: " << std::hex << i;
    }

    // PRG RAM test.
    // ================================================
    for(uint32_t i = 0x6000; i < 0x8000; i++) {
        ASSERT_TRUE(m.cpuRead(i, buffer));
        ASSERT_EQ(buffer, 0x00);
    }

    for(uint32_t i = 0x6000; i < 0x8000; i++) {
        EXPECT_TRUE(m.cpuWrite(i, fillValue));
    }

    for(uint32_t i = 0x6000; i < 0x8000; i++) {
        ASSERT_TRUE(m.cpuRead(i, buffer));
        ASSERT_EQ(buffer, fillValue);
    }
}

TEST(TestMapper000, PPUIO) {

    const uint8_t fillValue = 0xBB;
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> randByte(0, 255);

    std::vector<uint8_t> PRGROM(0x4000, 0x00);
    std::vector<uint8_t> CHRROM(0x2000);
    std::vector<uint8_t> CHRExpected;

    // Fill CHR ROM with random values.
    for(auto & byte : CHRROM) {
        uint8_t ran = randByte(rng);
        byte = ran;
        CHRExpected.push_back(ran);
    }

    Mapper000 m(PRGROM, CHRROM, Mapper::mirroringType_t::HORIZONTAL);
    uint8_t buffer;

    // Non mapped areas test.
    // ================================================
    for(uint32_t i = 0x2000; i < 0xFFFF; i++) {
        ASSERT_FALSE(m.ppuRead(i, buffer)) << "Error at address: " << std::hex << i;
        ASSERT_FALSE(m.ppuWrite(i, buffer)) << "Error at address: " << std::hex << i;
    }

    // CHR ROM test.
    // ================================================
    // Test for expected randomly generated values.
    for(uint32_t i = 0x0; i < 0x2000; i++) {
        ASSERT_TRUE(m.ppuRead(i, buffer));
        ASSERT_EQ(buffer, CHRExpected[i]) << "Error at address: " << std::hex << i;
    }

    // Write fixed values on even addresses.
    for(uint32_t i = 0x0; i < 0x2000; i += 2) {
        ASSERT_TRUE(m.ppuWrite(i, fillValue));
    }

    // Test whether only correct values were overwritten.
    for(uint32_t i = 0x0; i < 0x2000; i++) {
        ASSERT_TRUE(m.ppuRead(i, buffer));

        if(i % 2)
            ASSERT_EQ(buffer, CHRExpected[i]) << "Error at address: " << std::hex << i;
        else
            ASSERT_EQ(buffer, fillValue) << "Error at address: " << std::hex << i;
    }
}