//
// Created by John on 15/12/2022.
//
#define DEBUG false

#include "cpu.h"

CPU::CPU(uint8_t *binary, uint64_t binary_size) {
    m_pc = DRAM_BASE;
    m_integer_registers = new std::uint64_t[32]();
    csrs = new std::uint64_t[4096]();

    m_floating_point_registers = new std::uint64_t[32]();

    mode = Mode::Machine;

    bus = Bus(binary, binary_size);

    //setup x2 (sp) with the size of the memory when the cpu is instantiated
    m_integer_registers[2] = DRAM_BASE+MEMORY_SIZE;
}

CPU::~CPU() {
    delete[] csrs;
    delete[] m_integer_registers;
    delete[] m_floating_point_registers;
}

void CPU::dump_registers() {
    auto output = new char[2048]();

    std::string abi[] = {"zero", " ra ", " sp ", " gp ", " tp ", " t0 ", " t1 ", " t2 ", " s0 ", " s1 ", " a0 ",
                         " a1 ", " a2 ", " a3 ", " a4 ", " a5 ", " a6 ", " a7 ", " s2 ", " s3 ", " s4 ", " s5 ",
                         " s6 ", " s7 ", " s8 ", " s9 ", " s10", " s11", " t3 ", " t4 ", " t5 ", " t6 "};

    for (int i = 0; i<32;i+=4){
        auto temp = new char[1024](); //more allocations than necessary, but this is for debugging, so it's all good
        std::sprintf(temp, "x%02d(%s):%18lX x%02d(%s):%18lX x%02d(%s):%18lX x%02d(%s):%18lX",
                     i, abi[i].c_str(), m_integer_registers[i],
                     i+1, abi[i+1].c_str(), m_integer_registers[i+1],
                     i+2, abi[i+2].c_str(), m_integer_registers[i+2],
                     i+3, abi[i+3].c_str(), m_integer_registers[i+3]
        );
        std::sprintf(output, "%s\n%s", output, temp);
        delete[] temp;
    }
    std::printf("PC: %18lX", m_pc);
    std::printf("%s\n", output);
    delete[] output;
}

void CPU::dump_csrs() {
    std::printf("mstatus:%18lX mtvec:%18lX mepc:%18lX mcause:%18lX\nsstatus:%18lX stvec:%18lX sepc:%18lX scause:%18lX\n",
                load_csr(MSTATUS), load_csr(MTVEC), load_csr(MEPC), load_csr(MCAUSE),
                load_csr(SSTATUS), load_csr(STVEC), load_csr(SEPC), load_csr(SCAUSE));
}

uint64_t CPU::load_integer_register(std::uint64_t reg) {
    assert(reg <= 32 && "attempted to read from an invalid integer register");
    return m_integer_registers[reg];
}

void CPU::store_integer_register(std::uint64_t reg, std::uint64_t data) {
    assert(reg <= 32 && "attempted to write to an invalid integer register");
    if (reg == 0) //emulate ZERO register
        return;
    m_integer_registers[reg] = data;
}

std::uint64_t CPU::load_csr(std::uint64_t addr) {
    if (addr == SIE)
        return csrs[MIE] & csrs[MIDELEG];
    else
        return csrs[addr];
}

void CPU::store_csr(std::uint64_t addr, std::uint64_t value) {
    if (addr == SIE)
        csrs[MIE] = (csrs[MIE] & !csrs[MIDELEG]) | (value & csrs[MIDELEG]);
    else
        csrs[addr] = value;
}

uint64_t CPU::load(uint64_t addr, uint64_t size) {
    return bus.load(addr, size);
}

void CPU::store(uint64_t addr, uint64_t size, uint64_t data) {
    bus.store(addr,size,data);
}

std::uint64_t CPU::fetch() {
    return load(m_pc, 32);
}

int8_t CPU::cycle() {
    std::uint64_t current_instruction = fetch();
    m_pc += 4;

    if (execute(current_instruction) != 0){ //return error on unknown instruction
        return -2;
    }
    if (m_pc == 0x0){ //hack to stop infinite loops
        return -1;
    }
    return 0;
}

void CPU::loop() {
    while (cycle() == 0){
#if DEBUG
        dump_registers();
        dump_csrs();
#endif
    }
}

uint8_t CPU::execute(std::uint64_t instruction) {
    std::uint64_t opcode = instruction & 0x7f;
    std::uint64_t rd = (instruction >> 7) & 0x1f;
    std::uint64_t rs1 = (instruction >> 15) & 0x1f;
    std::uint64_t rs2 = (instruction >> 20) & 0x1f;
    std::uint64_t funct3 = (instruction >> 12) & 0x7;
    std::uint64_t funct7 = (instruction >> 25) & 0x7f;

    std::uint64_t imm, addr, shamt, temp, funct5, csr_addr; // acquire, release will be needed for proper atomics

//    printf("Instruction %08lX : ", instruction);
    switch (opcode) {
        case 0x03: //Load Instructions
            imm = (int64_t)((int32_t) (instruction & 0xfff00000)) >> 20;
            addr = load_integer_register(rs1) + imm;
            switch (funct3) {
                case 0x0: //lb
//                    std::printf("lb: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    store_integer_register(rd, ((int64_t) ((int8_t) load(addr, 8))));
                    break;
                case 0x1: //lh
//                    std::printf("lh: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    store_integer_register(rd, ((int64_t) ((int16_t) load(addr, 16))));
                    break;
                case 0x2: //lw
//                    std::printf("lw: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    store_integer_register(rd, ((int64_t) ((int32_t) load(addr, 32))));
                    break;
                case 0x3: //ld
//                    std::printf("ld: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    store_integer_register(rd, load(addr, 64));
                    break;
                case 0x4: //lbu
//                    std::printf("lbu: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    store_integer_register(rd, load(addr, 8));
                    break;
                case 0x5: //lhu
//                    std::printf("lhu: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    store_integer_register(rd, load(addr, 16));
                    break;
                case 0x6: //lwu
//                    std::printf("lwu: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    store_integer_register(rd, load(addr, 32));
                    break;
                default:
                    std::printf("ERROR : Invalid Load : funct3=%lX\n", funct3);
                    return -1;
            }
            break;
        case 0x13:
            imm = (int64_t)((int32_t) (instruction & 0xfff00000)) >> 20;
            shamt = (std::uint32_t) (imm & 0x3f);

            switch (funct3) {
                case 0x0: //addi
//                    std::printf("addi: rd=x%02ld imm=0x%08lX (%ld) rs1=x%02ld\n", rd, imm, imm, rs1);
                    store_integer_register(rd, load_integer_register(rs1) + imm);
                    break;
                case 0x1: //slli
//                    std::printf("slli: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                    store_integer_register(rd, load_integer_register(rs1) << shamt);
                    break;
                case 0x2: //slti
//                    std::printf("slti: rd=x%02ld imm=0x%08lX (%ld) rs1=x%02ld\n", rd, imm, imm, rs1);
                    store_integer_register(rd, (int64_t) load_integer_register(rs1) < (int64_t) imm ? 1 : 0);
                    break;
                case 0x3: //sltiu
//                    std::printf("sltiu: rd=x%02ld imm=0x%08lX (%ld) rs1=x%02ld\n", rd, imm, imm, rs1);
                    store_integer_register(rd, load_integer_register(rs1) < imm ? 1 : 0);
                    break;
                case 0x4: //xori
//                    std::printf("xori: rd=x%02ld imm=0x%08lX (%ld) rs1=x%02ld\n", rd, imm, imm, rs1);
                    store_integer_register(rd, load_integer_register(rs1) ^ imm);
                    break;
                case 0x5: //rotate
                    switch (funct7 >> 1) {
                        case 0x00: //srli
//                            std::printf("srli: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                            store_integer_register(rd, (load_integer_register(rs1) >> shamt)
                                                       | (load_integer_register(rs1) << (64 - shamt)));
                            break;
                        case 0x10: //srai
//                            std::printf("srai: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                            store_integer_register(rd, ((int64_t) load_integer_register(rs1) >> shamt)
                                                       | ((int64_t) load_integer_register(rs1) << (64 - shamt)));
                            break;
                        default:
                            std::printf("ERROR : Rotate Not Implemented Yet : funct7=%lX\n", funct7);
                            return -1;
                    }
                    break;
                case 0x6: //ori
//                    std::printf("ori: rd=x%02ld imm=0x%08lX (%ld) rs1=x%02ld\n", rd, imm, imm, rs1);
                    store_integer_register(rd, load_integer_register(rs1) | imm);
                    break;
                case 0x7: //andi
//                    std::printf("andi: rd=x%02ld imm=0x%08lX (%ld) rs1=x%02ld\n", rd, imm, imm, rs1);
                    store_integer_register(rd, load_integer_register(rs1) & imm);
                    break;
                default:
                    std::printf("ERROR : Not Implemented Yet : funct3=%lX\n", funct3);
                    return -1;
            }
            break;
        case 0x17: //auipc
            imm = (int64_t) ((int32_t) (instruction & 0xFFFFF000));
//            std::printf("auipc: rd=x%02ld imm=0x%08lX (%ld)\n", rd, imm, imm);
            store_integer_register(rd, m_pc + imm - 4);
            break;
        case 0x1b:
            imm = (int64_t) ((int32_t) instruction) >> 20;
            shamt = (uint32_t) (imm & 0x1f);
            switch (funct3) {
                case 0x0: //addiw
//                    std::printf("addiw: rd=x%02ld imm=0x%08lX (%ld) rs1=x%02ld\n", rd, imm, imm, rs1);
                    store_integer_register(rd, (int64_t) ((int32_t) (load_integer_register(rs1) + imm)));
                    break;
                case 0x1: //slliw
//                    std::printf("slliw: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                    store_integer_register(rd, (int64_t) ((int32_t) (((uint32_t) load_integer_register(rs1) << shamt)
                                                                     | ((uint32_t) load_integer_register(rs1)
                            >> (32 - shamt)))));
                    break;
                case 0x5:
                    switch (funct7) {
                        case 0x00: //srliw
//                            std::printf("srliw: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                            store_integer_register(rd, (int64_t) ((int32_t) (
                                    ((uint32_t) load_integer_register(rs1) >> shamt)
                                    | ((uint32_t) load_integer_register(rs1) << (32 - shamt)))));
                            break;
                        case 0x20: //sraiw
//                            std::printf("sraiw: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                            store_integer_register(rd,
                                                   (int64_t) ((int32_t) (((int32_t) load_integer_register(rs1) >> shamt)
                                                                         | ((int32_t) load_integer_register(rs1)
                                                           << (32 - shamt)))));
                            break;
                        default:
                            std::printf("ERROR : Not Implemented Yet : funct7=%lX\n", funct7);
                            return -1;
                    }
                    break;
                default:
                    std::printf("ERROR : Not Implemented Yet : funct3=%lX\n", funct3);
                    return -1;
            }
            break;
        case 0x23: //Store Instructions
            imm = ((uint64_t)((int64_t)((int32_t)(instruction & 0xFE000000)) >> 20)) | ((instruction >> 7) & 0x1F);
            addr = (load_integer_register(rs1) + imm) % 0xFFFFFFFF;
            switch (funct3) {
                case 0x0: //sb
//                    std::printf("sb: rs2=x%02ld addr=0x%08ld\n", rs2, addr);
                    store(addr, 8, load_integer_register(rs2));
                    break;
                case 0x1: //sh
//                    std::printf("sh: rs2=x%02ld addr=0x%08ld\n", rs2, addr);
                    store(addr, 16, load_integer_register(rs2));
                    break;
                case 0x2: //sw
//                    std::printf("sw: rs2=x%02ld addr=0x%08ld\n", rs2, addr);
                    store(addr, 32, load_integer_register(rs2));
                    break;
                case 0x3: //sd
//                    std::printf("sd: rs2=x%02ld addr=0x%08ld\n", rs2, addr);
                    store(addr, 64, load_integer_register(rs2));
                    break;
                default:
                    std::printf("ERROR : Invalid Store : funct3=%lX\n", funct3);
                    return -1;
            }
            break;
        case 0x2f: //RV64A : Atomic instructions
            funct5 = (funct7 & 0b11111100) >> 2;
            //acquire = (funct7 & 0b00000010) >> 1; //acquire access
            //release = funct7 & 0b00000001; //release access
            switch (funct3) {
                case 0x2:
                    switch (funct5) {
                        case 0x00: //amoadd.w
                            temp = load(load_integer_register(rs1), 32);
                            store(load_integer_register(rs1), 32, load_integer_register(rs2) + temp);
                            store_integer_register(rd, temp);
                            break;
                        case 0x01: //amoswap.w
                            temp = load(load_integer_register(rs1), 32);
                            store(load_integer_register(rs1), 32, load_integer_register(rs2));
                            store_integer_register(rd, temp);
                            break;
                        default:
                            std::printf("ERROR : Not Implemented Yet : funct5=%lX\n", funct5);
                            return -1;
                    }
                    break;
                case 0x3:
                    switch (funct5) {
                        case 0x00: //amoadd.d
                            temp = load(load_integer_register(rs1), 64);
                            store(load_integer_register(rs1), 32, load_integer_register(rs2) + temp);
                            store_integer_register(rd, temp);
                            break;
                        case 0x01: //amoswap.d
                            temp = load(load_integer_register(rs1), 64);
                            store(load_integer_register(rs1), 32, load_integer_register(rs2));
                            store_integer_register(rd, temp);
                            break;
                        default:
                            std::printf("ERROR : Not Implemented Yet : funct5=%lX\n", funct5);
                            return -1;
                    }
                    break;
                default:
                    std::printf("ERROR : Not Implemented Yet : funct3=%lX\n", funct3);
                    return -1;
            }
            break;
        case 0x33:
            shamt = (uint32_t) (load_integer_register(rs2) & 0x3f);
            switch (funct3) {
                case 0x0:
                    switch (funct7) {
                        case 0x00: //add
//                            std::printf("add: rd=x%02ld rs1=x%02ld rs2=x%02ld\n", rd, rs1, rs2);
                            store_integer_register(rd, load_integer_register(rs1) + load_integer_register(rs2));
                            break;
                        case 0x01: //mul
//                            std::printf("mul: rd=x%02ld rs1=x%02ld rs2=x%02ld\n", rd, rs1, rs2);
                            store_integer_register(rd, load_integer_register(rs1) * load_integer_register(rs2));
                            break;
                        case 0x20: //sub
//                            std::printf("sub: rd=x%02ld rs1=x%02ld rs2=x%02ld\n", rd, rs1, rs2);
                            store_integer_register(rd, load_integer_register(rs1) - load_integer_register(rs2));
                            break;
                        default:
                            std::printf("ERROR : Not Implemented Yet : funct7=%lX\n", funct7);
                            return -1;
                    }
                    break;
                case 0x1: //sll
//                    std::printf("sll: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                    store_integer_register(rd, (load_integer_register(rs1) << shamt)
                                               | (load_integer_register(rs1) >> (64 - shamt)));
                    break;
                case 0x2: //slt
//                    std::printf("slt: rd=x%02ld rs2=x%02ld rs1=x%02ld\n", rd, rs2, rs1);
                    store_integer_register(rd,
                                           (int64_t) load_integer_register(rs1) < (int64_t) load_integer_register(rs2)
                                           ? 1 : 0);
                    break;
                case 0x3: //sltu
//                    std::printf("sltu: rd=x%02ld rs2=x%02ld rs1=x%02ld\n", rd, rs2, rs1);
                    store_integer_register(rd, load_integer_register(rs1) < load_integer_register(rs2) ? 1 : 0);
                    break;
                case 0x4: //xor
//                    std::printf("xor: rd=x%02ld rs2=x%02ld rs1=x%02ld\n", rd, rs2, rs1);
                    store_integer_register(rd, load_integer_register(rs1) ^ load_integer_register(rs2));
                    break;
                case 0x5: //rotate
                    switch (funct7) {
                        case 0x00: //srl
//                            std::printf("srl: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                            store_integer_register(rd, (load_integer_register(rs1) >> shamt)
                                                       | (load_integer_register(rs1) << (64 - shamt)));
                            break;
                        case 0x20: //sra
//                            std::printf("sra: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                            store_integer_register(rd, ((int64_t) load_integer_register(rs1) >> shamt)
                                                       | ((int64_t) load_integer_register(rs1) << (64 - shamt)));
                            break;
                        default:
                            std::printf("ERROR : Rotate Not Implemented Yet : funct7=%lX\n", funct7);
                            return -1;
                    }
                    break;
                case 0x6: //or
//                    std::printf("or: rd=x%02ld rs2=x%02ld rs1=x%02ld\n", rd, rs2, rs1);
                    store_integer_register(rd, load_integer_register(rs1) | load_integer_register(rs2));
                    break;
                case 0x7: //and
//                    std::printf("and: rd=x%02ld rs2=x%02ld rs1=x%02ld\n", rd, rs2, rs1);
                    store_integer_register(rd, load_integer_register(rs1) & load_integer_register(rs2));
                    break;
                default:
                    std::printf("ERROR : Not Implemented Yet : funct3=%lX\n", funct3);
                    return -1;
            }
            break;
        case 0x37: //lui
            imm = (int64_t) ((int32_t) (instruction & 0xFFFFF000));
//            std::printf("lui: rd=x%02ld imm=0x%08lX (%ld)\n", rd, imm, imm);
            store_integer_register(rd, imm);
            break;
        case 0x3b:
            shamt = (uint32_t) (load_integer_register(rs2) & 0x1f);
            switch (funct3) {
                case 0x0:
                    switch (funct7) {
                        case 0x00: //addw
//                            std::printf("addw: rd=x%02ld rs1=x%02ld rs2=x%02ld\n", rd, rs1, rs2);
                            store_integer_register(rd, (int64_t) ((int32_t) (load_integer_register(rs1) +
                                                                             load_integer_register(rs2))));
                            break;
                        case 0x20: //subw
//                            std::printf("sub: rd=x%02ld rs1=x%02ld rs2=x%02ld\n", rd, rs1, rs2);
                            store_integer_register(rd, (int64_t) ((int32_t) (load_integer_register(rs1) -
                                                                             load_integer_register(rs2))));
                            break;
                        default:
                            std::printf("ERROR : Not Implemented Yet : funct7=%lX\n", funct7);
                            return -1;
                    }
                    break;
                case 0x1: //sllw
//                    std::printf("sllw: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                    store_integer_register(rd, (int32_t) (((uint32_t) load_integer_register(rs1) << shamt)
                                                          | ((uint32_t) load_integer_register(rs1) >> (32 - shamt))));
                    break;
                case 0x5:
                    switch (funct7) {
                        case 0x00: //srlw
//                            std::printf("srlw: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                            store_integer_register(rd, (int32_t) (((uint32_t) load_integer_register(rs1) >> shamt)
                                                                  | ((uint32_t) load_integer_register(rs1)
                                    << (32 - shamt))));
                            break;
                        case 0x20: //sraw
//                            std::printf("sraw: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                            store_integer_register(rd, ((int32_t) load_integer_register(rs1) >> (int32_t) shamt)
                                                       |
                                                       ((int32_t) load_integer_register(rs1) << (32 - (int32_t) shamt)));
                            break;
                        default:
                            std::printf("ERROR : Not Implemented Yet : funct7=%lX\n", funct7);
                            return -1;
                    }
                    break;
                default:
                    std::printf("ERROR : Not Implemented Yet : funct3=%lX\n", funct3);
                    return -1;
            }
            break;
        case 0x63:
            imm = (uint64_t)((int64_t)((int32_t)(instruction & 0x80000000)) >> 19)
                    | ((instruction & 0x80) << 4)
                    | ((instruction >> 20) & 0x7e0)
                    | ((instruction >> 7) & 0x1e);
            switch (funct3) {
                case 0x0: //beq
//                    std::printf("beq: imm=0x%08lX (%ld) rs2=x%02ld rs1=x%02ld\n", imm, imm, rs2, rs1);
                    if (load_integer_register(rs1) == load_integer_register(rs2)){
                        m_pc += imm - 4;
                    }
                    break;
                case 0x1: //bne
//                    std::printf("bne: imm=0x%08lX (%ld) rs2=x%02ld rs1=x%02ld\n", imm, imm, rs2, rs1);
                    if (load_integer_register(rs1) != load_integer_register(rs2)){
                        m_pc += imm - 4;
                    }
                    break;
                case 0x4: //blt
//                    std::printf("blt: imm=0x%08lX (%ld) rs2=x%02ld rs1=x%02ld\n", imm, imm, rs2, rs1);
                    if ((int64_t) load_integer_register(rs1) < (int64_t) load_integer_register(rs2)){
                        m_pc += imm - 4;
                    }
                    break;
                case 0x5: //bge
//                    std::printf("bge: imm=0x%08lX (%ld) rs2=x%02ld rs1=x%02ld\n", imm, imm, rs2, rs1);
                    if ((int64_t) load_integer_register(rs1) >= (int64_t) load_integer_register(rs2)){
                        m_pc += imm - 4;
                    }
                    break;
                case 0x6: //bltu
//                    std::printf("bltu: imm=0x%08lX (%ld) rs2=x%02ld rs1=x%02ld\n", imm, imm, rs2, rs1);
                    if (load_integer_register(rs1) < load_integer_register(rs2)){
                        m_pc += imm - 4;
                    }
                    break;
                case 0x7: //bgeu
//                    std::printf("bgeu: imm=0x%08lX (%ld) rs2=x%02ld rs1=x%02ld\n", imm, imm, rs2, rs1);
                    if (load_integer_register(rs1) >= load_integer_register(rs2)){
                        m_pc += imm - 4;
                    }
                    break;
                default:
                    std::printf("ERROR : Not Implemented Yet : funct3=%lX\n", funct3);
                    return -1;
            }
            break;
        case 0x67: //jalr
            temp = m_pc;
            imm = (int64_t)((int32_t)(instruction & 0xFFF00000)) >> 20;
//            std::printf("jalr: rd=x%02ld imm=0x%08lX (%ld) rs1=x%02ld\n", rd, imm, imm, rs1);
            m_pc = (load_integer_register(rs1) + imm) & 0xFFFFFFFE;
            store_integer_register(rd, temp);
            break;
        case 0x6f: //jal
            store_integer_register(rd, m_pc);
            imm = (uint64_t)((int64_t)((int32_t)(instruction & 0x80000000)) >> 11)
                  | (instruction & 0xFF000)
                  | ((instruction >> 9) & 0x800)
                  | ((instruction >> 20) & 0x7FE);
//            std::printf("jal: imm=0x%08lX (%ld)\n", imm, imm);
            m_pc += imm - 4;
            break;
        case 0x73:
            csr_addr = (instruction & 0xfff00000) >> 20; //csr address
            switch (funct3) {
                case 0x0:
                    if (funct7 == 0x9){ //sfence.vma
                        // do nothing
                    } else if (rs2 == 0x2) {
                        if (funct7 == 0x8) { //sret
                            m_pc = load_csr(SEPC);
                            mode = ((load_csr(SSTATUS) >> 8) & 1) == 1 ? Mode::Supervisor : Mode::User;
                            store_csr(SSTATUS, ((load_csr(SSTATUS) >> 5) & 1) == 1 ? load_csr(SSTATUS) | 2 : load_csr(SSTATUS) & 0xFFFFFFFFFFFFFFFD);
                            store_csr(SSTATUS, load_csr(SSTATUS) | 32);
                            store_csr(SSTATUS, load_csr(SSTATUS) & 0xFFFFFFFFFFFFFEFF);
                        } else if (funct7 == 0x18) { //mret
                            m_pc = load_csr(MEPC);
                            switch ((load_csr(MSTATUS)>>11) & 0b11) {
                                case 2:
                                    mode = Mode::Machine;
                                    break;
                                case 1:
                                    mode = Mode::Supervisor;
                                    break;
                                default:
                                    mode = Mode::User;
                                    break;
                            }
                            store_csr(MSTATUS, ((load_csr(MSTATUS) >> 7) &  1) == 1 ? load_csr(MSTATUS) | 8 : load_csr(MSTATUS) & 0xFFFFFFFFFFFFFFF7);
                        }
                    } else {
                        std::printf("ERROR: Instruction with opcode %04lX : Not Implemented\n", opcode);
                        return -1;
                    }
                    break;
                case 0x1: //csrrw
                    temp = load_csr(csr_addr);
                    store_csr(csr_addr, load_integer_register(rs1));
                    store_integer_register(rd, temp);
                    break;
                case 0x2: //csrrs
                    temp = load_csr(csr_addr);
                    store_csr(csr_addr, temp | load_integer_register(rs1));
                    store_integer_register(rd, temp);
                    break;
                case 0x3: //csrrc
                    temp = load_csr(csr_addr);
                    store_csr(csr_addr, temp & (0xFFFFFFFFFFFFFFFF ^ load_integer_register(rs1))); //NOTE: maybe wrong?
                    store_integer_register(rd, temp);
                    break;
                case 0x5: //csrrwi
                    store_integer_register(rd, load_csr(csr_addr));
                    store_csr(csr_addr, rs1);
                    break;
                case 0x6: //csrrsi
                    temp = load_csr(csr_addr);
                    store_csr(csr_addr, temp | rs1);
                    store_integer_register(rd, temp);
                    break;
                case 0x7: //csrrci
                    temp = load_csr(csr_addr);
                    store_csr(csr_addr, temp & (0xFFFFFFFFFFFFFFFF ^ rs1));
                    store_integer_register(rd, temp);
                    break;
                default:
                    std::printf("ERROR: Instruction with opcode %04lX : Not Implemented\n", opcode);
                    return -1;
            }
            break;
        default:
            std::printf("ERROR: Instruction with opcode %04lX : Not Implemented\n", opcode);
            return -1;
    }
    return 0;
}



