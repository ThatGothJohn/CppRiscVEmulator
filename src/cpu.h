//
// Created by John on 15/12/2022.
//

#ifndef CPPRV64_CPU_H
#define CPPRV64_CPU_H

#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstdio>

class CPU {
public:
    CPU();
    CPU(uint8_t*, uint64_t);
    ~CPU();

    void cycle();
    void dump_registers();
private:
    std::uint64_t m_pc{};
    std::uint8_t* m_memory{};
    std::uint64_t* m_integer_registers{};
    std::uint64_t* m_floating_point_registers{};

    std::uint64_t fetch();
    void execute(std::uint64_t);

    void write_integer_register(std::uint64_t, std::uint64_t);
    uint64_t read_integer_register(std::uint64_t);
};

#endif //CPPRV64_CPU_H
