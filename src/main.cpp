//
// Created by John on 15/12/2022.
//
#include <fstream>

#include "cpu.h"

#include <algorithm>

void loadFile(const std::string& filename, uint8_t* data, std::int64_t size) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        printf("Error: File Not Found\n");
        return;
    }
    printf("Successfully loaded %s\n", filename.c_str());
    file.read(reinterpret_cast<char*>(data), size);
}

int main(int argc, char** argv){
    std::string filename;
    if (argc < 2){
        filename = "../fib.bin";
    }
    else {
        filename = *argv[1];
    }
    auto code = new uint8_t[1024];

    loadFile(filename, code, 1024);

//    for (int a = 0; a<1024; a++) {
//        if (a != 0 && a % 64 == 0)
//            printf("\n");
//        printf("%02X ", code[a]);
//    }
//    printf("\n");

    auto test = CPU(code, 1024);

    delete[] code;
//    std::uint8_t code[] = {0x93, 0x0E, 0x50, 0x00,
//                           0x13, 0x0F, 0x50, 0x02,
//                           0xB3, 0x0F, 0xDF, 0x01}; //this is in add-addi.bin
//
//    auto test = CPU(code, 12);

//    test.dump_registers();
//    test.cycle();
//    test.cycle();
//    test.cycle();
    test.dump_registers();
    test.loop();
    return 0;
}