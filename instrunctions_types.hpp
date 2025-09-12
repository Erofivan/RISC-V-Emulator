#ifndef INSTRUNCTIONS_TYPES_HPP_
#define INSTRUNCTIONS_TYPES_HPP_

#include <cstdint>

#include "functions.hpp"

// Decoders for different RISC-V instruction formats.

struct RType {
    uint32_t rd{}, rs1{}, rs2{};
    explicit RType(uint32_t instruction) {
        rd = static_cast<uint32_t>(get_bits_shifted(instruction, 7, 12));
        rs1 = static_cast<uint32_t>(get_bits_shifted(instruction, 15, 20));
        rs2 = static_cast<uint32_t>(get_bits_shifted(instruction, 20, 25));
    }
};

struct IType {
    uint32_t rd{}, rs1{};
    uint32_t imm{};
    explicit IType(uint32_t instruction) {
        rd = static_cast<uint32_t>(get_bits_shifted(instruction, 7, 12));
        rs1 = static_cast<uint32_t>(get_bits_shifted(instruction, 15, 20));
        imm = static_cast<uint32_t>(get_bits_shifted(instruction, 20, 32));
    }
};

struct SType {
    uint32_t rs1{}, rs2{};
    uint32_t imm{};
    explicit SType(uint32_t instruction) {
        rs1 = static_cast<uint32_t>(get_bits_shifted(instruction, 15, 20));
        rs2 = static_cast<uint32_t>(get_bits_shifted(instruction, 20, 25));
        imm = static_cast<uint32_t>((get_bits_shifted(instruction, 25, 32) << 5) |
                                    get_bits_shifted(instruction, 7, 12));
    }
};

struct BType {
    uint32_t imm{};
    uint32_t rs1{}, rs2{};
    explicit BType(uint32_t instruction) {
        rs1 = static_cast<uint32_t>(get_bits_shifted(instruction, 15, 20));
        rs2 = static_cast<uint32_t>(get_bits_shifted(instruction, 20, 25));
        // Immediate value for B-type is scrambled.
        imm = static_cast<uint32_t>((get_bit(instruction, 31) << 12) |
                                    (get_bit(instruction, 7) << 11) |
                                    (get_bits_shifted(instruction, 25, 31) << 5) |
                                    (get_bits_shifted(instruction, 8, 12) << 1));
    }
};

struct UType {
    uint32_t rd{};
    uint32_t imm{};
    explicit UType(uint32_t instruction) {
        rd = static_cast<uint32_t>(get_bits_shifted(instruction, 7, 12));
        imm = static_cast<uint32_t>(get_bits(instruction, 12, 32));
    }
};

struct JType {
    uint32_t imm{};
    uint32_t rd{};
    explicit JType(uint32_t instruction) {
        rd = static_cast<uint32_t>(get_bits_shifted(instruction, 7, 12));
        // Immediate value for J-type is scrambled.
        imm = static_cast<uint32_t>((get_bit(instruction, 31) << 20) |
                                    (get_bits_shifted(instruction, 12, 20) << 1) |
                                    (get_bit(instruction, 20) << 11) |
                                    (get_bits_shifted(instruction, 21, 31) << 12));
    }
};

#endif  // INSTRUNCTIONS_TYPES_HPP_