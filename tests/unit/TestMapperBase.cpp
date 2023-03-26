/**
 * @file TestMapperBase.cpp NES Gamepak Mapper interface tests.
 *
 * The common interface contains only CIRAM handling.
 * */

#include "gtest/gtest.h"
#include "components/Gamepak/Mapper.h"

// Test CIRAM handling.
TEST(TestMapperBase, CIRAM) {

    // Create bare Mapper with mocked interface to test CIRAM.
    class DUT : public Mapper {
    public:
        bool cpuRead(uint16_t addr, uint8_t & data) override {
            return true;
        }
        bool cpuWrite(uint16_t addr, uint8_t data) override {
            return true;
        }
        bool ppuRead(uint16_t addr, uint8_t & data) override {

           if (addr >= 0x2000 && addr <= 0x3EFF) {
                data = CIRAMRead(addr);
                return true;
            }

           return false;
        }
        bool ppuWrite(uint16_t addr, uint8_t data) override {

            if(addr >= 0x2000 && addr <= 0x3EFF) {
                CIRAMWrite(addr, data);
                return true;
            }
            return false;
        }
        void drawGUI() override {};

        // Fake mirroring to test various CIRAM modes.
        void setMirroring(mirroringType_t mode) {
            m_mirroringType = mode;
        }
    } mapper;

    uint8_t buffer;
    // Fill NT 1.
    for(uint32_t i = 0x2000; i <= 0x23FF; i++) {
        mapper.ppuWrite(i, 0xAA);
    }

    // Read NT 1.
    for(uint32_t i = 0x2000; i <= 0x23FF; i++) {
        mapper.ppuRead(i, buffer);
        EXPECT_EQ(buffer, 0xAA);
    }

    // Fill NT 2.
    for(uint32_t i = 0x2400; i <= 0x27FF; i++) {
        mapper.ppuWrite(i, 0xBB);
    }

    // Read NT 1. It should be overwritten because of horizontal mirroring of nestest.
    for(uint32_t i = 0x2000; i <= 0x23FF; i++) {
        mapper.ppuRead(i, buffer);
        EXPECT_EQ(buffer, 0xBB);
    }

    // Read NT 2.
    for(uint32_t i = 0x2400; i <= 0x27FF; i++) {
        mapper.ppuRead(i, buffer);
        EXPECT_EQ(buffer, 0xBB);
    }

    // Write to NT 3.
    for(uint32_t i = 0x2800; i <= 0x2BFF; i++) {
        mapper.ppuWrite(i, 0xCC);
    }

    // Read from NT 3.
    for(uint32_t i = 0x2800; i <= 0x2BFF; i++) {
        mapper.ppuRead(i, buffer);
        EXPECT_EQ(buffer, 0xCC);
    }

    // Write to NT 4.
    for(uint32_t i = 0x2C00; i <= 0x2FFF; i++) {
        mapper.ppuWrite(i, 0xDD);
    }

    // Read from NT 3. It should be overwritten because of horizontal mirroring of nestest.
    for(uint32_t i = 0x2800; i <= 0x2BFF; i++) {
        mapper.ppuRead(i, buffer);
        EXPECT_EQ(buffer, 0xDD);
    }

    // Read from NT 4.
    for(uint32_t i = 0x2C00; i <= 0x2FFF; i++) {
        mapper.ppuRead(i, buffer);
        EXPECT_EQ(buffer, 0xDD);
    }

    // Verify by reading mirrored ranges.
    // NT1 and NT2.
    for(uint32_t i = 0x3000; i <= 0x37FF; i++) {
        mapper.ppuRead(i, buffer);
        EXPECT_EQ(buffer, 0xBB);
    }
    // NT3 and NT4.
    for(uint32_t i = 0x3800; i <= 0x3EFF; i++) {
        mapper.ppuRead(i, buffer);
        EXPECT_EQ(buffer, 0xDD);
    }

    // Switch to vertical mirroring.
    mapper.setMirroring(Mapper::mirroringType_t::VERTICAL);
    // Write to NT1 and read from NT3.
    for(uint32_t i = 0x2000; i <= 0x23FF; i++) {
        mapper.ppuWrite(i, 0x10);
    }

    for(uint32_t i = 0x2800; i <= 0x2BFF; i++) {
        mapper.ppuRead(i, buffer);
        EXPECT_EQ(buffer, 0x10);
    }

    // Write to NT2 and read from NT4.
    for(uint32_t i = 0x2400; i <= 0x27FF; i++) {
        mapper.ppuWrite(i, 0x20);
    }

    for(uint32_t i = 0x2C00; i <= 0x2FFF; i++) {
        mapper.ppuRead(i, buffer);
        EXPECT_EQ(buffer, 0x20);
    }

    // Verify by writing and reading from mirrors.
    // Reading from mirrored NT 1.
    for(uint32_t i = 0x3000; i <= 0x33FF; i++) {
        mapper.ppuRead(i, buffer);
        EXPECT_EQ(buffer, 0x10);
    }

    // Writing to mirrored NT 2 and reading from NT 4.
    for(uint32_t i = 0x3400; i <= 0x37FF; i++) {
        mapper.ppuWrite(i, 0x30);
    }
    for(uint32_t i = 0x2C00; i <= 0x2FFF; i++) {
        mapper.ppuRead(i, buffer);
        EXPECT_EQ(buffer, 0x30);
    }

    // Onescreen LO.
    mapper.setMirroring(Mapper::mirroringType_t::SINGLE_LO);
    for(uint32_t i = 0x2000; i <= 0x23FF; i++){
        mapper.ppuWrite(i, 0xFF);
    }
    for(uint32_t i = 0x2000; i <= 0x3EFF; i++){
        mapper.ppuRead(i, buffer);
        EXPECT_EQ(buffer, 0xFF);
    }

    // Onescreen HI.
    mapper.setMirroring(Mapper::mirroringType_t::SINGLE_HI);
    for(uint32_t i = 0x2400; i <= 0x27FF; i++){
        mapper.ppuWrite(i, 0x55);
    }
    for(uint32_t i = 0x2000; i <= 0x3EFF; i++){
        mapper.ppuRead(i, buffer);
        EXPECT_EQ(buffer, 0x55);
    }
}
