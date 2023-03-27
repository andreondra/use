/**
 * @file TestGamepak.cpp NES Gamepak tests.
 * */

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
            EXPECT_EQ(m_params.mirroringType, Mapper::mirroringType_t::HORIZONTAL);
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
            EXPECT_EQ(m_params.mirroringType, Mapper::mirroringType_t::HORIZONTAL);
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