/*
 * Copyright (C) 2021-2022 Nikolas Koesling <nikolas@koesling.info>.
 * This program is free software. You can redistribute it and/or modify it under the terms of the MIT License.
 */

#include "SHM.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>

#include <utility>


SHM::SHM(std::string name) : name(std::move(name)) {
    // open shared memory object
    fd = shm_open(this->name.c_str(), O_RDWR, 0660);
    if (fd < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to open shared memory '" + this->name + '\'');
    }

    // get size of shared memory object
    struct stat shm_stats {};
    if (fstat(fd, &shm_stats)) {
        if (close(fd)) { perror("close"); }
        fd = -1;
        throw std::system_error(
                errno, std::generic_category(), "Failed to read stats of shared memory '" + this->name + '\'');
    }
    size = static_cast<size_t>(shm_stats.st_size);

    // map shared memory
    addr = mmap(nullptr, size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED || addr == nullptr) {
        if (close(fd)) { perror("close"); }
        addr = nullptr;
        throw std::system_error(errno, std::generic_category(), "Failed to mmap shared memory '" + this->name + '\'');
    }
}

SHM::~SHM() {
    if (addr) {
        if (munmap(addr, size)) { perror("munmap"); }
    }
    if (fd != -1) {
        if (close(fd)) { perror("close"); }
    }
}
