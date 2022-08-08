/*
 * Copyright (C) 2022 Nikolas Koesling <nikolas@koesling.info>.
 * This program is free software. You can redistribute it and/or modify it under the terms of the MIT License.
 */

#pragma once

#include <cstdint>
#include <cxxendian.hpp>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>

static_assert(sizeof(unsigned long long) >= sizeof(uint64_t));
static_assert(sizeof(long long) >= sizeof(int64_t));

namespace InputParser {

template <class T, typename std::enable_if<std::is_integral<T> {} && !std::is_signed<T> {}, bool>::type = true>
static T parse_int(const std::string &value, int base) {
    if (value == "min") {
        return std::numeric_limits<T>::min();
    } else if (value == "max") {
        return std::numeric_limits<T>::max();
    } else if (value == "lowest") {
        return std::numeric_limits<T>::lowest();
    }

    unsigned long long ret;
    bool               fail = false;
    std::size_t        idx  = 0;

    try {
        ret = std::stoull(value, &idx, base);
    } catch (const std::exception &) { fail = true; }
    fail = fail || idx != value.size();
    fail = fail || ret > std::numeric_limits<T>::max();

    if (fail) throw std::invalid_argument("Failed to parse value '" + value + '\'');

    return static_cast<T>(ret);
}

template <class T, typename std::enable_if<std::is_integral<T> {} && std::is_signed<T> {}, bool>::type = true>
static T parse_int(const std::string &value, int base) {
    if (value == "min") {
        return std::numeric_limits<T>::min();
    } else if (value == "max") {
        return std::numeric_limits<T>::max();
    } else if (value == "lowest") {
        return std::numeric_limits<T>::lowest();
    }

    long long   ret;
    bool        fail = false;
    std::size_t idx  = 0;

    try {
        ret = std::stoll(value, &idx, base);
    } catch (const std::exception &) { fail = true; }
    fail = fail || idx != value.size();
    fail = fail || ret > std::numeric_limits<T>::max();
    fail = fail || ret < std::numeric_limits<T>::min();

    if (fail) throw std::invalid_argument("Failed to parse value '" + value + '\'');

    return static_cast<T>(ret);
}

/* =====================================================================================================================
 * =====================================================================================================================
 * 8 Bit
 * =====================================================================================================================
 * =====================================================================================================================
 */

static std::vector<Instruction>
        parse_u8_lo(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    union {
        uint16_t reg;
        uint8_t  i[2];
    };

    i[0] = parse_int<uint8_t>(value, base);
    i[1] = 0;

    if (verbose) {
        std::cerr << "# low byte unsigned integer 32 bit: " << i[0] << std::endl;
    }

    return {Instruction(type, addr, reg)};
}

static std::vector<Instruction>
        parse_u8_hi(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    union {
        uint16_t reg;
        uint8_t  i[2];
    };

    i[1] = parse_int<uint8_t>(value, base);
    i[0] = 0;

    if (verbose) {
        std::cerr << "# high byte unsigned integer 8 bit: " << i[1] << std::endl;
    }

    return {Instruction(type, addr, reg)};
}

// low byte
static std::vector<Instruction>
        parse_i8_lo(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    union {
        uint16_t reg;
        int8_t   i[2];
    };

    i[0] = parse_int<int8_t>(value, base);
    i[1] = 0;

    if (verbose) {
        std::cerr << "# low byte signed integer 8 bit: " << i[0] << std::endl;
    }

    return {Instruction(type, addr, reg)};
}

// high byte
static std::vector<Instruction>
        parse_i8_hi(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    union {
        uint16_t reg;
        int8_t   i[2];
    };

    i[1] = parse_int<int8_t>(value, base);
    i[0] = 0;

    if (verbose) {
        std::cerr << "# high byte signed integer 8 bit: " << i[1] << std::endl;
    }

    return {Instruction(type, addr, reg)};
}

/* =====================================================================================================================
 * =====================================================================================================================
 * 16 Bit
 * =====================================================================================================================
 * =====================================================================================================================
 */

// big endian
static std::vector<Instruction>
        parse_u16_ab(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    cxxendian::Host_Int<uint16_t> hi(parse_int<uint16_t>(value, base));
    cxxendian::BE_Int<uint16_t>   bi(hi);

    if (verbose) {
        std::cerr << "# big endian unsigned integer 16 bit: " << hi << std::endl;
    }

    return {Instruction(type, addr, bi.get())};
}

// little endian
static std::vector<Instruction>
        parse_u16_ba(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    cxxendian::Host_Int<uint16_t> hi(parse_int<uint16_t>(value, base));
    cxxendian::LE_Int<uint16_t>   li(hi);

    if (verbose) {
        std::cerr << "# little endian unsigned integer 16 bit: " << hi << std::endl;
    }

    return {Instruction(type, addr, li.get())};
}

// big endian
static std::vector<Instruction>
        parse_i16_ab(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    cxxendian::Host_Int<int16_t> hi(parse_int<int16_t>(value, base));
    cxxendian::BE_Int<int16_t>   bi(hi);

    union {
        uint16_t reg;
        int16_t  i;
    };

    i = bi.get();

    if (verbose) {
        std::cerr << "# big endian signed integer 16 bit: " << hi << std::endl;
    }

    if (verbose) {
        std::cerr << std::endl;
    }

    return {Instruction(type, addr, reg)};
}

// little endian
static std::vector<Instruction>
        parse_i16_ba(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    cxxendian::Host_Int<int16_t> hi(parse_int<int16_t>(value, base));
    cxxendian::LE_Int<int16_t>   li(hi);

    union {
        uint16_t reg;
        int16_t  i;
    };

    i = li.get();

    if (verbose) {
        std::cerr << "# little endian signed integer 16 bit: " << hi << std::endl;
    }

    return {Instruction(type, addr, reg)};
}

/* =====================================================================================================================
 * =====================================================================================================================
 * 32 Bit
 * =====================================================================================================================
 * =====================================================================================================================
 */

// big endian
static std::vector<Instruction>
        parse_u32abcd(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    typedef uint32_t           int_t;
    cxxendian::Host_Int<int_t> hi(parse_int<int_t>(value, base));
    cxxendian::BE_Int<int_t>   ei(hi);

    union {
        uint16_t reg[2];
        int_t    i;
    };

    i = ei.get();

    if (verbose) {
        std::cerr << "big endian unsigned integer " << sizeof(int_t) * 8 << " bit: " << hi << std::endl;
    }

    return {Instruction(type, addr, reg[0]), Instruction(type, addr + 1, reg[1])};
}

// little endian
static std::vector<Instruction>
        parse_u32dcba(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    typedef uint32_t           int_t;
    cxxendian::Host_Int<int_t> hi(parse_int<int_t>(value, base));
    cxxendian::LE_Int<int_t>   ei(hi);

    union {
        uint16_t reg[2];
        int_t    i;
    };

    i = ei.get();

    if (verbose) {
        std::cerr << "little endian unsigned integer " << sizeof(int_t) * 8 << " bit: " << hi << std::endl;
    }

    return {Instruction(type, addr, reg[0]), Instruction(type, addr + 1, reg[1])};
}

// big endian reversed
static std::vector<Instruction>
        parse_u32cdab(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    typedef uint32_t           int_t;
    cxxendian::Host_Int<int_t> hi(parse_int<int_t>(value, base));
    cxxendian::BE_Int<int_t>   ei(hi);

    union {
        uint16_t reg[2];
        int_t    i;
    };

    i = ei.get();

    if (verbose) {
        std::cerr << "big endian unsigned integer " << sizeof(int_t) * 8 << " bit (reversed register order): " << hi << std::endl;
    }

    return {Instruction(type, addr, reg[1]), Instruction(type, addr + 1, reg[0])};
}

// little endian reversed
static std::vector<Instruction>
        parse_u32badc(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    typedef uint32_t           int_t;
    cxxendian::Host_Int<int_t> hi(parse_int<int_t>(value, base));
    cxxendian::LE_Int<int_t>   ei(hi);

    union {
        uint16_t reg[2];
        int_t    i;
    };

    i = ei.get();

    if (verbose) {
        std::cerr << "little endian unsigned integer " << sizeof(int_t) * 8 << " bit (reversed register order): " << hi << std::endl;
    }

    return {Instruction(type, addr, reg[1]), Instruction(type, addr + 1, reg[0])};
}

// big endian
static std::vector<Instruction>
        parse_i32abcd(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    typedef int32_t            int_t;
    cxxendian::Host_Int<int_t> hi(parse_int<int_t>(value, base));
    cxxendian::BE_Int<int_t>   ei(hi);

    union {
        uint16_t reg[2];
        int_t    i;
    };

    i = ei.get();

    if (verbose) {
        std::cerr << "big endian signed integer " << sizeof(int_t) * 8 << " bit: " << hi << std::endl;
    }

    return {Instruction(type, addr, reg[0]), Instruction(type, addr + 1, reg[1])};
}

// little endian
static std::vector<Instruction>
        parse_i32dcba(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    typedef int32_t            int_t;
    cxxendian::Host_Int<int_t> hi(parse_int<int_t>(value, base));
    cxxendian::LE_Int<int_t>   ei(hi);

    union {
        uint16_t reg[2];
        int_t    i;
    };

    i = ei.get();

    if (verbose) {
        std::cerr << "little endian signed integer " << sizeof(int_t) * 8 << " bit: " << hi << std::endl;
    }

    return {Instruction(type, addr, reg[0]), Instruction(type, addr + 1, reg[1])};
}

// big endian reversed
static std::vector<Instruction>
        parse_i32cdab(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    typedef int32_t            int_t;
    cxxendian::Host_Int<int_t> hi(parse_int<int_t>(value, base));
    cxxendian::BE_Int<int_t>   ei(hi);

    union {
        uint16_t reg[2];
        int_t    i;
    };

    i = ei.get();

    if (verbose) {
        std::cerr << "big endian signed integer " << sizeof(int_t) * 8 << " bit (reversed register order): " << hi << std::endl;
    }

    return {Instruction(type, addr, reg[1]), Instruction(type, addr + 1, reg[0])};
}

// little endian reversed
static std::vector<Instruction>
        parse_i32badc(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    typedef int32_t            int_t;
    cxxendian::Host_Int<int_t> hi(parse_int<int_t>(value, base));
    cxxendian::LE_Int<int_t>   ei(hi);

    union {
        uint16_t reg[2];
        int_t    i;
    };

    i = ei.get();

    if (verbose) {
        std::cerr << "little endian signed integer " << sizeof(int_t) * 8 << " bit (reversed register order): " << hi << std::endl;
    }

    return {Instruction(type, addr, reg[1]), Instruction(type, addr + 1, reg[0])};
}

/* =====================================================================================================================
 * =====================================================================================================================
 * 64 Bit
 * =====================================================================================================================
 * =====================================================================================================================
 */

// big endian
static std::vector<Instruction>
        parse_u64abcdefgh(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    typedef uint64_t           int_t;
    cxxendian::Host_Int<int_t> hi(parse_int<int_t>(value, base));
    cxxendian::BE_Int<int_t>   ei(hi);

    union {
        uint16_t reg[4];
        int_t    i;
    };

    i = ei.get();

    if (verbose) {
        std::cerr << "big endian unsigned integer " << sizeof(int_t) * 8 << " bit: " << hi << std::endl;
    }

    return {
            Instruction(type, addr, reg[0]),
            Instruction(type, addr + 1, reg[1]),
            Instruction(type, addr + 2, reg[2]),
            Instruction(type, addr + 3, reg[3]),
    };
}

// little endian
static std::vector<Instruction>
        parse_u64hgfedcba(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    typedef uint64_t           int_t;
    cxxendian::Host_Int<int_t> hi(parse_int<int_t>(value, base));
    cxxendian::LE_Int<int_t>   ei(hi);

    union {
        uint16_t reg[4];
        int_t    i;
    };

    i = ei.get();

    if (verbose) {
        std::cerr << "little endian unsigned integer " << sizeof(int_t) * 8 << " bit: " << hi << std::endl;
    }

    return {
            Instruction(type, addr, reg[0]),
            Instruction(type, addr + 1, reg[1]),
            Instruction(type, addr + 2, reg[2]),
            Instruction(type, addr + 3, reg[3]),
    };
}

// big endian reversed
static std::vector<Instruction>
        parse_u64ghefcdab(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    typedef uint64_t           int_t;
    cxxendian::Host_Int<int_t> hi(parse_int<int_t>(value, base));
    cxxendian::BE_Int<int_t>   ei(hi);

    union {
        uint16_t reg[4];
        int_t    i;
    };

    i = ei.get();

    if (verbose) {
        std::cerr << "big endian unsigned integer " << sizeof(int_t) * 8 << " bit (reversed register order): " << hi << std::endl;
    }

    return {
            Instruction(type, addr, reg[3]),
            Instruction(type, addr + 1, reg[2]),
            Instruction(type, addr + 2, reg[1]),
            Instruction(type, addr + 3, reg[0]),
    };
}

// little endian reversed
static std::vector<Instruction>
        parse_u64badcfehg(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    typedef uint64_t           int_t;
    cxxendian::Host_Int<int_t> hi(parse_int<int_t>(value, base));
    cxxendian::LE_Int<int_t>   ei(hi);

    union {
        uint16_t reg[4];
        int_t    i;
    };

    i = ei.get();

    if (verbose) {
        std::cerr << "little endian unsigned integer " << sizeof(int_t) * 8 << " bit (reversed register order): " << hi << std::endl;
    }

    return {
            Instruction(type, addr, reg[3]),
            Instruction(type, addr + 1, reg[2]),
            Instruction(type, addr + 2, reg[1]),
            Instruction(type, addr + 3, reg[0]),
    };
}

// big endian
static std::vector<Instruction>
        parse_i64abcdefgh(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    typedef int64_t            int_t;
    cxxendian::Host_Int<int_t> hi(parse_int<int_t>(value, base));
    cxxendian::BE_Int<int_t>   ei(hi);

    union {
        uint16_t reg[4];
        int_t    i;
    };

    i = ei.get();

    if (verbose) {
        std::cerr << "big endian signed integer " << sizeof(int_t) * 8 << " bit: " << hi << std::endl;
    }

    return {
            Instruction(type, addr, reg[0]),
            Instruction(type, addr + 1, reg[1]),
            Instruction(type, addr + 2, reg[2]),
            Instruction(type, addr + 3, reg[3]),
    };
}

// little endian
static std::vector<Instruction>
        parse_i64hgfedcba(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    typedef int64_t            int_t;
    cxxendian::Host_Int<int_t> hi(parse_int<int_t>(value, base));
    cxxendian::LE_Int<int_t>   ei(hi);

    union {
        uint16_t reg[4];
        int_t    i;
    };

    i = ei.get();

    if (verbose) {
        std::cerr << "little endian signed integer " << sizeof(int_t) * 8 << " bit: " << hi << std::endl;
    }

    return {
            Instruction(type, addr, reg[0]),
            Instruction(type, addr + 1, reg[1]),
            Instruction(type, addr + 2, reg[2]),
            Instruction(type, addr + 3, reg[3]),
    };
}

// big endian reversed
static std::vector<Instruction>
        parse_i64ghefcdab(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    typedef int64_t            int_t;
    cxxendian::Host_Int<int_t> hi(parse_int<int_t>(value, base));
    cxxendian::BE_Int<int_t>   ei(hi);

    union {
        uint16_t reg[4];
        int_t    i;
    };

    i = ei.get();

    if (verbose) {
        std::cerr << "big endian signed integer " << sizeof(int_t) * 8 << " bit (reversed register order): " << hi << std::endl;
    }

    return {
            Instruction(type, addr, reg[3]),
            Instruction(type, addr + 1, reg[2]),
            Instruction(type, addr + 2, reg[1]),
            Instruction(type, addr + 3, reg[0]),
    };
}

// little endian reversed
static std::vector<Instruction>
        parse_i64badcfehg(Instruction::register_type_t type, std::size_t addr, const std::string &value, int base, bool verbose) {
    typedef int64_t            int_t;
    cxxendian::Host_Int<int_t> hi(parse_int<int_t>(value, base));
    cxxendian::LE_Int<int_t>   ei(hi);

    union {
        uint16_t reg[4];
        int_t    i;
    };

    i = ei.get();

    if (verbose) {
        std::cerr << "little endian signed integer " << sizeof(int_t) * 8 << " bit (reversed register order): " << hi << std::endl;
    }

    return {
            Instruction(type, addr, reg[3]),
            Instruction(type, addr + 1, reg[2]),
            Instruction(type, addr + 2, reg[1]),
            Instruction(type, addr + 3, reg[0]),
    };
}

}  // namespace InputParser
