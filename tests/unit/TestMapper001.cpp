/**
 * @file TestMapper001.cpp Test Mapper 001 class.
 * */

#include "gtest/gtest.h"
#include "components/Gamepak/Mapper001.h"

// Test construction of the Mapper 001.
TEST(TestMapper001, Construction) {

    std::vector<uint8_t> PRGROM;
    std::vector<uint8_t> CHRROM;

    // Empty ROMs.
    EXPECT_THROW(Mapper001(PRGROM, CHRROM), std::invalid_argument);

    // Invalid sized PRG ROM.
    CHRROM.resize(0x20000);
    PRGROM.resize(0x10000);
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