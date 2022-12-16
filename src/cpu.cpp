//
// Created by John on 15/12/2022.
//

#include "cpu.h"

#define MEMORY_SIZE (1024*1024*64) //constant for the default size of the ram

CPU::CPU() {
    m_pc = 0x80000000;
    m_integer_registers = new std::uint64_t[32];
    csrs = new std::uint64_t[4096];
    //setup x0
    m_integer_registers[0] = 0x0000000000000000;
    m_floating_point_registers = new std::uint64_t[32];

    bus = Bus();

    //setup x2 with the size of the memory when the cpu is instantiated
    m_integer_registers[2] = MEMORY_SIZE;
}

CPU::CPU(uint8_t *binary, uint64_t binary_size) {
    m_pc = 0x80000000;
    m_integer_registers = new std::uint64_t[32];
    csrs = new std::uint64_t[4096];

    //setup x0
    m_integer_registers[0] = 0x00000000;
    m_floating_point_registers = new std::uint64_t[32];

    bus = Bus(binary, binary_size);

    //setup x2 with the size of the memory when the cpu is instantiated
    m_integer_registers[2] = MEMORY_SIZE;
}

CPU::~CPU() {
    delete[] csrs;
    delete[] m_integer_registers;
    delete[] m_floating_point_registers;
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

int8_t CPU::cycle() {
    std::uint64_t current_instruction = fetch();
    //maybe update pc here, although I feel updating it during execution will give more flexibility
    execute(current_instruction);
    if (m_pc == 0x0){ //hack to stop infinite loops
        return -1;
    }
    return 0;
}

std::uint64_t CPU::fetch() {
    return bus.load(m_pc, 32);
}

void CPU::execute(std::uint64_t instruction) {
    std::uint64_t opcode = instruction & 0x7f;
    std::uint64_t rd = (instruction >> 7) & 0x1f;
    std::uint64_t rs1 = (instruction >> 15) & 0x1f;
    std::uint64_t rs2 = (instruction >> 20) & 0x1f;
    std::uint64_t funct3 = (instruction >> 12) & 0x7;

    std::uint64_t imm, addr;

    printf("Instruction %08lX : ", instruction);

    switch (opcode) {
        case 0x03: //Load Instructions
            imm = (int64_t)((int32_t)(instruction & 0xfff00000)) >> 20;
            addr = read_integer_register(rs1) + imm;
            switch (funct3) {
                case 0x0: //lb
                    std::printf("lb: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    write_integer_register(rd, ((int64_t)((int8_t)bus.load(addr, 8))));
                    m_pc+=4;
                    break;
                case 0x1: //lh
                    std::printf("lh: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    write_integer_register(rd, ((int64_t)((int16_t)bus.load(addr, 16))));
                    m_pc+=4;
                    break;
                case 0x2: //lw
                    std::printf("lw: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    write_integer_register(rd, ((int64_t)((int32_t)bus.load(addr, 32))));
                    m_pc+=4;
                    break;
                case 0x3: //ld
                    std::printf("ld: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    write_integer_register(rd, bus.load(addr, 64));
                    m_pc+=4;
                    break;
                case 0x4: //lbu
                    std::printf("lbu: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    write_integer_register(rd, bus.load(addr, 8));
                    m_pc+=4;
                    break;
                case 0x5: //lhu
                    std::printf("lhu: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    write_integer_register(rd, bus.load(addr, 16));
                    m_pc+=4;
                    break;
                case 0x6: //lwu
                    std::printf("lwu: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    write_integer_register(rd, bus.load(addr, 32));
                    m_pc+=4;
                    break;
                default:
                    break;
            }
            break;
        case 0x23: //Store Instructions
            imm = ((int64_t)((int32_t)(instruction & 0xfe000000)) >> 20) | ((instruction >> 7) & 0x1f);
            addr = read_integer_register(rs1) + imm;
            switch (funct3) {
                case 0x0: //sb
                    std::printf("sb: rs2=x%02ld addr=0x%08ld\n", rs2, addr);
                    bus.store(addr, 8, read_integer_register(rs2));
                    m_pc+=4;
                    break;
                case 0x1: //sh
                    std::printf("sh: rs2=x%02ld addr=0x%08ld\n", rs2, addr);
                    bus.store(addr, 16, read_integer_register(rs2));
                    m_pc+=4;
                    break;
                case 0x2: //sw
                    std::printf("sw: rs2=x%02ld addr=0x%08ld\n", rs2, addr);
                    bus.store(addr, 32, read_integer_register(rs2));
                    m_pc+=4;
                    break;
                case 0x3: //sd
                    std::printf("sd: rs2=x%02ld addr=0x%08ld\n", rs2, addr);
                    bus.store(addr, 64, read_integer_register(rs2));
                    m_pc+=4;
                    break;
                default:
                    break;
            }
            break;
        case 0x13: //addi
            imm = (int64_t)((int32_t)(instruction & 0xfff00000)) >> 20;
            std::printf("addi: rd=x%02ld imm=0x%08lX (%ld) rs1=x%02ld\n", rd, imm, imm, rs1);
            write_integer_register(rd, read_integer_register(rs1) + imm);
            m_pc+=4;
            break;
        case 0x33: //add
            std::printf("add: rd=x%02ld rs1=x%02ld rs2=x%02ld\n", rd, rs1, rs2);
            write_integer_register(rd, read_integer_register(rs1) + read_integer_register(rs2));
            m_pc+=4;
            break;
        default:
            std::printf("ERROR: Instruction with opcode %04lX : Not Implemented\n", opcode);
            m_pc+=4;
            break;
    }

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

void CPU::loop() {
    while (cycle() == 0){
        //dump_registers();
    }
}

std::uint64_t CPU::load_csr(std::uint64_t addr) {
    if (addr == 0x104) //SIE
        return csrs[0x304] & csrs[0x303]; //MIE & MIDELEG
    else
        return csrs[addr];
}

void CPU::load_csr(std::uint64_t addr, std::uint64_t value) {
    if (addr == 0x104) //SIE
        csrs[0x304] = (csrs[0x304] & !csrs[0x303]) | (value & csrs[0x303]); //MIE = (MIE & MIDELEG) | (value & MIDELEG)
    else
        csrs[addr] = value;
}



