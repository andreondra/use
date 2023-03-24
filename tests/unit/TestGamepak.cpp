//
// Created by golas on 23.3.23.
//

#include <fstream>
#include "gtest/gtest.h"
#include "components/Gamepak/Gamepak.h"

/**
 * Test Gamepak initial values.
 * */
TEST(TestGamepak, InitialValues) {

    class DUT : public Gamepak {
    public:
        void initTest() {
            EXPECT_EQ(m_params.PRGROMsize, 0);
            EXPECT_EQ(m_params.CHRROMsize, 0);
            EXPECT_EQ(m_params.mirroringType, mirroringType_t::HORIZONTAL);
            EXPECT_EQ(m_params.fileFormat, fileFormat_t::INES);
            EXPECT_EQ(m_params.hasPersistentMemory, false);
            EXPECT_EQ(m_params.hasTrainer, false);
            EXPECT_EQ(m_params.mapperNumber, 0);
            EXPECT_EQ(m_params.PRGRAMsize, 0);
            EXPECT_EQ(m_params.consoleType, consoleType_t::STANDARD);
            EXPECT_EQ(m_params.tvSystem, tvSystem_t::NTSC);
            EXPECT_EQ(m_params.submapperNumber, 0);
            EXPECT_EQ(m_params.PRGNVRAMsize, 0);
            EXPECT_EQ(m_params.CHRRAMsize, 0);
            EXPECT_EQ(m_params.CHRNVRAMsize, 0);
        }
    } gamepak;

    gamepak.initTest();
}

/**
 * Test Gamepak file parsing.
 * This is a white box test, thus is implementation-defined.
 *
 * Tests whether the header was correctly parsed and PRG/CHR data properly loaded.
 */
TEST(TestGamepak, FileLoad) {

    // Create a derived class with built-in testing procedure.
    class DUT : public Gamepak {
    public:
        void test(std::vector<uint8_t> & rawPRGData, std::vector<uint8_t> & rawCHRData) {
            EXPECT_EQ(m_params.PRGROMsize, 0x4000);
            EXPECT_EQ(m_params.CHRROMsize, 0x2000);
            EXPECT_EQ(m_params.mirroringType, mirroringType_t::HORIZONTAL);
            EXPECT_EQ(m_params.fileFormat, fileFormat_t::INES);
            EXPECT_EQ(m_params.hasPersistentMemory, false);
            EXPECT_EQ(m_params.hasTrainer, false);
            EXPECT_EQ(m_params.mapperNumber, 0);
            EXPECT_EQ(m_params.PRGRAMsize, 0x2000);
            EXPECT_EQ(m_params.consoleType, consoleType_t::STANDARD);
            EXPECT_EQ(m_params.tvSystem, tvSystem_t::NTSC);
            EXPECT_EQ(m_params.submapperNumber, 0);
            EXPECT_EQ(m_params.PRGNVRAMsize, 0);
            EXPECT_EQ(m_params.CHRRAMsize, 0);
            EXPECT_EQ(m_params.CHRNVRAMsize, 0);

            EXPECT_EQ(m_PRGROM, rawPRGData);
            EXPECT_EQ(m_CHRROM, rawCHRData);
        }
    } gamepak;

    // Load a test ROM.
    std::ifstream nestest("testfiles/nestest.nes", std::ios_base::binary);
    ASSERT_TRUE(nestest) << "Can't open nestest.nes!";
    EXPECT_NO_THROW(gamepak.load(nestest));

    // Load PRG data manually.
    nestest.seekg(0x10, std::ios::beg);
    std::vector<uint8_t> rawPRGData(0x4000, 0x00);
    nestest.read((char*)rawPRGData.data(), rawPRGData.size());

    // Load CHR data manually.
    nestest.seekg(0x4010, std::ios::beg);
    std::vector<uint8_t> rawCHRData(0x2000, 0x00);
    nestest.read((char*)rawCHRData.data(), rawCHRData.size());

    gamepak.test(rawPRGData, rawCHRData);

    // Test init fuction - it should not change any parameters.
    gamepak.init();
    gamepak.test(rawPRGData, rawCHRData);
}

TEST(TestGamepak, CIRAM) {

    class DUT : public Gamepak {

    public:
        // Fake mirroring to test various CIRAM modes.
        void setMirroring(int mode) {
            switch (mode) {
                case 0: m_params.mirroringType = mirroringType_t::HORIZONTAL; break;
                case 1: m_params.mirroringType = mirroringType_t::VERTICAL; break;
                case 2: m_params.mirroringType = mirroringType_t::SINGLE_LO; break;
                case 3: m_params.mirroringType = mirroringType_t::SINGLE_HI; break;
            }
        }
    } gamepak;

    std::ifstream nestest("testfiles/nestest.nes", std::ios_base::binary);
    ASSERT_TRUE(nestest) << "Can't open nestest.nes!";

    EXPECT_NO_THROW(gamepak.load(nestest));

    std::weak_ptr<Connector> ppuCon = gamepak.getConnector("ppu");

    uint32_t buffer;
    // Fill NT 1.
    for(uint32_t i = 0x2000; i <= 0x23FF; i++) {
        ppuCon.lock()->getDataInterface().write(i, 0xAA);
    }

    // Read NT 1.
    for(uint32_t i = 0x2000; i <= 0x23FF; i++) {
        ppuCon.lock()->getDataInterface().read(i, buffer);
        EXPECT_EQ(buffer, 0xAA);
    }

    // Fill NT 2.
    for(uint32_t i = 0x2400; i <= 0x27FF; i++) {
        ppuCon.lock()->getDataInterface().write(i, 0xBB);
    }

    // Read NT 1. It should be overwritten because of horizontal mirroring of nestest.
    for(uint32_t i = 0x2000; i <= 0x23FF; i++) {
        ppuCon.lock()->getDataInterface().read(i, buffer);
        EXPECT_EQ(buffer, 0xBB);
    }

    // Read NT 2.
    for(uint32_t i = 0x2400; i <= 0x27FF; i++) {
        ppuCon.lock()->getDataInterface().read(i, buffer);
        EXPECT_EQ(buffer, 0xBB);
    }

    // Write to NT 3.
    for(uint32_t i = 0x2800; i <= 0x2BFF; i++) {
        ppuCon.lock()->getDataInterface().write(i, 0xCC);
    }

    // Read from NT 3.
    for(uint32_t i = 0x2800; i <= 0x2BFF; i++) {
        ppuCon.lock()->getDataInterface().read(i, buffer);
        EXPECT_EQ(buffer, 0xCC);
    }

    // Write to NT 4.
    for(uint32_t i = 0x2C00; i <= 0x2FFF; i++) {
        ppuCon.lock()->getDataInterface().write(i, 0xDD);
    }

    // Read from NT 3. It should be overwritten because of horizontal mirroring of nestest.
    for(uint32_t i = 0x2800; i <= 0x2BFF; i++) {
        ppuCon.lock()->getDataInterface().read(i, buffer);
        EXPECT_EQ(buffer, 0xDD);
    }

    // Read from NT 4.
    for(uint32_t i = 0x2C00; i <= 0x2FFF; i++) {
        ppuCon.lock()->getDataInterface().read(i, buffer);
        EXPECT_EQ(buffer, 0xDD);
    }

    // Verify by reading mirrored ranges.
    // NT1 and NT2.
    for(uint32_t i = 0x3000; i <= 0x37FF; i++) {
        ppuCon.lock()->getDataInterface().read(i, buffer);
        EXPECT_EQ(buffer, 0xBB);
    }
    // NT3 and NT4.
    for(uint32_t i = 0x3800; i <= 0x3EFF; i++) {
        ppuCon.lock()->getDataInterface().read(i, buffer);
        EXPECT_EQ(buffer, 0xDD);
    }

    // Switch to vertical mirroring.
    gamepak.setMirroring(1);
    // Write to NT1 and read from NT3.
    for(uint32_t i = 0x2000; i <= 0x23FF; i++) {
        ppuCon.lock()->getDataInterface().write(i, 0x10);
    }

    for(uint32_t i = 0x2800; i <= 0x2BFF; i++) {
        ppuCon.lock()->getDataInterface().read(i, buffer);
        EXPECT_EQ(buffer, 0x10);
    }

    // Write to NT2 and read from NT4.
    for(uint32_t i = 0x2400; i <= 0x27FF; i++) {
        ppuCon.lock()->getDataInterface().write(i, 0x20);
    }

    for(uint32_t i = 0x2C00; i <= 0x2FFF; i++) {
        ppuCon.lock()->getDataInterface().read(i, buffer);
        EXPECT_EQ(buffer, 0x20);
    }

    // Verify by writing and reading from mirrors.
    // Reading from mirrored NT 1.
    for(uint32_t i = 0x3000; i <= 0x33FF; i++) {
        ppuCon.lock()->getDataInterface().read(i, buffer);
        EXPECT_EQ(buffer, 0x10);
    }

    // Writing to mirrored NT 2 and reading from NT 4.
    for(uint32_t i = 0x3400; i <= 0x37FF; i++) {
        ppuCon.lock()->getDataInterface().write(i, 0x30);
    }
    for(uint32_t i = 0x2C00; i <= 0x2FFF; i++) {
        ppuCon.lock()->getDataInterface().read(i, buffer);
        EXPECT_EQ(buffer, 0x30);
    }

    // Onescreen LO.
    gamepak.setMirroring(2);
    for(uint32_t i = 0x2000; i <= 0x23FF; i++){
        ppuCon.lock()->getDataInterface().write(i, 0xFF);
    }
    for(uint32_t i = 0x2000; i <= 0x3EFF; i++){
        ppuCon.lock()->getDataInterface().read(i, buffer);
        EXPECT_EQ(buffer, 0xFF);
    }

    // Onescreen HI.
    gamepak.setMirroring(3);
    for(uint32_t i = 0x2400; i <= 0x27FF; i++){
        ppuCon.lock()->getDataInterface().write(i, 0x55);
    }
    for(uint32_t i = 0x2000; i <= 0x3EFF; i++){
        ppuCon.lock()->getDataInterface().read(i, buffer);
        EXPECT_EQ(buffer, 0x55);
    }
}