//
// Created by John on 15/12/2022.
//

#ifndef CPPRV64_BUS_H
#define CPPRV64_BUS_H

#include "memory.h"

struct Bus {
private:
    Memory m_dram;
public:
    Bus() = default;
    Bus(std::uint8_t*, std::uint64_t);
    ~Bus() = default;

    std::uint64_t load(std::uint64_t, std::uint64_t);
    void store(std::uint64_t,std::uint64_t,std::uint64_t);
};

#endif //CPPRV64_BUS_H
