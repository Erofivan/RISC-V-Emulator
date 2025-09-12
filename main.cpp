#include <format>
#include <iostream>
#include <optional>
#include <string>

#include "bit_lru_policy.hpp"
#include "cache.hpp"
#include "lru_policy.hpp"
#include "cmd_parser.hpp"
#include "address.hpp"
#include "memory.hpp"
#include "emulator.hpp"
#include "processing.hpp"

namespace {

void print_result_line(const std::string& policy_name,
                       const std::optional<EmulationResult>& result) {
    if (result) {
        // The output format must be exactly this to match requirements.
        std::cout << std::format("{:>11}\t{:.5f}%%\t{:.5f}%%\t{:.5f}%%\n",
                                 policy_name, result->total_hit_rate,
                                 result->instruction_hit_rate, result->data_hit_rate);
    } else {
        std::cout << std::format(
            "{:>11}\tunsupported\tunsupported\tunsupported\n", policy_name);
    }
}

void print_emulation_summary(
    const std::optional<EmulationResult>& lru_result,
    const std::optional<EmulationResult>& bit_lru_result) {
    std::cout << "replacement\thit rate\thit rate (inst)\thit rate (data)\n";
    print_result_line("LRU", lru_result);
    print_result_line("bpLRU", bit_lru_result);
}

}  // namespace

int main(int argc, char* argv[]) {
    const auto args = parse_command_line(argc, argv);
    if (!args.is_valid) {
        if (!args.error_message.empty()) {
            std::cerr << "Error: " << args.error_message << std::endl;
        }
        std::cerr << "Usage: " << argv[0]
                  << " -i <input_file> [-o <output_file> <addr> <size>]\n";
        return EXIT_FAILURE;
    }

    try {
        // Define the CPU types with different cache policies
        using LruCpu = RiscV<Cache<LruPolicy>>;
        using BpLruCpu = RiscV<Cache<BitLruPolicy>>;

        EmulationRunner<LruCpu> lru_emulator;
        EmulationRunner<BpLruCpu> bp_lru_emulator;

        // Load the same initial state into both emulators
        lru_emulator.load_data(*args.input_config);
        bp_lru_emulator.load_data(*args.input_config);

        // Run simulations
        const auto lru_result = lru_emulator.run();
        const auto bp_lru_result = bp_lru_emulator.run();

        print_emulation_summary(lru_result, bp_lru_result);

        // If output is requested, write the final state of the LRU simulation
        if (args.output_config) {
            lru_emulator.write_output(*args.output_config);
        }
    } catch (const std::exception& e) {
        std::cerr << "An exception occurred: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}