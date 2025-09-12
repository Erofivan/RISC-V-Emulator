#ifndef CMD_PARSER_HPP_
#define CMD_PARSER_HPP_

#include <optional>
#include <string>
#include <string_view>

struct InputConfig {
    std::string file_name;
};

struct OutputConfig {
    std::string file_name;
    int begin_address = 0;
    int size = 0;
};

struct CommandLineArgs {
    bool is_valid = false;
    std::string error_message;
    std::optional<InputConfig> input_config;
    std::optional<OutputConfig> output_config;
};

CommandLineArgs parse_command_line(int argc, char** argv);

#endif  // CMD_PARSER_HPP_