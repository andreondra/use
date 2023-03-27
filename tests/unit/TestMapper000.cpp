/**
 * @file TestMapper000.cpp Test Mapper 000 class.
 * */

#include <random>
#include "gtest/gtest.h"
#include "components/Gamepak/Mapper000.h"

// Test construction of the Mapper 000.
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

    // Correctly sized ROMs.
    CHRROM.resize(0x2000);
    PRGROM.resize(0x4000);
    EXPECT_NO_THROW(Mapper000(PRGROM, CHRROM, Mapper::mirroringType_t::HORIZONTAL));

    CHRROM.resize(0x0);
    PRGROM.resize(0x8000);
    EXPECT_NO_THROW(Mapper000(PRGROM, CHRROM, Mapper::mirroringType_t::HORIZONTAL));

    // Wrong mirroring types.
    EXPECT_THROW(Mapper000(PRGROM, CHRROM, Mapper::mirroringType_t::SINGLE_HI), std::invalid_argument);
    EXPECT_THROW(Mapper000(PRGROM, CHRROM, Mapper::mirroringType_t::SINGLE_LO), std::invalid_argument);
    EXPECT_THROW(Mapper000(PRGROM, CHRROM, Mapper::mirroringType_t::FOURSCREEN), std::invalid_argument);
}

// Test CPU interface (PRG ROM and RAM).
TEST(TestMapper000, CPUIO) {

    const uint8_t fillValue = 0xBB;
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> randByte(0, 255);
    uint8_t buffer;

    // Test with 16 KiB of PRG ROM.
    {
        std::vector<uint8_t> PRGROM(0x4000, 0x00);
        std::vector<uint8_t> PRGExpected;
        std::vector<uint8_t> CHRROM;

        // Fill PRG ROM with random values.
        for(auto & byte : PRGROM) {
            PRGExpected.push_back(randByte(rng));
            byte = PRGExpected.back();
        }

        Mapper000 m(PRGROM, CHRROM, Mapper::mirroringType_t::HORIZONTAL);

        // Non mapped areas test.
        // ================================================
        for(uint32_t i = 0x0; i < 0x6000; i++) {
            EXPECT_FALSE(m.cpuRead(i, buffer)) << "Error at address: " << std::hex << i;
            EXPECT_FALSE(m.cpuWrite(i, buffer)) << "Error at address: " << std::hex << i;
        }

        // PRG ROM test.
        // ================================================
        // Test for expected randomly generated values.
        for(uint32_t i = 0x8000; i < 0xFFFF; i++) {
            EXPECT_TRUE(m.cpuRead(i, buffer));
            EXPECT_EQ(buffer, PRGExpected[i & 0x3FFF]) << "Error at address: " << std::hex << i;
        }

        // Write fixed values.
        for(uint32_t i = 0x8000; i < 0xFFFF; i ++) {
            EXPECT_FALSE(m.cpuWrite(i, fillValue));
        }

        // PRG RAM test.
        // ================================================
        for(uint32_t i = 0x6000; i < 0x8000; i++) {
            EXPECT_TRUE(m.cpuRead(i, buffer));
            EXPECT_EQ(buffer, 0x00);
        }

        for(uint32_t i = 0x6000; i < 0x8000; i++) {
            EXPECT_TRUE(m.cpuWrite(i, fillValue));
        }

        for(uint32_t i = 0x6000; i < 0x8000; i++) {
            EXPECT_TRUE(m.cpuRead(i, buffer));
            EXPECT_EQ(buffer, fillValue);
        }
    }

    // Test with 32 KiB of PRG ROM.
    {
        std::vector<uint8_t> PRGROM(0x8000, 0x00);
        std::vector<uint8_t> PRGExpected;
        std::vector<uint8_t> CHRROM;

        // Fill PRG ROM with random values.
        for(auto & byte : PRGROM) {
            PRGExpected.push_back(randByte(rng));
            byte = PRGExpected.back();
        }

        Mapper000 m(PRGROM, CHRROM, Mapper::mirroringType_t::HORIZONTAL);

        // PRG ROM test.
        // ================================================
        // Test for expected randomly generated values.
        for(uint32_t i = 0x8000; i < 0xFFFF; i++) {
            EXPECT_TRUE(m.cpuRead(i, buffer));
            EXPECT_EQ(buffer, PRGExpected[i & 0x7FFF]) << "Error at address: " << std::hex << i;
        }

        // Write fixed values.
        for(uint32_t i = 0x8000; i < 0xFFFF; i ++) {
            EXPECT_FALSE(m.cpuWrite(i, fillValue));
        }
    }
}

// Test PPU interface with CHR ROM.
TEST(TestMapper000, PPUIOCHRROM) {

    const uint8_t fillValue = 0xBB;
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> randByte(0, 255);

    std::vector<uint8_t> PRGROM(0x4000, 0x00);
    std::vector<uint8_t> CHRROM(0x2000);
    std::vector<uint8_t> CHRExpected;

    // Fill CHR ROM with random values.
    for(auto & byte : CHRROM) {
        CHRExpected.push_back(randByte(rng));
        byte = CHRExpected.back();
    }

    Mapper000 m(PRGROM, CHRROM, Mapper::mirroringType_t::HORIZONTAL);
    uint8_t buffer;

    // Non mapped areas test.
    // ================================================
    for(uint32_t i = 0x3F00; i < 0xFFFF; i++) {
        EXPECT_FALSE(m.ppuRead(i, buffer)) << "Error at address: " << std::hex << i;
        EXPECT_FALSE(m.ppuWrite(i, buffer)) << "Error at address: " << std::hex << i;
    }

    // CHR ROM test.
    // ================================================
    // Test for expected randomly generated values.
    for(uint32_t i = 0x0; i < 0x2000; i++) {
        EXPECT_TRUE(m.ppuRead(i, buffer));
        EXPECT_EQ(buffer, CHRExpected[i]) << "Error at address: " << std::hex << i;
    }

    // Write fixed values on even addresses.
    // Should return false in a case of ROM.
    for(uint32_t i = 0x0; i < 0x2000; i += 2) {
        EXPECT_FALSE(m.ppuWrite(i, fillValue));
    }

    // Nothing should have been overwritten.
    for(uint32_t i = 0x0; i < 0x2000; i++) {
        EXPECT_TRUE(m.ppuRead(i, buffer));
        EXPECT_EQ(buffer, CHRExpected[i]) << "Error at address: " << std::hex << i;
    }
}

// Test PPU interface with CHR RAM.
TEST(TestMapper000, PPUIOCHRRAM) {

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> randByte(0, 255);

    std::vector<uint8_t> PRGROM(0x4000, 0x00);
    // Empty CHR ROM = RAM will be used.
    std::vector<uint8_t> CHRROM;
    std::vector<uint8_t> CHRExpected;

    Mapper000 m(PRGROM, CHRROM, Mapper::mirroringType_t::HORIZONTAL);
    uint8_t buffer;

    // CHR ROM test.
    // ================================================
    // Fill with random values.
    // Should return true in a case of RAM.
    for(uint32_t i = 0x0; i < 0x2000; i++) {
        CHRExpected.push_back(randByte(rng));
        EXPECT_TRUE(m.ppuWrite(i, CHRExpected.back()));
    }

    // Test whether all values were written correctly.
    for(uint32_t i = 0x0; i < 0x2000; i++) {
        EXPECT_TRUE(m.ppuRead(i, buffer));
        EXPECT_EQ(buffer, CHRExpected[i]) << "Error at address: " << std::hex << i;
    }
}

// Test whether the CIRAM is correctly mapped.
// This does not test mirroring features (this is tested in the base class test).
TEST(TestMapper000, PPUIOCIRAM) {

    uint8_t fillValue = 0x55;
    std::vector<uint8_t> PRGROM(0x4000, 0x00);
    std::vector<uint8_t> CHRROM;

    Mapper000 m(PRGROM, CHRROM, Mapper::mirroringType_t::HORIZONTAL);
    uint8_t buffer;

    // CIRAM test.
    // ================================================
    // Fill with fixed value.
    for(uint32_t i = 0x2000; i < 0x3000; i++) {
        EXPECT_TRUE(m.ppuWrite(i, fillValue));
    }

    // Test whether all values were written correctly.
    for(uint32_t i = 0x2000; i < 0x3000; i++) {
        EXPECT_TRUE(m.ppuRead(i, buffer));
        EXPECT_EQ(buffer, fillValue) << "Error at address: " << std::hex << i;
    }
}