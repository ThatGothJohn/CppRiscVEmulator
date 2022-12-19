//
// Created by John on 15/12/2022.
//

#ifndef CPPRV64_CPU_H
#define CPPRV64_CPU_H

#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <string>

#include "bus.h"
#include "memory.h"

class CPU {
public:
    CPU();
    CPU(uint8_t*, uint64_t);
    ~CPU();

    int8_t cycle();
    void loop();
    void dump_registers();
private:
    std::uint64_t m_pc{};
    Bus bus;
    std::uint64_t* csrs; //Control and status registers
    std::uint64_t* m_integer_registers{};
    std::uint64_t* m_floating_point_registers{};

    std::uint64_t load_csr(std::uint64_t);
    void load_csr(std::uint64_t,std::uint64_t);

    std::uint64_t fetch();
    uint8_t execute(std::uint64_t);

    void write_integer_register(std::uint64_t, std::uint64_t);
    uint64_t read_integer_register(std::uint64_t);

    uint64_t load(uint64_t, uint64_t);
    void store(uint64_t, uint64_t, uint64_t);
};

#endif //CPPRV64_CPU_H
