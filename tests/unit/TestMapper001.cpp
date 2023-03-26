/**
 * @file TestMapper001.cpp Test Mapper 001 class.
 * */

#include <random>
#include "gtest/gtest.h"
#include "components/Gamepak/Mapper001.h"

/// Helper function used to write 5-bit data to the Mapper 001's serial port.
static void serialWrite(Mapper001 & m, uint16_t address, uint8_t data) {
    for(int i = 0; i < 5; i++) {
        m.cpuWrite(address, data & 0x1);
        data >>= 1;
    }
}

// Test construction of the Mapper 001.
TEST(TestMapper001, Construction) {

    std::vector<uint8_t> PRGROM;
    std::vector<uint8_t> CHRROM;

    // Empty ROMs.
    EXPECT_THROW(Mapper001(PRGROM, CHRROM), std::invalid_argument);

    // Invalid sized CHR ROM.
    PRGROM.resize(0x40000);
    CHRROM.resize(0x40000);
    EXPECT_THROW(Mapper001(PRGROM, CHRROM), std::invalid_argument);

    // Correctly sized ROMs.
    CHRROM.resize(0x20000);
    PRGROM.resize(0x40000);
    EXPECT_NO_THROW(Mapper001(PRGROM, CHRROM));
    PRGROM.resize(0x80000);
    EXPECT_NO_THROW(Mapper001(PRGROM, CHRROM));

    // Invalid sized PRG RAM.
    EXPECT_THROW(Mapper001(PRGROM, CHRROM, 0x00), std::invalid_argument);
    EXPECT_THROW(Mapper001(PRGROM, CHRROM, 0x9000), std::invalid_argument);
}

// Test CPU RAM interface including bankswitching.
TEST(TestMapper001, CPUIORAM) {

    // Common test routine.
    auto test = [](int bankCount){

        uint8_t buffer;
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> randByte(0, 255);

        std::vector<uint8_t> PRGROM(0x40000, 0x00);
        std::vector<uint8_t> CHRROM(0x20000, 0x00);
        std::array<std::vector<uint8_t>, 4> banks;
        Mapper001 m(PRGROM, CHRROM);

        // Write to all four banks.
        uint8_t bankID = 0x0;
        for(auto & bank : banks) {

            // Switch banks.
            serialWrite(m, 0xA000, (bankID) << 2);

            for(uint32_t i = 0x6000; i < 0x8000; i++) {
                bank.push_back(randByte(rng));
                ASSERT_TRUE(m.cpuWrite(i, bank.back()));
            }

            bankID++;
        }

        // Read from all four banks.
        bankID = 0x0;
        for(auto & bank : banks) {

            // Switch banks.
            serialWrite(m, 0xA000, (bankID) << 2);

            for(uint32_t i = 0x6000; i < 0x8000; i++) {
                ASSERT_TRUE(m.cpuRead(i, buffer));
                ASSERT_EQ(buffer, bank[i & 0x1FFF]) << "Failed on bank " << (int)bankID << " address: " << std::hex << i;
            }

            bankID++;
        }
    };

    // 32, 16 and 8 KiB test.
    // ================================================
    test(4);
    test(2);
    test(1);
}

// Test CPU ROM interface including bankswitching.
TEST(TestMapper001, CPUIOROM) {

    // Common test routine.
    auto test = [](int mode){

        constexpr int bankCount = 0x10;
        uint8_t buffer;
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> randByte(0, 255);

        std::vector<uint8_t> PRGROM(0x4000 * bankCount, 0x00);
        std::vector<uint8_t> CHRROM(0x20000, 0x00);
        std::vector<uint8_t> PRGExpected;

        Mapper001 m(PRGROM, CHRROM);

        // Fill PRG ROM with random data.
        for(auto & byte : PRGROM) {
            PRGExpected.push_back(randByte(rng));
            byte = PRGExpected.back();
        }

        // Set PRG switching mode.
        serialWrite(m, 0x8000, mode << 2);

        // Read from all banks.
        switch(mode) {
            // 32 KiB mode.
            case 0:
            [[fallthrough]];
            case 1:
                for(int bank = 0; bank < bankCount; bank++) {
                    // Switch banks. (Bank ID corresponds to the high address bytes).
                    serialWrite(m, 0xE000, bank);
                    for (uint32_t addr = 0x8000; addr <= 0xFFFF; addr++) {
                        ASSERT_TRUE(m.cpuRead(addr, buffer));
                        ASSERT_EQ(buffer, PRGExpected[((bank & ~0x1) << 14) | (addr & 0x7FFF)]) << "Failed on bank " << (int) bank << " address: " << std::hex << addr;
                    }
                }
                break;
            // Low fixed, high switchable.
            case 2:
                for(int bank = 0; bank < bankCount; bank++) {
                    // Switch banks. (Bank ID corresponds to the high address bytes).
                    serialWrite(m, 0xE000, bank);

                    for (uint32_t addr = 0x8000; addr < 0xC000; addr++) {
                        ASSERT_TRUE(m.cpuRead(addr, buffer));
                        ASSERT_EQ(buffer, PRGExpected[addr & 0x3FFF]) << "Failed on bank " << (int) bank << " address: " << std::hex << addr;
                    }

                    for (uint32_t addr = 0xC000; addr <= 0xFFFF; addr++) {
                        ASSERT_TRUE(m.cpuRead(addr, buffer));
                        ASSERT_EQ(buffer, PRGExpected[(bank << 14) | (addr & 0x3FFF)]) << "Failed on bank " << (int) bank << " address: " << std::hex << addr;
                    }
                }
                break;
            // High fixed, low switchable.
            default:
                for(int bank = 0; bank < bankCount; bank++) {
                    // Switch banks. (Bank ID corresponds to the high address bytes).
                    serialWrite(m, 0xE000, bank);

                    for (uint32_t addr = 0x8000; addr < 0xC000; addr++) {
                        ASSERT_TRUE(m.cpuRead(addr, buffer));
                        ASSERT_EQ(buffer, PRGExpected[(bank << 14) | (addr & 0x3FFF)]) << "Failed on bank " << (int) bank << " address: " << std::hex << addr;
                    }

                    for (uint32_t addr = 0xC000; addr <= 0xFFFF; addr++) {
                        ASSERT_TRUE(m.cpuRead(addr, buffer));
                        ASSERT_EQ(buffer, PRGExpected[((bankCount - 1) << 14) | addr & 0x3FFF]) << "Failed on bank " << (int) bank << " address: " << std::hex << addr;
                    }
                }
                break;
        }
    };

    // All modes test.
    // ================================================
    for(int i = 0; i < 4; i++)
        test(i);
}