//
// Created by John on 15/12/2022.
//

#ifndef CPPRV64_MEMORY_H
#define CPPRV64_MEMORY_H

#include <cstdint>
#include <cassert>
#include <cstring>

#define DRAM_BASE 0x80000000
#define MEMORY_SIZE (1024*1024*128) //constant for the default size of the ram (128MiB)

struct Memory {
private:
    std::uint8_t* memory;

    uint64_t load8(uint64_t);
    uint64_t load16(uint64_t);
    uint64_t load32(uint64_t);
    uint64_t load64(uint64_t);

    void store8(uint64_t, uint64_t);
    void store16(uint64_t, uint64_t);
    void store32(uint64_t, uint64_t);
    void store64(uint64_t, uint64_t);
public:
    Memory(){
        memory = new std::uint8_t[MEMORY_SIZE]();
    }

    Memory(uint8_t* code, uint64_t len){
        memory = new std::uint8_t[MEMORY_SIZE]();
        assert(len < MEMORY_SIZE && "Tried loading a binary that is too large");
        std::memcpy(memory, code, len);
    }

    ~Memory() = default;

    std::uint64_t load(uint64_t, uint64_t);
    void store(uint64_t, uint64_t, uint64_t);
};

#endif //CPPRV64_MEMORY_H
