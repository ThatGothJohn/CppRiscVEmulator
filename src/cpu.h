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
    CPU(uint8_t*, uint64_t);
    ~CPU();

    int8_t cycle();
    void loop();

    void dump_registers();
    void dump_csrs();
private:
    enum Mode {
        User = 0b00,
        Supervisor = 0b01,
        Machine = 0b11,
    };

    std::uint64_t m_pc{};
    Bus bus;
    Mode mode;
    std::uint64_t* csrs; //Control and status registers
    std::uint64_t* m_integer_registers{};
    std::uint64_t* m_floating_point_registers{};

    uint64_t load(uint64_t, uint64_t);
    void store(uint64_t, uint64_t, uint64_t);

    uint64_t load_integer_register(std::uint64_t reg);
    void store_integer_register(std::uint64_t reg, std::uint64_t data);

    std::uint64_t load_csr(std::uint64_t);
    void store_csr(std::uint64_t,std::uint64_t);

    std::uint64_t fetch();
    uint8_t execute(std::uint64_t);
};
//A whole bunch of constants for CSR addresses

// Machine-level CSRs.
/// Hardware thread ID.
#define MHARTID 0xf14
/// Machine status register.
#define MSTATUS 0x300
/// Machine exception delefation register.
#define MEDELEG 0x302
/// Machine interrupt delefation register.
#define MIDELEG 0x303
/// Machine interrupt-enable register.
#define MIE 0x304
/// Machine trap-handler base address.
#define MTVEC 0x305
/// Machine counter enable.
#define MCOUNTEREN 0x306
/// Scratch register for machine trap handlers.
#define MSCRATCH 0x340
/// Machine exception program counter.
#define MEPC 0x341
/// Machine trap cause.
#define MCAUSE 0x342
/// Machine bad address or instruction.
#define MTVAL 0x343
/// Machine interrupt pending.
#define MIP 0x344

// Supervisor-level CSRs.
/// Supervisor status register.
#define SSTATUS 0x100
/// Supervisor interrupt-enable register.
#define SIE 0x104
/// Supervisor trap handler base address.
#define STVEC 0x105
/// Scratch register for supervisor trap handlers.
#define SSCRATCH 0x140
/// Supervisor exception program counter.
#define SEPC 0x141
/// Supervisor trap cause.
#define SCAUSE 0x142
/// Supervisor bad address or instruction.
#define STVAL 0x143
/// Supervisor interrupt pending.
#define SIP 0x144
/// Supervisor address translation and protection.
#define SATP 0x180

#endif //CPPRV64_CPU_H
