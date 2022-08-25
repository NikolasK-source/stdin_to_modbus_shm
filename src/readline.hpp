/*
 * Copyright (C) 2021-2022 Nikolas Koesling <nikolas@koesling.info>.
 * This program is free software. You can redistribute it and/or modify it under the terms of the MIT License.
 */

#include "readline/history.h"
#include "readline/readline.h"

class Readline {
public:
    Readline() { rl_catch_signals = 0; }

    ~Readline() { rl_cleanup_after_signal(); }

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
