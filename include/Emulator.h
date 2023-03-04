//
// Created by golas on 21.2.23.
//

#ifndef USE_EMULATOR_H
#define USE_EMULATOR_H

class Emulator{

private:
    void guiStatusBar();
    void guiToolbar();
    void guiMenuItems();
    void setup();
    void mainLoop();

public:
    Emulator() = default;
    int run();
};

#endif //USE_EMULATOR_H
