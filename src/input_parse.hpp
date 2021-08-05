#pragma once

#include "split_string.hpp"

#include <stdexcept>
#include <string>

struct input_data_t {
    enum class register_type_t { DO, DI, AO, AI };

    register_type_t register_type;
    std::size_t     address;
    std::size_t     value;
};

/*! \brief parse input
 *
 * @param input input string
 * @param target target data struct
 * @param base_addr optional: numeric base for address conversion (see
 * https://en.cppreference.com/w/cpp/string/basic_string/stoul)
 * @param base_value optional: numeric base for value conversion (see
 * https://en.cppreference.com/w/cpp/string/basic_string/stoul)
 */
static void parse_input(const std::string &input, input_data_t &target, int base_addr = 10, int base_value = 10) {
    static constexpr std::size_t EXPECTED_ELEMENTS = 3;
    static constexpr char        DELIMITER         = ':';

    // split string
    auto split_input = split_string(input, DELIMITER);

    // check number of elements
    if (split_input.size() != EXPECTED_ELEMENTS) {
        throw std::invalid_argument("The input does not contain the appropriate number of delimiters");
    }

    // get register type
    input_data_t::register_type_t type;
    auto &                        type_str = split_input.at(0);
    for (auto &c : type_str)
        c = static_cast<char>(std::tolower(c));
    if (type_str == "do") {
        type = input_data_t::register_type_t::DO;
    } else if (type_str == "di") {
        type = input_data_t::register_type_t::DI;
    } else if (type_str == "ao") {
        type = input_data_t::register_type_t::AO;
    } else if (type_str == "ai") {
        type = input_data_t::register_type_t::AI;
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

    // get value
    std::size_t value;
    const auto &value_str = split_input.at(2);
    try {
        value = std::stoull(value_str, &idx, base_value);
    } catch (const std::exception &) { fail = true; }
    fail = fail || idx != value_str.size();

    if (fail) throw std::invalid_argument("Failed to parse value '" + value_str + '\'');

    target.register_type = type;
    target.address       = addr;
    target.value         = value;
}
