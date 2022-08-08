/*
 * Copyright (C) 2022 Nikolas Koesling <nikolas@koesling.info>.
 * This program is free software. You can redistribute it and/or modify it under the terms of the MIT License.
 */

#pragma once

#include "InputParser.hpp"

#include "InputParser_float.hpp"

#include <cxxendian.hpp>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace InputParser {

/* =====================================================================================================================
 * =====================================================================================================================
 * FLOAT 32 Bit (float)
 * =====================================================================================================================
 * =====================================================================================================================
 */

/**
 * @brief convert string to float
 *
 * @details
 * can also handle the following constants:
 *  - nan       not a number
 *  - inf       positive infinity
 *  - -inf      negative infinity
 *  - min       smallest positive value possible
 *  - max       maximum value
 *  - epsilon   machine epsilon: difference between 1.0 and the next possible representable value
 *  - lowest    lowest possible value
 *
 * @param value string value to convert
 * @return float value
 */
static float parse_float(const std::string &value) {
    static_assert(std::numeric_limits<float>::is_iec559, "IEEE 754 required");

    typedef float float_t;

    if (value == "nan") {
        return std::numeric_limits<float_t>::quiet_NaN();
    } else if (value == "inf") {
        return std::numeric_limits<float_t>::infinity();
    } else if (value == "-inf") {
        return -std::numeric_limits<float_t>::infinity();
    } else if (value == "min") {
        std::numeric_limits<float_t>::min();
    } else if (value == "max") {
        std::numeric_limits<float_t>::max();
    } else if (value == "epsilon") {
        std::numeric_limits<float_t>::epsilon();
    } else if (value == "lowest") {
        std::numeric_limits<float_t>::lowest();
    }

    float       ret;
    bool        fail = false;
    std::size_t idx  = 0;

    try {
        ret = std::stof(value, &idx);
    } catch (const std::exception &) { fail = true; }
    fail = fail || idx != value.size();

    if (fail) throw std::invalid_argument("Failed to parse value '" + value + '\'');

    return ret;
}

/**
 * @brief get Instructions for a big endian float
 * @param type register type
 * @param addr start address
 * @param value string to convert
 * @return list of instructions
 */

static std::vector<Instruction> parse_f32abcd(
        Instruction::register_type_t type, std::size_t addr, const std::string &value, int, bool verbose) {
    cxxendian::Host_Float<float> hf(parse_float(value));
    cxxendian::BE_Float<float>   bf(hf);

    union {
        uint16_t reg[2];
        float    f;
    };

    f = bf.get();

    if (verbose) {
        std::cerr << std::setprecision(std::numeric_limits<float>::digits10);
        std::cerr << "# big endian float 32: " << hf << std::endl;
    }

    return {Instruction(type, addr, reg[0]), Instruction(type, addr + 1, reg[1])};
}

/**
 * @brief get Instructions for a big endian float (reversed register order)
 * @param type register type
 * @param addr start address
 * @param value string to convert
 * @return list of instructions
 */
static std::vector<Instruction> parse_f32cdab(
        Instruction::register_type_t type, std::size_t addr, const std::string &value, int, bool verbose) {
    cxxendian::Host_Float<float> hf(parse_float(value));
    cxxendian::BE_Float<float>   bf(hf);

    union {
        uint16_t reg[2];
        float    f;
    };

    f = bf.get();

    if (verbose) {
        std::cerr << std::setprecision(std::numeric_limits<float>::digits10);
        std::cerr << "# big endian float 32 (reversed register order): " << hf << std::endl;
    }

    return {Instruction(type, addr, reg[1]), Instruction(type, addr + 1, reg[0])};
}

/**
 * @brief get Instructions for a little endian float
 * @param type register type
 * @param addr start address
 * @param value string to convert
 * @return list of instructions
 */
static std::vector<Instruction> parse_f32dcba(
        Instruction::register_type_t type, std::size_t addr, const std::string &value, int, bool verbose) {
    cxxendian::Host_Float<float> hf(parse_float(value));
    cxxendian::LE_Float<float>   lf(hf);

    union {
        uint16_t reg[2];
        float    f;
    };

    f = lf.get();

    if (verbose) {
        std::cerr << std::setprecision(std::numeric_limits<float>::digits10);
        std::cerr << "# little endian float 32: " << hf << std::endl;
    }

    return {Instruction(type, addr, reg[0]), Instruction(type, addr + 1, reg[1])};
}

/**
 * @brief get Instructions for a little endian float (reversed register order)
 * @param type register type
 * @param addr start address
 * @param value string to convert
 * @return list of instructions
 */
static std::vector<Instruction> parse_f32badc(
        Instruction::register_type_t type, std::size_t addr, const std::string &value, int, bool verbose) {
    cxxendian::Host_Float<float> hf(parse_float(value));
    cxxendian::LE_Float<float>   lf(hf);

    union {
        uint16_t reg[2];
        float    f;
    };

    f = lf.get();

    if (verbose) {
        std::cerr << std::setprecision(std::numeric_limits<float>::digits10);
        std::cerr << "# little endian floating point 32 bit (reversed register order): " << hf << std::endl;
    }

    return {Instruction(type, addr, reg[1]), Instruction(type, addr + 1, reg[0])};
}

/* =====================================================================================================================
 * =====================================================================================================================
 * FLOAT 64 Bit (double)
 * =====================================================================================================================
 * =====================================================================================================================
 */

/**
 * @brief convert string to double
 *
 * @details
 * can also handle the following constants:
 *  - nan       not a number
 *  - inf       positive infinity
 *  - -inf      negative infinity
 *  - min       smallest positive value possible
 *  - max       maximum value
 *  - epsilon   machine epsilon: difference between 1.0 and the next possible representable value
 *  - lowest    lowest possible value
 *
 * @param value string value to convert
 * @return double value
 */
static double parse_double(const std::string &value) {
    static_assert(std::numeric_limits<float>::is_iec559, "IEEE 754 required");

    typedef double float_t;

    if (value == "nan") {
        return std::numeric_limits<float_t>::quiet_NaN();
    } else if (value == "inf") {
        return std::numeric_limits<float_t>::infinity();
    } else if (value == "-inf") {
        return -std::numeric_limits<float_t>::infinity();
    } else if (value == "min") {
        std::numeric_limits<float_t>::min();
    } else if (value == "max") {
        std::numeric_limits<float_t>::max();
    } else if (value == "epsilon") {
        std::numeric_limits<float_t>::epsilon();
    } else if (value == "lowest") {
        std::numeric_limits<float_t>::lowest();
    }

    double      ret;
    bool        fail = false;
    std::size_t idx  = 0;

    try {
        ret = std::stod(value, &idx);
    } catch (const std::exception &) { fail = true; }
    fail = fail || idx != value.size();

    if (fail) throw std::invalid_argument("Failed to parse value '" + value + '\'');

    return ret;
}

/**
 * @brief get Instructions for a big endian double
 * @param type register type
 * @param addr start address
 * @param value string to convert
 * @return list of instructions
 */
static std::vector<Instruction> parse_f64abcdefgh(
        Instruction::register_type_t type, std::size_t addr, const std::string &value, int, bool verbose) {
    cxxendian::Host_Float<double> hd(parse_double(value));
    cxxendian::BE_Float<double>   bd(hd);

    union {
        uint16_t reg[4];
        double   d;
    };

    d = bd.get();

    if (verbose) {
        std::cerr << std::setprecision(std::numeric_limits<double>::digits10);
        std::cerr << "# big endian float 64: " << hd << std::endl;
    }

    return {Instruction(type, addr, reg[0]),
            Instruction(type, addr + 1, reg[1]),
            Instruction(type, addr + 2, reg[2]),
            Instruction(type, addr + 3, reg[3])};
}

/**
 * @brief get Instructions for a little endian double
 * @param type register type
 * @param addr start address
 * @param value string to convert
 * @return list of instructions
 */
static std::vector<Instruction> parse_f64hgfedcba(
        Instruction::register_type_t type, std::size_t addr, const std::string &value, int, bool verbose) {
    cxxendian::Host_Float<double> hd(parse_double(value));
    cxxendian::LE_Float<double>   ld(hd);

    union {
        uint16_t reg[4];
        double   d;
    };

    d = ld.get();

    if (verbose) {
        std::cerr << std::setprecision(std::numeric_limits<double>::digits10);
        std::cerr << "# little endian float 64: " << hd << std::endl;
    }

    return {Instruction(type, addr, reg[0]),
            Instruction(type, addr + 1, reg[1]),
            Instruction(type, addr + 2, reg[2]),
            Instruction(type, addr + 3, reg[3])};
}

/**
 * @brief get Instructions for a big endian double (reversed register order)
 * @param type register type
 * @param addr start address
 * @param value string to convert
 * @return list of instructions
 */
static std::vector<Instruction> parse_f64ghefcdab(
        Instruction::register_type_t type, std::size_t addr, const std::string &value, int, bool verbose) {
    cxxendian::Host_Float<double> hd(parse_double(value));
    cxxendian::BE_Float<double>   bd(hd);

    union {
        uint16_t reg[4];
        double   d;
    };

    d = bd.get();

    if (verbose) {
        std::cerr << std::setprecision(std::numeric_limits<double>::digits10);
        std::cerr << "# big endian float 64 (reversed register order): " << hd << std::endl;
    }

    return {Instruction(type, addr, reg[3]),
            Instruction(type, addr + 1, reg[2]),
            Instruction(type, addr + 2, reg[1]),
            Instruction(type, addr + 3, reg[0])};
}

/**
 * @brief get Instructions for a little endian double (reversed register order)
 * @param type register type
 * @param addr start address
 * @param value string to convert
 * @return list of instructions
 */
static std::vector<Instruction> parse_f64badcfehg(
        Instruction::register_type_t type, std::size_t addr, const std::string &value, int, bool verbose) {
    cxxendian::Host_Float<double> hd(parse_double(value));
    cxxendian::LE_Float<double>   ld(hd);

    union {
        uint16_t reg[4];
        double   d;
    };

    d = ld.get();

    if (verbose) {
        std::cerr << std::setprecision(std::numeric_limits<double>::digits10);
        std::cerr << "# big endian float 64 (reversed register order): " << hd << std::endl;
    }

    return {Instruction(type, addr, reg[3]),
            Instruction(type, addr + 1, reg[2]),
            Instruction(type, addr + 2, reg[1]),
            Instruction(type, addr + 3, reg[0])};
}


}  // namespace InputParser
