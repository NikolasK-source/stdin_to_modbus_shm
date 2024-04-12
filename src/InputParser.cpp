/*
 * Copyright (C) 2022 Nikolas Koesling <nikolas@koesling.info>.
 * This program is free software. You can redistribute it and/or modify it under the terms of the MIT License.
 */

#include "InputParser.hpp"

#include "InputParser_float.hpp"
#include "InputParser_int.hpp"
#include "split_string.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace InputParser {

typedef std::vector<Instruction> (*parse_function)(
        Instruction::register_type_t, std::size_t, const std::string &, int, bool);

/* Supported data types:
 *  - Float:
 *      - 32 Bit:
 *          - f32_abcd, f32_big, f32b                       32-Bit floating point in big endian
 *          - f32_dcba, f32_little, f32l                    32-Bit floating point in little endian
 *          - f32_cdab, f32_big_rev, f32br                  32-Bit floating point in big endian, registers reversed
 *          - f32_badc, f32_little_rev, f32lr               32-Bit floating point in little endian, registers reversed
 *      - 64 Bit:
 *          - f64_abcdefgh, f64_big, f64b                   64-Bit floating point in big endian
 *          - f64_ghefcdab, f64_little, f64l                64-Bit floating point in little endian
 *          - f64_badcfehg, f64_big_rev, f64br              64-Bit floating point in big endian, registers reversed
 *          - f64_hgfedcba, f64_little_rev, f64lr           64-Bit floating point in little endian, registers reversed
 *  - Int:
 *      - 8 Bit:
 *          - u8_lo                                         8-Bit unsigned integer written to low byte of register
 *          - u8_hi                                         8-Bit unsigned integer written to high byte of register
 *          - i8_lo                                         8-Bit signed integer written to low byte of register
 *          - i8_hi                                         8-Bit signed integer written to high byte of register
 *      - 16 Bit
 *          - u16_ab, u16_big, u16b                         16-Bit unsigned integer in big endian
 *          - i16_ab, i16_big, i16b                         16-Bit signed integer in big endian
 *          - u16_ba, u16_little, u16l                      16-Bit unsigned integer in little endian
 *          - i16_ba, i16_little, i16l                      16-Bit signed integer in little endian
 *      - 32 Bit:
 *          - u32_abcd, u32_big, u32b                       32-Bit unsigned integer in big endian
 *          - i32_abcd, i32_big, i32b                       32-Bit signed integer in big endian
 *          - u32_dcba, u32_little, u32l                    32-Bit unsigned integer in little endian
 *          - i32_dcba, i32_little, i32l                    32-Bit signed integer in little endian
 *          - u32_cdab, u32_big_rev, u32br                  32-Bit unsigned integer in big endian, registers reversed
 *          - i32_cdab, i32_big_rev, i32br                  32-Bit signed integer in big endian, registers reversed
 *          - u32_badc, u32_little_rev, u32lr               32-Bit unsigned integer in little endian, registers reversed
 *          - i32_badc, i32_little_rev, i32lr               32-Bit signed integer in little endian, registers reversed
 *      - 64 Bit:
 *          - u64_abcdefgh, u64_big, u64b                   64-Bit unsigned integer in big endian
 *          - i64_abcdefgh, i64_big, i64b                   64-Bit signed integer in big endian
 *          - u64_hgfedcba, u64_little, u64l                64-Bit unsigned integer in little endian
 *          - i64_hgfedcba, i64_little, i64l                64-Bit signed integer in little endian
 *          - u64_ghefcdab, u64_big_rev, u64br              64-Bit unsigned integer in big endian, registers reversed
 *          - i64_ghefcdab, i64_big_rev, i64br              64-Bit signed integer in big endian, registers reversed
 *          - u64_badcfehg, u64_little_rev, u64lr           64-Bit unsigned integer in little endian, registers reversed
 *          - i64_badcfehg, i64_little_rev, i64lr           64-Bit signed integer in little endian, registers reversed
 *
 */
static const std::unordered_map<std::string, parse_function> PARSE_FUNCTIONS = {
        // float
        {"f32_abcd", parse_f32abcd},
        {"f32_cdab", parse_f32cdab},
        {"f32_badc", parse_f32badc},
        {"f32_dcba", parse_f32dcba},
        {"f32_little", parse_f32dcba},
        {"f32l", parse_f32dcba},
        {"f32_big", parse_f32abcd},
        {"f32b", parse_f32abcd},
        {"f32_little_rev", parse_f32badc},
        {"f32lr", parse_f32badc},
        {"f32_big_rev", parse_f32cdab},
        {"f32br", parse_f32cdab},

        // double
        {"f64_abcdefgh", parse_f64abcdefgh},
        {"f64_ghefcdab", parse_f64ghefcdab},
        {"f64_badcfehg", parse_f64badcfehg},
        {"f64_hgfedcba", parse_f64hgfedcba},
        {"f64_little", parse_f64hgfedcba},
        {"f64l", parse_f64hgfedcba},
        {"f64_big", parse_f64abcdefgh},
        {"f64b", parse_f64abcdefgh},
        {"f64_little_rev", parse_f64badcfehg},
        {"f64lr", parse_f64badcfehg},
        {"f64_big_rev", parse_f64ghefcdab},
        {"f64br", parse_f64ghefcdab},

        // 8 bit integer
        {"u8_lo", parse_u8_lo},
        {"u8_hi", parse_u8_hi},
        {"i8_lo", parse_i8_lo},
        {"i8_hi", parse_i8_hi},

        // 16 bit integer
        {"u16_ab", parse_u16_ab},
        {"u16_big", parse_u16_ab},
        {"u16b", parse_u16_ab},
        {"u16_ba", parse_u16_ba},
        {"u16_little", parse_u16_ba},
        {"u16l", parse_u16_ba},
        {"i16_ab", parse_i16_ab},
        {"i16_big", parse_i16_ab},
        {"i16b", parse_i16_ab},
        {"i16_ba", parse_i16_ba},
        {"i16_little", parse_i16_ba},
        {"i16l", parse_i16_ba},

        // 32 bit integer
        {"u32_abcd", parse_u32abcd},
        {"u32_cdab", parse_u32cdab},
        {"u32_badc", parse_u32badc},
        {"u32_dcba", parse_u32dcba},
        {"u32_little", parse_u32dcba},
        {"u32l", parse_u32dcba},
        {"u32_big", parse_u32abcd},
        {"u32b", parse_u32abcd},
        {"u32_little_rev", parse_u32badc},
        {"u32lr", parse_u32badc},
        {"u32_big_rev", parse_u32cdab},
        {"u32br", parse_u32cdab},
        {"i32_abcd", parse_i32abcd},
        {"i32_cdab", parse_i32cdab},
        {"i32_badc", parse_i32badc},
        {"i32_dcba", parse_i32dcba},
        {"i32_little", parse_i32dcba},
        {"i32l", parse_i32dcba},
        {"i32_big", parse_i32abcd},
        {"i32b", parse_i32abcd},
        {"i32_little_rev", parse_i32badc},
        {"i32lr", parse_i32badc},
        {"i32_big_rev", parse_i32cdab},
        {"i32br", parse_i32cdab},

        // 64 bit integer
        {"u64_abcdefgh", parse_u64abcdefgh},
        {"u64_ghefcdab", parse_u64ghefcdab},
        {"u64_badcfehg", parse_u64badcfehg},
        {"u64_hgfedcba", parse_u64hgfedcba},
        {"u64_little", parse_u64hgfedcba},
        {"u64l", parse_u64hgfedcba},
        {"u64_big", parse_u64abcdefgh},
        {"u64b", parse_u64abcdefgh},
        {"u64_little_rev", parse_u64badcfehg},
        {"u64lr", parse_u64badcfehg},
        {"u64_big_rev", parse_u64ghefcdab},
        {"u64br", parse_u64ghefcdab},
        {"i64_abcdefgh", parse_i64abcdefgh},
        {"i64_ghefcdab", parse_i64ghefcdab},
        {"i64_badcfehg", parse_i64badcfehg},
        {"i64_hgfedcba", parse_i64hgfedcba},
        {"i64_little", parse_i64hgfedcba},
        {"i64l", parse_i64hgfedcba},
        {"i64_big", parse_i64abcdefgh},
        {"i64b", parse_i64abcdefgh},
        {"i64_little_rev", parse_i64badcfehg},
        {"i64lr", parse_i64badcfehg},
        {"i64_big_rev", parse_i64ghefcdab},
        {"i64br", parse_i64ghefcdab},
};

std::vector<Instruction> parse(std::string line, int base_addr, int base_value, bool verbose) {
    static constexpr std::size_t MIN_ELEMENTS = 3;
    static constexpr std::size_t MAX_ELEMENTS = 4;
    static constexpr char        DELIMITER    = ':';

    // to lower input string
    std::transform(line.begin(), line.end(), line.begin(), [](char c) { return std::tolower(c); });

    // compatibility to modbus_conv_float (refactor lines that start with f:)
    if (line.size() >= 2 && line[0] == 'f' && line[1] == ':') { line = line.substr(2) + ':' + "f32_badc"; }

    // split string
    auto split_input = split_string(line, DELIMITER);

    // check number of elements
    if (split_input.size() > MAX_ELEMENTS || split_input.size() < MIN_ELEMENTS) {
        throw std::invalid_argument("The input does not contain the appropriate number of delimiters");
    }

    // convert value expressions
    if (split_input[2] == "true" || split_input[2] == "one" || split_input[2] == "high" || split_input[2] == "active" ||
        split_input[2] == "on" || split_input[2] == "enabled") {
        split_input[2] = "1";
    } else if (split_input[2] == "false" || split_input[2] == "zero" || split_input[2] == "low" ||
               split_input[2] == "inactive" || split_input[2] == "off" || split_input[2] == "disabled") {
        split_input[2] = "0";
    } else if (split_input[2] == "pi") {
        split_input[2] = PI;
    } else if (split_input[2] == "npi" || split_input[2] == "-pi") {
        split_input[2] = NPI;
    } else if (split_input[2] == "sqrt2") {
        split_input[2] = SQRT2;
    } else if (split_input[2] == "sqrt3") {
        split_input[2] = SQRT3;
    } else if (split_input[2] == "phi") {
        split_input[2] = PHI;
    } else if (split_input[2] == "ln2") {
        split_input[2] = LN2;
    } else if (split_input[2] == "e") {
        split_input[2] = E;
    }

    // get register type
    Instruction::register_type_t type {};
    auto                        &type_str = split_input.at(0);
    for (auto &c : type_str)
        c = static_cast<char>(std::tolower(c));
    if (type_str == "do") {
        type = Instruction::register_type_t::DO;
    } else if (type_str == "di") {
        type = Instruction::register_type_t::DI;
    } else if (type_str == "ao") {
        type = Instruction::register_type_t::AO;
    } else if (type_str == "ai") {
        type = Instruction::register_type_t::AI;
    } else {
        throw std::invalid_argument('\'' + type_str + "' is not a valid register type");
    }

    // get address
    std::size_t addr;
    const auto &addr_str = split_input.at(1);
    bool        fail     = false;
    std::size_t idx      = 0;
    try {
        addr = std::stoull(addr_str, &idx, base_addr);
    } catch (const std::exception &) { fail = true; }
    fail = fail || idx != addr_str.size();

    if (fail) throw std::invalid_argument("Failed to parse address '" + addr_str + '\'');

    const auto &value_str = split_input.at(2);

    if (split_input.size() == MIN_ELEMENTS) {  // input does not specify a data type --> write single register
        // get value
        std::size_t value;
        try {
            value = std::stoull(value_str, &idx, base_value);
        } catch (const std::exception &) { fail = true; }
        fail = fail || idx != value_str.size();

        if (fail) throw std::invalid_argument("Failed to parse value '" + value_str + '\'');

        return {Instruction(type, addr, static_cast<uint16_t>(value))};

    } else {
        // check register type
        switch (type) {
            case Instruction::register_type_t::DO:
            case Instruction::register_type_t::DI:
                throw std::invalid_argument("Data type specification for coils is not allowed");
            case Instruction::register_type_t::AO:
            case Instruction::register_type_t::AI:
                // do noting
                break;
        }

        if (PARSE_FUNCTIONS.find(split_input.at(3)) == PARSE_FUNCTIONS.end()) {
            throw std::invalid_argument("Unknown data type '" + split_input[3] + '\'');
        }

        return PARSE_FUNCTIONS.at(split_input[3])(type, addr, value_str, base_value, verbose);
    }
}

}  // namespace InputParser
