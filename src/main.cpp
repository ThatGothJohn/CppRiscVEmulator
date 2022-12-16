//
// Created by John on 15/12/2022.
//
#include <istream>

#include "cpu.h"

int main(int argc, char** argv){
    (void)argc,(void)argv;

    std::uint8_t code[] = {0x93, 0x0E, 0x50, 0x00,
                           0x13, 0x0F, 0x50, 0x02,
                            0xB3, 0x0F, 0xDF, 0x01};

    auto test = CPU(code, 24);

    test.dump_registers();
    test.cycle();
    test.cycle();
    test.cycle();
    test.dump_registers();

//    test.loop();
    return 0;
}