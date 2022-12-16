//
// Created by John on 15/12/2022.
//

#include "bus.h"

Bus::Bus(std::uint8_t *data, std::uint64_t len) {
    memcpy(m_dram.get_memory(), data, len);
}

std::uint64_t Bus::load(std::uint64_t addr, std::uint64_t size) {
    if (DRAM_BASE <= addr){
        return m_dram.load(addr, size);
    }
    assert(false && "Error: Cannot load address below 0x80000000, MMIO not implemented yet");
}

void Bus::store(std::uint64_t addr, std::uint64_t size, std::uint64_t data) {
    if (DRAM_BASE <= addr){
        m_dram.store(addr,size,data);
    }
    assert(false && "Error: Cannot write to an address below 0x80000000, MMIO not implemented yet");
}

