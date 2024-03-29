/**
 * @file TestTrigger.cpp Trigger tests.
 * */

#include "gtest/gtest.h"
#include "components/Trigger.h"

TEST(TestTrigger, Basic) {

    bool triggered = false;

    Trigger t(0xABC, 0x1);
    std::shared_ptr<Connector> c = std::make_shared<Connector>(
            SignalInterface{
                    .send = [& triggered](){
                        triggered = true;
                    }
            });

    t.connect("target", c);
    std::weak_ptr<Connector> trigInterface = t.getConnector("trigger");

    EXPECT_FALSE(triggered);

    trigInterface.lock()->getDataInterface().write(0xAAA, 0x5);
    EXPECT_FALSE(triggered);

    trigInterface.lock()->getDataInterface().write(0xAAA, 0x1);
    EXPECT_FALSE(triggered);

    trigInterface.lock()->getDataInterface().write(0xABC, 0x9);
    EXPECT_FALSE(triggered);

    trigInterface.lock()->getDataInterface().write(0xABC, 0x1);
    EXPECT_TRUE(triggered);

    trigInterface.lock()->getDataInterface().write(0x34, 0x1);
    EXPECT_TRUE(triggered);

    trigInterface.lock()->getDataInterface().write(0xABC, 0x1);
    EXPECT_TRUE(triggered);
}

TEST(TestTrigger, Masking) {

    bool triggered = false;

    Trigger t(0xABC, 0x80, 0x80);
    std::shared_ptr<Connector> c = std::make_shared<Connector>(
            SignalInterface{
                    .send = [& triggered](){
                        triggered = true;
                    }
            });

    t.connect("target", c);
    std::weak_ptr<Connector> trigInterface = t.getConnector("trigger");

    EXPECT_FALSE(triggered);

    trigInterface.lock()->getDataInterface().write(0xABC, 0x80);
    EXPECT_TRUE(triggered);
    triggered = false;

    trigInterface.lock()->getDataInterface().write(0xABC, 0xFF);
    EXPECT_TRUE(triggered);
    triggered = false;

    trigInterface.lock()->getDataInterface().write(0xABC, 0x7F);
    EXPECT_FALSE(triggered);
}