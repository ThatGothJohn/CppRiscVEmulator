//
// Created by John on 15/12/2022.
//

#include "cpu.h"

CPU::CPU() {
    m_pc = DRAM_BASE;
    m_integer_registers = new std::uint64_t[32]();
    csrs = new std::uint64_t[4096]();
    m_floating_point_registers = new std::uint64_t[32]();

    bus = Bus();

    //setup x2 with the size of the memory when the cpu is instantiated
    m_integer_registers[2] = DRAM_BASE+MEMORY_SIZE;
}

CPU::CPU(uint8_t *binary, uint64_t binary_size) {
    m_pc = DRAM_BASE;
    m_integer_registers = new std::uint64_t[32]();
    csrs = new std::uint64_t[4096]();

    m_floating_point_registers = new std::uint64_t[32]();

    bus = Bus(binary, binary_size);

    //setup x2 with the size of the memory when the cpu is instantiated
    m_integer_registers[2] = DRAM_BASE+MEMORY_SIZE;
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
    m_pc += 4;

    if (execute(current_instruction) != 0){ //return error on unknown instruction
        return -2;
    }
    if (m_pc == 0x0){ //hack to stop infinite loops
        return -1;
    }
    return 0;
}

std::uint64_t CPU::fetch() {
    return bus.load(m_pc, 32);
}

uint8_t CPU::execute(std::uint64_t instruction) {
    std::uint64_t opcode = instruction & 0x7f;
    std::uint64_t rd = (instruction >> 7) & 0x1f;
    std::uint64_t rs1 = (instruction >> 15) & 0x1f;
    std::uint64_t rs2 = (instruction >> 20) & 0x1f;
    std::uint64_t funct3 = (instruction >> 12) & 0x7;
    std::uint64_t funct7 = (instruction >> 25) & 0x7f;

    std::uint64_t imm, addr, shamt, temp;

    printf("Instruction %08lX : ", instruction);

    switch (opcode) {
        case 0x03: //Load Instructions
            imm = (int64_t) ((int32_t) (instruction & 0xfff00000)) >> 20;
            addr = read_integer_register(rs1) + imm;
            switch (funct3) {
                case 0x0: //lb
                    std::printf("lb: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    write_integer_register(rd, ((int64_t) ((int8_t) bus.load(addr, 8))));
                    break;
                case 0x1: //lh
                    std::printf("lh: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    write_integer_register(rd, ((int64_t) ((int16_t) bus.load(addr, 16))));
                    break;
                case 0x2: //lw
                    std::printf("lw: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    write_integer_register(rd, ((int64_t) ((int32_t) bus.load(addr, 32))));
                    break;
                case 0x3: //ld
                    std::printf("ld: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    write_integer_register(rd, bus.load(addr, 64));
                    break;
                case 0x4: //lbu
                    std::printf("lbu: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    write_integer_register(rd, bus.load(addr, 8));
                    break;
                case 0x5: //lhu
                    std::printf("lhu: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    write_integer_register(rd, bus.load(addr, 16));
                    break;
                case 0x6: //lwu
                    std::printf("lwu: rd=x%02ld imm=0x%08lX (%ld) addr=0x%08ld\n", rd, imm, imm, addr);
                    write_integer_register(rd, bus.load(addr, 32));
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
                    std::printf("addi: rd=x%02ld imm=0x%08lX (%ld) rs1=x%02ld\n", rd, imm, imm, rs1);
                    write_integer_register(rd, read_integer_register(rs1) + imm);
                    break;
                case 0x1: //slli
                    std::printf("slli: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                    write_integer_register(rd, read_integer_register(rs1) << shamt);
                    break;
                case 0x2: //slti
                    std::printf("slti: rd=x%02ld imm=0x%08lX (%ld) rs1=x%02ld\n", rd, imm, imm, rs1);
                    write_integer_register(rd, (int64_t) read_integer_register(rs1) < (int64_t) imm ? 1 : 0);
                    break;
                case 0x3: //sltiu
                    std::printf("sltiu: rd=x%02ld imm=0x%08lX (%ld) rs1=x%02ld\n", rd, imm, imm, rs1);
                    write_integer_register(rd, read_integer_register(rs1) < imm ? 1 : 0);
                    break;
                case 0x4: //xori
                    std::printf("xori: rd=x%02ld imm=0x%08lX (%ld) rs1=x%02ld\n", rd, imm, imm, rs1);
                    write_integer_register(rd, read_integer_register(rs1) ^ imm);
                    break;
                case 0x5: //rotate
                    switch (funct7 >> 1) {
                        case 0x00: //srli
                            std::printf("srli: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                            write_integer_register(rd, (read_integer_register(rs1) >> shamt)
                                                       | (read_integer_register(rs1) << (64 - shamt)));
                            break;
                        case 0x10: //srai
                            std::printf("srai: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                            write_integer_register(rd, ((int64_t) read_integer_register(rs1) >> shamt)
                                                       | ((int64_t) read_integer_register(rs1) << (64 - shamt)));
                            break;
                        default:
                            std::printf("ERROR : Rotate Not Implemented Yet : funct7=%lX\n", funct7);
                            return -1;
                    }
                    break;
                case 0x6: //ori
                    std::printf("ori: rd=x%02ld imm=0x%08lX (%ld) rs1=x%02ld\n", rd, imm, imm, rs1);
                    write_integer_register(rd, read_integer_register(rs1) | imm);
                    break;
                case 0x7: //andi
                    std::printf("andi: rd=x%02ld imm=0x%08lX (%ld) rs1=x%02ld\n", rd, imm, imm, rs1);
                    write_integer_register(rd, read_integer_register(rs1) & imm);
                    break;
                default:
                    std::printf("ERROR : Not Implemented Yet : funct3=%lX\n", funct3);
                    return -1;
            }
            break;
        case 0x17: //auipc
            imm = (int64_t) ((int32_t) (instruction & 0xFFFFF000));
            std::printf("auipc: rd=x%02ld imm=0x%08lX (%ld)\n", rd, imm, imm);
            write_integer_register(rd, m_pc + imm - 4);
            break;
        case 0x1b:
            imm = (int64_t) ((int32_t) instruction) >> 20;
            shamt = (uint32_t) (imm & 0x1f);
            switch (funct3) {
                case 0x0: //addiw
                    std::printf("addiw: rd=x%02ld imm=0x%08lX (%ld) rs1=x%02ld\n", rd, imm, imm, rs1);
                    write_integer_register(rd, (int64_t)((int32_t)(read_integer_register(rs1) + imm)));
                    break;
                case 0x1: //slliw
                    std::printf("slliw: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                    write_integer_register(rd, (int64_t) ((int32_t) (((uint32_t) read_integer_register(rs1) << shamt)
                                                                     | ((uint32_t) read_integer_register(rs1)
                            >> (32 - shamt)))));
                    break;
                case 0x5:
                    switch (funct7) {
                        case 0x00: //srliw
                            std::printf("srliw: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                            write_integer_register(rd, (int64_t) ((int32_t) (
                                    ((uint32_t) read_integer_register(rs1) >> shamt)
                                    | ((uint32_t) read_integer_register(rs1) << (32 - shamt)))));
                            break;
                        case 0x20: //sraiw
                            std::printf("sraiw: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                            write_integer_register(rd,
                                                   (int64_t) ((int32_t) (((int32_t) read_integer_register(rs1) >> shamt)
                                                                         | ((int32_t) read_integer_register(rs1)
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
            addr = (read_integer_register(rs1) + imm) % 0xFFFFFFFF;
            switch (funct3) {
                case 0x0: //sb
                    std::printf("sb: rs2=x%02ld addr=0x%08ld\n", rs2, addr);
                    bus.store(addr, 8, read_integer_register(rs2));
                    break;
                case 0x1: //sh
                    std::printf("sh: rs2=x%02ld addr=0x%08ld\n", rs2, addr);
                    bus.store(addr, 16, read_integer_register(rs2));
                    break;
                case 0x2: //sw
                    std::printf("sw: rs2=x%02ld addr=0x%08ld\n", rs2, addr);
                    bus.store(addr, 32, read_integer_register(rs2));
                    break;
                case 0x3: //sd
                    std::printf("sd: rs2=x%02ld addr=0x%08ld\n", rs2, addr);
                    bus.store(addr, 64, read_integer_register(rs2));
                    break;
                default:
                    std::printf("ERROR : Invalid Store : funct3=%lX\n", funct3);
                    return -1;
            }
            break;
        case 0x33:
            shamt = (uint32_t) (read_integer_register(rs2) & 0x3f);
            switch (funct3) {
                case 0x0:
                    switch (funct7) {
                        case 0x00: //add
                            std::printf("add: rd=x%02ld rs1=x%02ld rs2=x%02ld\n", rd, rs1, rs2);
                            write_integer_register(rd, read_integer_register(rs1) + read_integer_register(rs2));
                            break;
                        case 0x01: //mul
                            std::printf("mul: rd=x%02ld rs1=x%02ld rs2=x%02ld\n", rd, rs1, rs2);
                            write_integer_register(rd, read_integer_register(rs1) * read_integer_register(rs2));
                            break;
                        case 0x20: //sub
                            std::printf("sub: rd=x%02ld rs1=x%02ld rs2=x%02ld\n", rd, rs1, rs2);
                            write_integer_register(rd, read_integer_register(rs1) - read_integer_register(rs2));
                            break;
                        default:
                            std::printf("ERROR : Not Implemented Yet : funct7=%lX\n", funct7);
                            return -1;
                    }
                    break;
                case 0x1: //sll
                    std::printf("sll: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                    write_integer_register(rd, (read_integer_register(rs1) << shamt)
                                               | (read_integer_register(rs1) >> (64 - shamt)));
                    break;
                case 0x2: //slt
                    std::printf("slt: rd=x%02ld rs2=x%02ld rs1=x%02ld\n", rd, rs2, rs1);
                    write_integer_register(rd,
                                           (int64_t) read_integer_register(rs1) < (int64_t) read_integer_register(rs2)
                                           ? 1 : 0);
                    break;
                case 0x3: //sltu
                    std::printf("sltu: rd=x%02ld rs2=x%02ld rs1=x%02ld\n", rd, rs2, rs1);
                    write_integer_register(rd, read_integer_register(rs1) < read_integer_register(rs2) ? 1 : 0);
                    break;
                case 0x4: //xor
                    std::printf("xor: rd=x%02ld rs2=x%02ld rs1=x%02ld\n", rd, rs2, rs1);
                    write_integer_register(rd, read_integer_register(rs1) ^ read_integer_register(rs2));
                    break;
                case 0x5: //rotate
                    switch (funct7) {
                        case 0x00: //srl
                            std::printf("srl: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                            write_integer_register(rd, (read_integer_register(rs1) >> shamt)
                                                       | (read_integer_register(rs1) << (64 - shamt)));
                            break;
                        case 0x20: //sra
                            std::printf("sra: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                            write_integer_register(rd, ((int64_t) read_integer_register(rs1) >> shamt)
                                                       | ((int64_t) read_integer_register(rs1) << (64 - shamt)));
                            break;
                        default:
                            std::printf("ERROR : Rotate Not Implemented Yet : funct7=%lX\n", funct7);
                            return -1;
                    }
                    break;
                case 0x6: //or
                    std::printf("or: rd=x%02ld rs2=x%02ld rs1=x%02ld\n", rd, rs2, rs1);
                    write_integer_register(rd, read_integer_register(rs1) | read_integer_register(rs2));
                    break;
                case 0x7: //and
                    std::printf("and: rd=x%02ld rs2=x%02ld rs1=x%02ld\n", rd, rs2, rs1);
                    write_integer_register(rd, read_integer_register(rs1) & read_integer_register(rs2));
                    break;
                default:
                    std::printf("ERROR : Not Implemented Yet : funct3=%lX\n", funct3);
                    return -1;
            }
            break;
        case 0x37: //lui
            imm = (int64_t) ((int32_t) (instruction & 0xFFFFF000));
            std::printf("lui: rd=x%02ld imm=0x%08lX (%ld)\n", rd, imm, imm);
            write_integer_register(rd, imm);
            break;
        case 0x3b:
            shamt = (uint32_t) (read_integer_register(rs2) & 0x1f);
            switch (funct3) {
                case 0x0:
                    switch (funct7) {
                        case 0x00: //addw
                            std::printf("addw: rd=x%02ld rs1=x%02ld rs2=x%02ld\n", rd, rs1, rs2);
                            write_integer_register(rd, (int64_t)((int32_t)(read_integer_register(rs1) + read_integer_register(rs2))));
                            break;
                        case 0x20: //subw
                            std::printf("sub: rd=x%02ld rs1=x%02ld rs2=x%02ld\n", rd, rs1, rs2);
                            write_integer_register(rd, (int64_t)((int32_t)(read_integer_register(rs1) - read_integer_register(rs2))));
                            break;
                        default:
                            std::printf("ERROR : Not Implemented Yet : funct7=%lX\n", funct7);
                            return -1;
                    }
                    break;
                case 0x1: //sllw
                    std::printf("sllw: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                    write_integer_register(rd, (int32_t)(((uint32_t)read_integer_register(rs1) << shamt)
                                               | ((uint32_t)read_integer_register(rs1) >> (32 - shamt))));
                    break;
                case 0x5:
                    switch (funct7) {
                        case 0x00: //srlw
                            std::printf("srlw: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                            write_integer_register(rd, (int32_t)(((uint32_t)read_integer_register(rs1) >> shamt)
                                                     | ((uint32_t)read_integer_register(rs1) << (32 - shamt))));
                            break;
                        case 0x20: //sraw
                            std::printf("sraw: rd=x%02ld shamt=0x%08lX (%ld) rs1=x%02ld\n", rd, shamt, shamt, rs1);
                            write_integer_register(rd, ((int32_t)read_integer_register(rs1) >> (int32_t)shamt)
                                                     | ((int32_t)read_integer_register(rs1) << (32 - (int32_t)shamt)));
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
                    std::printf("beq: imm=0x%08lX (%ld) rs2=x%02ld rs1=x%02ld\n", imm, imm, rs2, rs1);
                    if (read_integer_register(rs1) == read_integer_register(rs2)){
                        m_pc += imm - 4;
                    }
                    break;
                case 0x1: //bne
                    std::printf("bnq: imm=0x%08lX (%ld) rs2=x%02ld rs1=x%02ld\n", imm, imm, rs2, rs1);
                    if (read_integer_register(rs1) != read_integer_register(rs2)){
                        m_pc += imm - 4;
                    }
                    break;
                case 0x4: //blt
                    std::printf("blt: imm=0x%08lX (%ld) rs2=x%02ld rs1=x%02ld\n", imm, imm, rs2, rs1);
                    if ((int64_t)read_integer_register(rs1) < (int64_t)read_integer_register(rs2)){
                        m_pc += imm - 4;
                    }
                    break;
                case 0x5: //bge
                    std::printf("bge: imm=0x%08lX (%ld) rs2=x%02ld rs1=x%02ld\n", imm, imm, rs2, rs1);
                    if ((int64_t)read_integer_register(rs1) >= (int64_t)read_integer_register(rs2)){
                        m_pc += imm - 4;
                    }
                    break;
                case 0x6: //bltu
                    std::printf("bltu: imm=0x%08lX (%ld) rs2=x%02ld rs1=x%02ld\n", imm, imm, rs2, rs1);
                    if (read_integer_register(rs1) < read_integer_register(rs2)){
                        m_pc += imm - 4;
                    }
                    break;
                case 0x7: //bgeu
                    std::printf("bgeu: imm=0x%08lX (%ld) rs2=x%02ld rs1=x%02ld\n", imm, imm, rs2, rs1);
                    if (read_integer_register(rs1) >= read_integer_register(rs2)){
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
            std::printf("jalr: rd=x%02ld imm=0x%08lX (%ld) rs1=x%02ld\n", rd, imm, imm, rs1);
            m_pc = (read_integer_register(rs1) + imm) & 0xFFFFFFFE;
            write_integer_register(rd, temp);
            break;
        case 0x6f: //jal
            imm = (uint64_t)((int64_t)((int32_t)(instruction & 0x80000000)) >> 11)
                  | (instruction & 0xFF000)
                  | ((instruction >> 9) & 0x800)
                  | ((instruction >> 20) & 0x7FE);
            std::printf("jal: imm=0x%08lX (%ld)\n", imm, imm);
            m_pc += imm - 4;
            break;
        default:
            std::printf("ERROR: Instruction with opcode %04lX : Not Implemented\n", opcode);
            return -1;
    }
    return 0;
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
        std::sprintf(output, "%s\n%s", output,temp);
        delete[] temp;
    }
    std::printf("PC: %18lX", m_pc);
    std::printf("%s\n", output);
    delete[] output;
}

void CPU::loop() {
    while (cycle() == 0){
        dump_registers();
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



