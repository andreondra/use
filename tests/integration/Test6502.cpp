//
// Created by golas on 5.3.23.
//

#include "gtest/gtest.h"
#include "systems/Bare6502.h"

TEST(Test6502, Basic) {

    class SUT : public Bare6502{
    public:
        bool test() {
            return true;
        }
    };

    SUT sut;

    while(!sut.test()) {
        sut.doClocks(1);
    }
}