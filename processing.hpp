#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

#include "cmd_parser.hpp"
#include "memory.hpp"

// Aggregated emulation statistics for reporting.
struct EmulationResult {
    double total_hit_rate = 0.0;
    double instruction_hit_rate = 0.0;
    double data_hit_rate = 0.0;
};

// Runs an emulation session for a given RISC-V variant (with a specific cache).
// Templated to allow different cache replacement policies.
template <typename RiscVVariant>
class EmulationRunner {
    static constexpr int kRaRegister = 1;  // Return address register (x1).

public:
    EmulationRunner() : cpu_(&memory_) {}

    // Load registers and memory frames from an input binary file.
    void load_data(const InputConfig& input_config) {
        std::ifstream fin(input_config.file_name, std::ios::binary);
        if (!fin) {
            throw std::runtime_error("Failed to open input file: " + input_config.file_name);
        }
        load_registers(fin);
        // Termination point: return address from main function.
        end_of_execution_addr_ = cpu_.read_reg(kRaRegister);
        load_memory(fin);
    }

    // Run until either the CPU finishes or the PC reaches the termination address.
    EmulationResult run() {
        while (!cpu_.is_finished() && cpu_.get_pc() != end_of_execution_addr_) {
            cpu_.execute_next_instruction();
        }
        cpu_.sync_memory();
        return calculate_result();
    }

    // Write out final registers and a slice of memory to the output file.
    void write_output(const OutputConfig& output_config) const {
        std::ofstream fout(output_config.file_name, std::ios::binary);
        if (!fout) {
            throw std::runtime_error("Failed to open output file: " + output_config.file_name);
        }
        write_registers(fout);
        memory_.write_data_to_stream(fout, static_cast<uint32_t>(output_config.begin_address),
                                     static_cast<size_t>(output_config.size));
    }

private:
    void load_registers(std::istream& in) {
        uint32_t regs[32]{};
        in.read(reinterpret_cast<char*>(regs), sizeof(regs));
        cpu_.set_pc(regs[0]);
        for (int i = 1; i < 32; ++i) {
            cpu_.set_reg(i, regs[i]);
        }
    }

    void write_registers(std::ostream& out) const {
        uint32_t regs[32]{};
        regs[0] = cpu_.get_pc();
        for (int i = 1; i < 32; ++i) {
            regs[i] = cpu_.read_reg(i);
        }
        out.write(reinterpret_cast<const char*>(regs), sizeof(regs));
    }

    void load_memory(std::istream& in) {
        while (in.peek() != EOF) {
            uint32_t begin_addr{};
            uint32_t frame_size{};
            in.read(reinterpret_cast<char*>(&begin_addr), sizeof(begin_addr));
            in.read(reinterpret_cast<char*>(&frame_size), sizeof(frame_size));
            if (in) {
                memory_.read_bytes_from_stream(in, begin_addr, frame_size);
            }
        }
    }

    static double calculate_hit_rate(size_t hits, size_t total) {
        if (total == 0) {
            // When no queries occur, consider it as 100% by convention used here.
            return 100.0;
        }
        return static_cast<double>(hits) * 100.0 / static_cast<double>(total);
    }

    EmulationResult calculate_result() {
        const size_t total_hits = cpu_.get_data_hits() + cpu_.get_instruction_hits();
        return EmulationResult{
            .total_hit_rate = calculate_hit_rate(total_hits, cpu_.get_total_queries()),
            .instruction_hit_rate =
                calculate_hit_rate(cpu_.get_instruction_hits(), cpu_.get_instruction_queries()),
            .data_hit_rate = calculate_hit_rate(cpu_.get_data_hits(), cpu_.get_data_queries()),
        };
    }

private:
    uint32_t end_of_execution_addr_ = 0;
    Memory memory_;
    RiscVVariant cpu_;
};