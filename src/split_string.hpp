#pragma once

#include <string>
#include <vector>

/*! \brief Split a string into a vector of strings
 *
 * @param string the string that shall be split
 * @param delimiter Specifies the separator to use when splitting the string (string of any length)
 * @param max_split optional: Specifies how many splits to do. (default: infinite from the point of view of possible
 * system resources)
 * @return split string as vector of strings
 */
[[nodiscard]] static inline std::vector<std::string> split_string(
        std::string string, const std::string &delimiter, std::size_t max_split = ~static_cast<std::size_t>(0)) {
    std::vector<std::string> split_string;  // result vector

    std::size_t pos = 0;
    while (max_split && ((pos = string.find(delimiter)) != std::string::npos)) {
        split_string.emplace_back(string.substr(0, pos));
        string.erase(0, pos + delimiter.length());
        max_split--;
    }

    if (!string.empty()) split_string.emplace_back(string);

    return split_string;
}

/*! \brief Split a string into a vector of strings
 *
 * @param string the string that shall be split
 * @param delimiter Specifies the separator to use when splitting the string (single character)
 * @param max_split optional: Specifies how many splits to do. (default: infinite from the point of view of possible
 * system resources)
 * @return split string as vector of strings
 */
[[nodiscard]] static inline std::vector<std::string>
        split_string(const std::string &string, char delimiter, std::size_t max_split = ~static_cast<std::size_t>(0)) {
    return split_string(string, std::string(1, delimiter), max_split);
}
