/*
 * Copyright (C) 2021-2022 Nikolas Koesling <nikolas@koesling.info>.
 * This program is free software. You can redistribute it and/or modify it under the terms of the GPLv3 License.
 */

#include "readline/history.h"
#include "readline/readline.h"

/**
 * @brief readline wrapper class
 */
class Readline {
public:
    /**
     * @brief initialize readline
     *
     * @details
     * - Disables the readline library signal handling
     * - Output to stderr instead of stdout --> passthrough option still works
     */
    Readline() {
        rl_catch_signals = 0;
        rl_outstream     = stderr;
    }

    /**
     * @brief disable readline
     *
     * @details
     * Restores the terminal state. Does only change anything if the readline() call was interrupted by a signal.
     */
    ~Readline() { rl_cleanup_after_signal(); }

    /**
     * @brief read a line from stdin using the readline library
     * @param prompt user prompt
     * @return read line
     *
     * @exception runtime_error thrown if there is no more data to read (EOF)
     */
    std::string get_line(const std::string &prompt = "") {
        const char *line = readline(prompt.c_str());
        if (line) {
            auto ret = std::string(line);
            free(const_cast<char *>(line));
            return ret;
        } else {
            throw std::runtime_error("eof");
        }
    }
};
