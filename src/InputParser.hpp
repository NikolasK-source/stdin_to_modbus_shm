/*
 * Copyright (C) 2022 Nikolas Koesling <nikolas@koesling.info>.
 * This program is free software. You can redistribute it and/or modify it under the terms of the MIT License.
 */

#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace InputParser {

static constexpr const char *PI    = "3.14159265358979323846";
static constexpr const char *NPI   = "-3.14159265358979323846";
static constexpr const char *SQRT2 = "1.41421356237309504880";
static constexpr const char *SQRT3 = "1.73205080756887729352";
static constexpr const char *PHI   = "1.61803398874989484820";
static constexpr const char *LN2   = "0.69314718055994530941";
static constexpr const char *E     = "2.71828182845904523536";

/**
 * @brief modbus write instruction
 */
struct Instruction {
    /**
     * @brief lists of all possible register types
     */
    enum class register_type_t { DO, DI, AO, AI };
    register_type_t register_type;  //*< register type
    std::size_t     address;        //*< register address
    uint16_t        value;          //*< register value (will be converted to bool for DO and DI register type)

    /**
     * @brief initialize all values
     * @param register_type register type
     * @param address register address
     * @param value register value
     */
    Instruction(register_type_t register_type, std::size_t address, uint16_t value)
        : register_type(register_type), address(address), value(value) {}
};

/**
 * @brief convert instruction line to list of modbus write instructions
 * @param line input instruction line
 * @param base_addr numerical base for converting addresses
 * @param base_value numerical base for converting values
 * @return list of instructions
 */
std::vector<Instruction> parse(std::string line, int base_addr = 0, int base_value = 0, bool verbose = false);

}  // namespace InputParser
