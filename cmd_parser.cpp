#include "cmd_parser.hpp"

#include <charconv>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace {

// Tries to convert a string_view to an integer.
// Returns std::nullopt if the conversion fails.
std::optional<int> string_to_int(std::string_view s, int base = 10) {
    int value{};
    const auto* const last = s.data() + s.size();
    auto [ptr, ec] = std::from_chars(s.data(), last, value, base);
    if (ec == std::errc() && ptr == last) {
        return value;
    }
    return std::nullopt;
}

}  // namespace

CommandLineArgs parse_command_line(int argc, char** argv) {
    CommandLineArgs args;
    if (argc <= 1) {
        args.is_valid = false;
        args.error_message = "No arguments provided.";
        return args;
    }

    for (int i = 1; i < argc; ++i) {
        const std::string_view current_arg = argv[i];

        if (current_arg == "-i") {
            if (i + 1 >= argc) {
                args.is_valid = false;
                args.error_message = "Missing filename after -i.";
                return args;
            }
            const std::string_view input_file = argv[i + 1];
            if (!std::filesystem::exists(input_file)) {
                args.is_valid = false;
                args.error_message = "Unable to locate input file: " + std::string(input_file);
                return args;
            }
            args.input_config = InputConfig{std::string(input_file)};
            i += 1;  // Consume the filename argument
        } else if (current_arg == "-o") {
            if (i + 3 >= argc) {
                args.is_valid = false;
                args.error_message =
                    "-o option requires exactly 3 arguments: <file> <addr> <size>.";
                return args;
            }

            const std::string_view output_file = argv[i + 1];
            const auto addr = string_to_int(argv[i + 2]);
            const auto size = string_to_int(argv[i + 3]);

            if (!addr || !size) {
                args.is_valid = false;
                args.error_message = "Invalid address or size for -o option.";
                return args;
            }

            args.output_config = OutputConfig{
                .file_name = std::string(output_file),
                .begin_address = *addr,
                .size = *size,
            };
            i += 3;  // Consume the 3 arguments for -o
        } else {
            args.is_valid = false;
            args.error_message = "Unknown argument: " + std::string(current_arg);
            return args;
        }
    }

    if (!args.input_config) {
        args.is_valid = false;
        args.error_message = "Input file must be specified with -i.";
        return args;
    }

    args.is_valid = true;
    return args;
}