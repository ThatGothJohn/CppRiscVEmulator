//
// Created by John on 15/12/2022.
//

#include "cpu.h"

#define MEMORY_SIZE (1024*1024*64) //constant for the default size of the ram

CPU::CPU() {
    m_pc = 0x0000000000000000;
    m_integer_registers = new std::uint64_t[32];
    //setup x0
    m_integer_registers[0] = 0x0000000000000000;
    m_floating_point_registers = new std::uint64_t[32];

    m_memory = new std::uint8_t[MEMORY_SIZE]; //64MiB Main memory

    //setup x2 with the size of the memory when the cpu is instantiated
    m_integer_registers[2] = MEMORY_SIZE;
}

CPU::CPU(uint8_t *binary, uint64_t binary_size) {
    m_pc = 0x0000000000000000;
    m_integer_registers = new std::uint64_t[32];
    //setup x0
    m_integer_registers[0] = 0x00000000;
    m_floating_point_registers = new std::uint64_t[32];

    m_memory = new std::uint8_t[MEMORY_SIZE]; //64MiB Main memory

    //setup x2 with the size of the memory when the cpu is instantiated
    m_integer_registers[2] = MEMORY_SIZE;

    assert(binary_size < MEMORY_SIZE-(1024*1024) && "Tried loading a binary that is too large");
    std::memcpy(m_memory, binary, binary_size);
}

CPU::~CPU() {
    delete[] m_integer_registers;
    delete[] m_floating_point_registers;
    delete[] m_memory;
}

void CPU::write_integer_register(std::uint64_t reg, std::uint64_t data) {
//    std::printf("Write %08lX to Reg x%ld\n", data, reg);
    assert(reg <= 32 && "attempted to write to an invalid integer register");
    if (reg == 0)
        return;
    m_integer_registers[reg] = data;
}

uint64_t CPU::read_integer_register(std::uint64_t reg) {
//    std::printf("Read Reg x%ld\n", reg);
    assert(reg <= 32 && "attempted to read from an invalid integer register");
    return m_integer_registers[reg];
}

void CPU::cycle() {
    std::uint64_t current_instruction = fetch();
    //maybe update pc here, although I feel updating it during execution will give more flexibility
    execute(current_instruction);
}

std::uint64_t CPU::fetch() {
    return ((std::uint64_t)m_memory[m_pc]) |
           (((std::uint64_t)m_memory[m_pc + 1]) << 8 ) |
           (((std::uint64_t)m_memory[m_pc + 2]) << 16) |
           (((std::uint64_t)m_memory[m_pc + 3]) << 24) |
           (((std::uint64_t)m_memory[m_pc + 4]) << 32) |
           (((std::uint64_t)m_memory[m_pc + 5]) << 40) |
           (((std::uint64_t)m_memory[m_pc + 6]) << 48) |
           (((std::uint64_t)m_memory[m_pc + 7]) << 56);
}

void CPU::execute(std::uint64_t instruction) {
    //fixme: this implementation will only work for specific RV32I instructions
    std::uint64_t opcode = instruction & 0x7f;
    std::uint64_t rd = (instruction >> 7) & 0x1f;
    std::uint64_t rs1 = (instruction >> 15) & 0x1f;
    std::uint64_t rs2 = (instruction >> 20) & 0x1f;

    printf("Instruction %08lX : ", instruction);

    switch (opcode) {
        case 0x13: //addi
            std::printf("addi: rd=x%02ld imm=0x%08lX rs1=x%02ld\n", rd, (instruction & 0xfff00000) >> 20, rs1);
            write_integer_register(rd, ((instruction & 0xfff00000) >> 20) + read_integer_register(rs1));
            break;
        case 0x33: //add
            std::printf("add: rd=x%02ld rs1=x%02ld rs2=x%02ld\n", rd, rs1, rs2);
            write_integer_register(rd, read_integer_register(rs1) + read_integer_register(rs2));
            break;
        default:
            std::printf("ERROR: Instruction with opcode %04lX : Not Implemented\n", opcode);
            break;
    }
    m_pc+=8;
}

void CPU::dump_registers() {
    std::printf("PC: %04lX\n", m_pc);
    for (int x = 0; x < 32; x++) {
        if (x%16 == 0 && x!=0)
            std::printf("\n");
        std::printf("x%d: %lX  ", x, m_integer_registers[x]);
    }
    std::printf("\n");
}



