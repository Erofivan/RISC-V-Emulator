#ifndef EMULATOR_HPP_
#define EMULATOR_HPP_

#include <array>
#include <cstdint>
#include <iostream>
#include <utility>

#include "functions.hpp"
#include "instrunctions_types.hpp"
#include "system_parameters.hpp"

// Helper macros for defining instruction masks
#define R_TYPE_MASK(func7, func3, opcode) #func7 "*****" #func3 "*****" #opcode
#define I_TYPE_MASK(func3, opcode) "***********" #func3 "*****" #opcode

template <typename CacheType>
class RiscV {
public:
    explicit RiscV(Memory* memory_ptr) : cache_{memory_ptr} {}

    uint32_t read_reg(int addr) const noexcept { return (addr != 0) ? registers_[addr] : 0; }

    void set_reg(int address, uint32_t value) noexcept {
        if (address != 0) {
            registers_[address] = value;
        }
    }

    uint32_t get_pc() const noexcept { return pc_; }
    void set_pc(uint32_t value) noexcept { pc_ = value; }

    void execute_next_instruction() {
        if (is_finished_) {
            return;
        }

        const uint32_t current_instruction = read_from_cache(get_pc(), true);

        for (const auto& [handler, mask] : kInstructionSet) {
            if (satisfy_mask(current_instruction, mask)) {
                set_pc(handler(this, current_instruction));
                return;
            }
        }

        std::cerr << "Error: Unknown instruction at PC=0x" << std::hex << get_pc()
                  << " (Instruction: 0x" << current_instruction << ")" << std::dec << std::endl;
        is_finished_ = true;
    }

    void sync_memory() { cache_.sync_with_memory(); }
    bool is_finished() const noexcept { return is_finished_; }

    // Statistics
    size_t get_total_queries() const noexcept { return total_cache_queries_count_; }
    size_t get_instruction_queries() const noexcept { return instruction_queries_count_; }
    size_t get_data_queries() const noexcept { return data_queries_count_; }
    size_t get_instruction_hits() const noexcept { return instruction_hit_count_; }
    size_t get_data_hits() const noexcept { return data_hit_count_; }

private:
    // Memory access methods that go through the cache
    uint32_t read_from_cache(uint32_t addr, bool is_instruction = false) {
        uint32_t value = cache_.read_word(addr);
        update_hit_counters(is_instruction);
        return value;
    }
    uint16_t read_half_from_cache(uint32_t addr) {
        uint16_t value = cache_.read_half_word(addr);
        update_hit_counters(false);
        return value;
    }
    std::byte read_byte_from_cache(uint32_t addr) {
        std::byte value = cache_.read_byte(addr);
        update_hit_counters(false);
        return value;
    }
    void write_word_to_cache(uint32_t addr, uint32_t value) {
        cache_.write_word(addr, value);
        update_hit_counters(false);
    }
    void write_half_to_cache(uint32_t addr, uint16_t value) {
        cache_.write_half_word(addr, value);
        update_hit_counters(false);
    }
    void write_byte_to_cache(uint32_t addr, std::byte byte) {
        cache_.write_byte(addr, byte);
        update_hit_counters(false);
    }

    void update_hit_counters(bool is_instruction) {
        total_cache_queries_count_++;
        if (is_instruction) {
            instruction_queries_count_++;
            if (!cache_.last_op_was_miss()) {
                instruction_hit_count_++;
            }
        } else {
            data_queries_count_++;
            if (!cache_.last_op_was_miss()) {
                data_hit_count_++;
            }
        }
    }

    // Forward declarations for instruction handlers
#define DECLARE_INSTRUCTION(name) static uint32_t name(RiscV* cpu, uint32_t instr)

    DECLARE_INSTRUCTION(lui);
    DECLARE_INSTRUCTION(auipc);
    DECLARE_INSTRUCTION(jal);
    DECLARE_INSTRUCTION(jalr);
    DECLARE_INSTRUCTION(beq);
    DECLARE_INSTRUCTION(bne);
    DECLARE_INSTRUCTION(blt);
    DECLARE_INSTRUCTION(bge);
    DECLARE_INSTRUCTION(bltu);
    DECLARE_INSTRUCTION(bgeu);
    DECLARE_INSTRUCTION(lb);
    DECLARE_INSTRUCTION(lh);
    DECLARE_INSTRUCTION(lw);
    DECLARE_INSTRUCTION(lbu);
    DECLARE_INSTRUCTION(lhu);
    DECLARE_INSTRUCTION(sb);
    DECLARE_INSTRUCTION(sh);
    DECLARE_INSTRUCTION(sw);
    DECLARE_INSTRUCTION(addi);
    DECLARE_INSTRUCTION(slti);
    DECLARE_INSTRUCTION(sltiu);
    DECLARE_INSTRUCTION(xori);
    DECLARE_INSTRUCTION(ori);
    DECLARE_INSTRUCTION(andi);
    DECLARE_INSTRUCTION(slli);
    DECLARE_INSTRUCTION(srli);
    DECLARE_INSTRUCTION(srai);
    DECLARE_INSTRUCTION(add);
    DECLARE_INSTRUCTION(sub);
    DECLARE_INSTRUCTION(sll);
    DECLARE_INSTRUCTION(slt);
    DECLARE_INSTRUCTION(sltu);
    DECLARE_INSTRUCTION(xor_op);
    DECLARE_INSTRUCTION(srl);
    DECLARE_INSTRUCTION(sra);
    DECLARE_INSTRUCTION(or_op);
    DECLARE_INSTRUCTION(and_op);
    DECLARE_INSTRUCTION(fence);
    DECLARE_INSTRUCTION(ecall);
    DECLARE_INSTRUCTION(ebreak);
    DECLARE_INSTRUCTION(mul);
    DECLARE_INSTRUCTION(mulh);
    DECLARE_INSTRUCTION(mulhsu);
    DECLARE_INSTRUCTION(mulhu);
    DECLARE_INSTRUCTION(div_op);
    DECLARE_INSTRUCTION(divu);
    DECLARE_INSTRUCTION(rem);
    DECLARE_INSTRUCTION(remu);

#undef DECLARE_INSTRUCTION

    using InstructionHandler = uint32_t (*)(RiscV*, uint32_t);
    using OpMaskPair = std::pair<InstructionHandler, const char*>;

    static const std::array<OpMaskPair, 47> kInstructionSet;

    uint32_t pc_ = 0;
    uint32_t registers_[32] = {};
    CacheType cache_;
    bool is_finished_ = false;

    // Statistics counters
    size_t total_cache_queries_count_ = 0;
    size_t instruction_queries_count_ = 0;
    size_t data_queries_count_ = 0;
    size_t instruction_hit_count_ = 0;
    size_t data_hit_count_ = 0;
};

// --- Instruction Implementations ---
template <typename CacheType>
uint32_t RiscV<CacheType>::lui(RiscV* cpu, uint32_t instr) {
    UType data{instr};
    cpu->set_reg(static_cast<int>(data.rd), data.imm);
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}

template <typename CacheType>
uint32_t RiscV<CacheType>::auipc(RiscV* cpu, uint32_t instr) {
    UType data{instr};
    cpu->set_reg(static_cast<int>(data.rd), cpu->get_pc() + data.imm);
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}

template <typename CacheType>
uint32_t RiscV<CacheType>::jal(RiscV* cpu, uint32_t instr) {
    JType data{instr};
    cpu->set_reg(static_cast<int>(data.rd), cpu->get_pc() + system_parameters::INSTRUCTION_LEN);
    return cpu->get_pc() + sign_extend32(data.imm, 21);
}

template <typename CacheType>
uint32_t RiscV<CacheType>::jalr(RiscV* cpu, uint32_t instr) {
    IType data{instr};
    uint32_t next_pc = cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
    uint32_t target_pc =
        (cpu->read_reg(static_cast<int>(data.rs1)) + sign_extend32(data.imm, 12)) & ~1U;
    cpu->set_reg(static_cast<int>(data.rd), next_pc);
    return target_pc;
}

// Branch instructions
#define BRANCH_IMPL(name, condition)                          \
    template <typename CacheType>                             \
    uint32_t RiscV<CacheType>::name(RiscV* cpu, uint32_t instr) { \
        BType data{instr};                                    \
        if (condition) {                                      \
            return cpu->get_pc() + sign_extend32(data.imm, 13); \
        }                                                     \
        return cpu->get_pc() + system_parameters::INSTRUCTION_LEN; \
    }

BRANCH_IMPL(beq, cpu->read_reg(static_cast<int>(data.rs1)) ==
                     cpu->read_reg(static_cast<int>(data.rs2)))
BRANCH_IMPL(bne, cpu->read_reg(static_cast<int>(data.rs1)) !=
                     cpu->read_reg(static_cast<int>(data.rs2)))
BRANCH_IMPL(blt, static_cast<int32_t>(cpu->read_reg(static_cast<int>(data.rs1))) <
                     static_cast<int32_t>(cpu->read_reg(static_cast<int>(data.rs2))))
BRANCH_IMPL(bge, static_cast<int32_t>(cpu->read_reg(static_cast<int>(data.rs1))) >=
                     static_cast<int32_t>(cpu->read_reg(static_cast<int>(data.rs2))))
BRANCH_IMPL(bltu, cpu->read_reg(static_cast<int>(data.rs1)) <
                      cpu->read_reg(static_cast<int>(data.rs2)))
BRANCH_IMPL(bgeu, cpu->read_reg(static_cast<int>(data.rs1)) >=
                      cpu->read_reg(static_cast<int>(data.rs2)))
#undef BRANCH_IMPL

// Load instructions
template <typename CacheType>
uint32_t RiscV<CacheType>::lb(RiscV* cpu, uint32_t instr) {
    IType data{instr};
    uint32_t addr = cpu->read_reg(static_cast<int>(data.rs1)) + sign_extend32(data.imm, 12);
    uint32_t val = static_cast<uint32_t>(cpu->read_byte_from_cache(addr));
    cpu->set_reg(static_cast<int>(data.rd), sign_extend32(val, 8));
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}
template <typename CacheType>
uint32_t RiscV<CacheType>::lh(RiscV* cpu, uint32_t instr) {
    IType data{instr};
    uint32_t addr = cpu->read_reg(static_cast<int>(data.rs1)) + sign_extend32(data.imm, 12);
    uint32_t val = cpu->read_half_from_cache(addr);
    cpu->set_reg(static_cast<int>(data.rd), sign_extend32(val, 16));
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}
template <typename CacheType>
uint32_t RiscV<CacheType>::lw(RiscV* cpu, uint32_t instr) {
    IType data{instr};
    uint32_t addr = cpu->read_reg(static_cast<int>(data.rs1)) + sign_extend32(data.imm, 12);
    cpu->set_reg(static_cast<int>(data.rd), cpu->read_from_cache(addr));
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}
template <typename CacheType>
uint32_t RiscV<CacheType>::lbu(RiscV* cpu, uint32_t instr) {
    IType data{instr};
    uint32_t addr = cpu->read_reg(static_cast<int>(data.rs1)) + sign_extend32(data.imm, 12);
    cpu->set_reg(static_cast<int>(data.rd),
                 static_cast<uint32_t>(cpu->read_byte_from_cache(addr)));
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}
template <typename CacheType>
uint32_t RiscV<CacheType>::lhu(RiscV* cpu, uint32_t instr) {
    IType data{instr};
    uint32_t addr = cpu->read_reg(static_cast<int>(data.rs1)) + sign_extend32(data.imm, 12);
    cpu->set_reg(static_cast<int>(data.rd), cpu->read_half_from_cache(addr));
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}

// Store instructions
template <typename CacheType>
uint32_t RiscV<CacheType>::sb(RiscV* cpu, uint32_t instr) {
    SType data{instr};
    uint32_t addr = cpu->read_reg(static_cast<int>(data.rs1)) + sign_extend32(data.imm, 12);
    cpu->write_byte_to_cache(
        addr, static_cast<std::byte>(cpu->read_reg(static_cast<int>(data.rs2))));
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}
template <typename CacheType>
uint32_t RiscV<CacheType>::sh(RiscV* cpu, uint32_t instr) {
    SType data{instr};
    uint32_t addr = cpu->read_reg(static_cast<int>(data.rs1)) + sign_extend32(data.imm, 12);
    cpu->write_half_to_cache(addr, static_cast<uint16_t>(cpu->read_reg(static_cast<int>(data.rs2))));
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}
template <typename CacheType>
uint32_t RiscV<CacheType>::sw(RiscV* cpu, uint32_t instr) {
    SType data{instr};
    uint32_t addr = cpu->read_reg(static_cast<int>(data.rs1)) + sign_extend32(data.imm, 12);
    cpu->write_word_to_cache(addr, cpu->read_reg(static_cast<int>(data.rs2)));
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}

// Immediate arithmetic/logic instructions
#define IMM_OP_IMPL(name, operation)                                \
    template <typename CacheType>                                    \
    uint32_t RiscV<CacheType>::name(RiscV* cpu, uint32_t instr) {    \
        IType data{instr};                                           \
        cpu->set_reg(static_cast<int>(data.rd), (operation));        \
        return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;   \
    }
IMM_OP_IMPL(addi, cpu->read_reg(static_cast<int>(data.rs1)) + sign_extend32(data.imm, 12))
IMM_OP_IMPL(slti, static_cast<int32_t>(cpu->read_reg(static_cast<int>(data.rs1))) <
                      static_cast<int32_t>(sign_extend32(data.imm, 12)))
IMM_OP_IMPL(sltiu, cpu->read_reg(static_cast<int>(data.rs1)) < sign_extend32(data.imm, 12))
IMM_OP_IMPL(xori, cpu->read_reg(static_cast<int>(data.rs1)) ^ sign_extend32(data.imm, 12))
IMM_OP_IMPL(ori, cpu->read_reg(static_cast<int>(data.rs1)) | sign_extend32(data.imm, 12))
IMM_OP_IMPL(andi, cpu->read_reg(static_cast<int>(data.rs1)) & sign_extend32(data.imm, 12))
#undef IMM_OP_IMPL

// Shift immediate instructions
template <typename CacheType>
uint32_t RiscV<CacheType>::slli(RiscV* cpu, uint32_t instr) {
    IType data{instr};
    uint32_t shamt = static_cast<uint32_t>(get_bits_shifted(data.imm, 0, 5));
    cpu->set_reg(static_cast<int>(data.rd), cpu->read_reg(static_cast<int>(data.rs1)) << shamt);
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}
template <typename CacheType>
uint32_t RiscV<CacheType>::srli(RiscV* cpu, uint32_t instr) {
    IType data{instr};
    uint32_t shamt = static_cast<uint32_t>(get_bits_shifted(data.imm, 0, 5));
    cpu->set_reg(static_cast<int>(data.rd), cpu->read_reg(static_cast<int>(data.rs1)) >> shamt);
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}
template <typename CacheType>
uint32_t RiscV<CacheType>::srai(RiscV* cpu, uint32_t instr) {
    IType data{instr};
    uint32_t shamt = static_cast<uint32_t>(get_bits_shifted(data.imm, 0, 5));
    cpu->set_reg(static_cast<int>(data.rd),
                 static_cast<uint32_t>(static_cast<int32_t>(
                                           cpu->read_reg(static_cast<int>(data.rs1))) >>
                                       shamt));
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}

// Register-register arithmetic/logic instructions
#define REG_OP_IMPL(name, operation)                                 \
    template <typename CacheType>                                    \
    uint32_t RiscV<CacheType>::name(RiscV* cpu, uint32_t instr) {    \
        RType data{instr};                                           \
        cpu->set_reg(static_cast<int>(data.rd), (operation));        \
        return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;   \
    }
REG_OP_IMPL(add, cpu->read_reg(static_cast<int>(data.rs1)) +
                   cpu->read_reg(static_cast<int>(data.rs2)))
REG_OP_IMPL(sub, cpu->read_reg(static_cast<int>(data.rs1)) -
                   cpu->read_reg(static_cast<int>(data.rs2)))
REG_OP_IMPL(sll, cpu->read_reg(static_cast<int>(data.rs1)) <<
                   (cpu->read_reg(static_cast<int>(data.rs2)) & 0x1F))
REG_OP_IMPL(slt, static_cast<int32_t>(cpu->read_reg(static_cast<int>(data.rs1))) <
                    static_cast<int32_t>(cpu->read_reg(static_cast<int>(data.rs2))))
REG_OP_IMPL(sltu, cpu->read_reg(static_cast<int>(data.rs1)) <
                    cpu->read_reg(static_cast<int>(data.rs2)))
REG_OP_IMPL(xor_op, cpu->read_reg(static_cast<int>(data.rs1)) ^
                       cpu->read_reg(static_cast<int>(data.rs2)))
REG_OP_IMPL(srl, cpu->read_reg(static_cast<int>(data.rs1)) >>
                   (cpu->read_reg(static_cast<int>(data.rs2)) & 0x1F))
REG_OP_IMPL(sra, static_cast<uint32_t>(static_cast<int32_t>(
                       cpu->read_reg(static_cast<int>(data.rs1))) >>
                                       (cpu->read_reg(static_cast<int>(data.rs2)) & 0x1F)))
REG_OP_IMPL(or_op, cpu->read_reg(static_cast<int>(data.rs1)) |
                      cpu->read_reg(static_cast<int>(data.rs2)))
REG_OP_IMPL(and_op, cpu->read_reg(static_cast<int>(data.rs1)) &
                       cpu->read_reg(static_cast<int>(data.rs2)))
#undef REG_OP_IMPL

// System instructions
template <typename CacheType>
uint32_t RiscV<CacheType>::fence(RiscV* cpu, uint32_t) {
    // FENCE is a no-op in this simple simulation.
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}
template <typename CacheType>
uint32_t RiscV<CacheType>::ecall(RiscV* cpu, uint32_t) {
    cpu->is_finished_ = true;
    return cpu->get_pc();  // Don't advance PC after ecall
}
template <typename CacheType>
uint32_t RiscV<CacheType>::ebreak(RiscV* cpu, uint32_t) {
    cpu->is_finished_ = true;
    return cpu->get_pc();
}

// M-extension instructions
template <typename CacheType>
uint32_t RiscV<CacheType>::mul(RiscV* cpu, uint32_t instr) {
    RType data{instr};
    cpu->set_reg(static_cast<int>(data.rd), cpu->read_reg(static_cast<int>(data.rs1)) *
                                              cpu->read_reg(static_cast<int>(data.rs2)));
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}
template <typename CacheType>
uint32_t RiscV<CacheType>::mulh(RiscV* cpu, uint32_t instr) {
    RType data{instr};
    int64_t val1 = static_cast<int32_t>(cpu->read_reg(static_cast<int>(data.rs1)));
    int64_t val2 = static_cast<int32_t>(cpu->read_reg(static_cast<int>(data.rs2)));
    cpu->set_reg(static_cast<int>(data.rd), static_cast<uint32_t>((val1 * val2) >> 32));
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}
template <typename CacheType>
uint32_t RiscV<CacheType>::mulhsu(RiscV* cpu, uint32_t instr) {
    RType data{instr};
    int64_t val1 = static_cast<int32_t>(cpu->read_reg(static_cast<int>(data.rs1)));
    uint64_t val2 = cpu->read_reg(static_cast<int>(data.rs2));
    __int128_t result = static_cast<__int128_t>(val1) * static_cast<__int128_t>(val2);
    cpu->set_reg(static_cast<int>(data.rd), static_cast<uint32_t>(result >> 64));
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}
template <typename CacheType>
uint32_t RiscV<CacheType>::mulhu(RiscV* cpu, uint32_t instr) {
    RType data{instr};
    uint64_t val1 = cpu->read_reg(static_cast<int>(data.rs1));
    uint64_t val2 = cpu->read_reg(static_cast<int>(data.rs2));
    __int128_t result = static_cast<__int128_t>(val1) * static_cast<__int128_t>(val2);
    cpu->set_reg(static_cast<int>(data.rd), static_cast<uint32_t>(result >> 64));
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}
template <typename CacheType>
uint32_t RiscV<CacheType>::div_op(RiscV* cpu, uint32_t instr) {
    RType data{instr};
    int32_t dividend = static_cast<int32_t>(cpu->read_reg(static_cast<int>(data.rs1)));
    int32_t divisor = static_cast<int32_t>(cpu->read_reg(static_cast<int>(data.rs2)));
    if (divisor == 0) cpu->set_reg(static_cast<int>(data.rd), static_cast<uint32_t>(-1));
    else if (dividend == INT32_MIN && divisor == -1) cpu->set_reg(static_cast<int>(data.rd), static_cast<uint32_t>(dividend));
    else cpu->set_reg(static_cast<int>(data.rd), static_cast<uint32_t>(dividend / divisor));
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}
template <typename CacheType>
uint32_t RiscV<CacheType>::divu(RiscV* cpu, uint32_t instr) {
    RType data{instr};
    uint32_t dividend = cpu->read_reg(static_cast<int>(data.rs1));
    uint32_t divisor = cpu->read_reg(static_cast<int>(data.rs2));
    if (divisor == 0) cpu->set_reg(static_cast<int>(data.rd), static_cast<uint32_t>(-1));
    else cpu->set_reg(static_cast<int>(data.rd), dividend / divisor);
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}
template <typename CacheType>
uint32_t RiscV<CacheType>::rem(RiscV* cpu, uint32_t instr) {
    RType data{instr};
    int32_t dividend = static_cast<int32_t>(cpu->read_reg(static_cast<int>(data.rs1)));
    int32_t divisor = static_cast<int32_t>(cpu->read_reg(static_cast<int>(data.rs2)));
    if (divisor == 0) cpu->set_reg(static_cast<int>(data.rd), static_cast<uint32_t>(dividend));
    else if (dividend == INT32_MIN && divisor == -1) cpu->set_reg(static_cast<int>(data.rd), 0);
    else cpu->set_reg(static_cast<int>(data.rd), static_cast<uint32_t>(dividend % divisor));
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}
template <typename CacheType>
uint32_t RiscV<CacheType>::remu(RiscV* cpu, uint32_t instr) {
    RType data{instr};
    uint32_t dividend = cpu->read_reg(static_cast<int>(data.rs1));
    uint32_t divisor = cpu->read_reg(static_cast<int>(data.rs2));
    if (divisor == 0) cpu->set_reg(static_cast<int>(data.rd), dividend);
    else cpu->set_reg(static_cast<int>(data.rd), dividend % divisor);
    return cpu->get_pc() + system_parameters::INSTRUCTION_LEN;
}

// Full instruction set with masks
template <typename CacheType>
const std::array<typename RiscV<CacheType>::OpMaskPair, 47> RiscV<CacheType>::kInstructionSet = {{
    {lui,    "?????????????????????????0110111"},
    {auipc,  "?????????????????????????0010111"},
    {jal,    "?????????????????????????1101111"},
    {jalr,   I_TYPE_MASK(000, 1100111)},
    {beq,    I_TYPE_MASK(000, 1100011)},
    {bne,    I_TYPE_MASK(001, 1100011)},
    {blt,    I_TYPE_MASK(100, 1100011)},
    {bge,    I_TYPE_MASK(101, 1100011)},
    {bltu,   I_TYPE_MASK(110, 1100011)},
    {bgeu,   I_TYPE_MASK(111, 1100011)},
    {lb,     I_TYPE_MASK(000, 0000011)},
    {lh,     I_TYPE_MASK(001, 0000011)},
    {lw,     I_TYPE_MASK(010, 0000011)},
    {lbu,    I_TYPE_MASK(100, 0000011)},
    {lhu,    I_TYPE_MASK(101, 0000011)},
    {sb,     I_TYPE_MASK(000, 0100011)},
    {sh,     I_TYPE_MASK(001, 0100011)},
    {sw,     I_TYPE_MASK(010, 0100011)},
    {addi,   I_TYPE_MASK(000, 0010011)},
    {slti,   I_TYPE_MASK(010, 0010011)},
    {sltiu,  I_TYPE_MASK(011, 0010011)},
    {xori,   I_TYPE_MASK(100, 0010011)},
    {ori,    I_TYPE_MASK(110, 0010011)},
    {andi,   I_TYPE_MASK(111, 0010011)},
    {slli,   R_TYPE_MASK(0000000, 001, 0010011)},
    {srli,   R_TYPE_MASK(0000000, 101, 0010011)},
    {srai,   R_TYPE_MASK(0100000, 101, 0010011)},
    {add,    R_TYPE_MASK(0000000, 000, 0110011)},
    {sub,    R_TYPE_MASK(0100000, 000, 0110011)},
    {sll,    R_TYPE_MASK(0000000, 001, 0110011)},
    {slt,    R_TYPE_MASK(0000000, 010, 0110011)},
    {sltu,   R_TYPE_MASK(0000000, 011, 0110011)},
    {xor_op, R_TYPE_MASK(0000000, 100, 0110011)},
    {srl,    R_TYPE_MASK(0000000, 101, 0110011)},
    {sra,    R_TYPE_MASK(0100000, 101, 0110011)},
    {or_op,  R_TYPE_MASK(0000000, 110, 0110011)},
    {and_op, R_TYPE_MASK(0000000, 111, 0110011)},
    {fence,  "0000????????0000000000000001111"},
    {ecall,  "00000000000000000000000001110011"},
    {ebreak, "00000000000100000000000001110011"},
    {mul,    R_TYPE_MASK(0000001, 000, 0110011)},
    {mulh,   R_TYPE_MASK(0000001, 001, 0110011)},
    {mulhsu, R_TYPE_MASK(0000001, 010, 0110011)},
    {mulhu,  R_TYPE_MASK(0000001, 011, 0110011)},
    {div_op, R_TYPE_MASK(0000001, 100, 0110011)},
    {divu,   R_TYPE_MASK(0000001, 101, 0110011)},
    {rem,    R_TYPE_MASK(0000001, 110, 0110011)},
}};

#endif  // EMULATOR_HPP_