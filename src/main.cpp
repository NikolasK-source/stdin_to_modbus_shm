/*
 * Copyright (C) 2021-2022 Nikolas Koesling <nikolas@koesling.info>.
 * This program is free software. You can redistribute it and/or modify it under the terms of the MIT License.
 */

#include "SHM.hpp"
#include "input_parse.hpp"
#include "license.hpp"

#include <csignal>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <sysexits.h>
#include <thread>

//! maximum number of modbus registers
static constexpr std::size_t MAX_MODBUS_REGS = 0x10000;

/*! \brief main function
 *
 * @param argc number of arguments
 * @param argv arguments as char* array
 * @return exit code
 */
int main(int argc, char **argv) {
    const std::string exe_name = std::filesystem::path(argv[0]).filename().string();
    cxxopts::Options  options(exe_name, "Read instructions from stdin and write them to a modbus shared memory");

    auto exit_usage = [&exe_name]() {
        std::cerr << "Use '" << exe_name << " --help' for more information." << std::endl;
        exit(EX_USAGE);
    };

    // establish signal handler
    static volatile bool terminate   = false;
    auto                 sig_handler = [](int) { terminate = true; };
    if (signal(SIGINT, sig_handler) || signal(SIGTERM, sig_handler)) {
        perror("Failed to establish signal handler");
        exit(EX_OSERR);
    }

    // all command line arguments
    options.add_options()("n,name-prefix",
                          "name prefix of the shared memory objects",
                          cxxopts::value<std::string>()->default_value("modbus_"));
    options.add_options()("address-base",
                          "Numerical base (radix) that is used for the addresses (see "
                          "https://en.cppreference.com/w/cpp/string/basic_string/stoul)",
                          cxxopts::value<int>()->default_value("0"));
    options.add_options()("value-base",
                          "Numerical base (radix) that is used for the values (see "
                          "https://en.cppreference.com/w/cpp/string/basic_string/stoul)",
                          cxxopts::value<int>()->default_value("0"));
    options.add_options()("h,help", "print usage");
    options.add_options()("version", "print version information");
    options.add_options()("license", "show licenses");

    // parse arguments
    cxxopts::ParseResult args;
    try {
        args = options.parse(argc, argv);
    } catch (cxxopts::OptionParseException &e) {
        std::cerr << "Failed to parse arguments: " << e.what() << '.' << std::endl;
        exit_usage();
    }

    // print usage
    if (args.count("help")) {
        options.set_width(120);
        std::cout << options.help() << std::endl;
        std::cout << std::endl;
        std::cout << "Data input format: reg_type:address:value" << std::endl;
        std::cout << "    reg_type: modbus register type:                         [do|di|ao|ai]" << std::endl;
        std::cout << "    address : address of the target register:               [0-" << MAX_MODBUS_REGS - 1 << "]"
                  << std::endl;
        std::cout << "    value   : value that is written to the target register: [0-"
                  << std::numeric_limits<uint16_t>::max() << "]" << std::endl;
        std::cout << "              For the registers do and di all numerical values different from 0 are interpreted "
                     "as 1."
                  << std::endl;
        std::cout << std::endl;
        std::cout << "This application uses the following libraries:" << std::endl;
        std::cout << "  - cxxopts by jarro2783 (https://github.com/jarro2783/cxxopts)" << std::endl;
        exit(EX_OK);
    }

    // print version
    if (args.count("version")) {
        std::cout << PROJECT_NAME << " " << PROJECT_VERSION << std::endl;
        exit(EX_OK);
    }

    // print licenses
    if (args.count("license")) {
        print_licenses(std::cout);
        exit(EX_OK);
    }

    // open shared memory objects
    const auto &name_prefix = args["name-prefix"].as<std::string>();

    std::unique_ptr<SHM> shm_do;
    std::unique_ptr<SHM> shm_di;
    std::unique_ptr<SHM> shm_ao;
    std::unique_ptr<SHM> shm_ai;

    try {
        shm_do = std::make_unique<SHM>(name_prefix + "DO");
        shm_di = std::make_unique<SHM>(name_prefix + "DI");
        shm_ao = std::make_unique<SHM>(name_prefix + "AO");
        shm_ai = std::make_unique<SHM>(name_prefix + "AI");
    } catch (const std::system_error &e) {
        std::cerr << e.what() << std::endl;
        exit(EX_OSERR);
    }

    // check shared mem
    if (shm_do->get_size() > MAX_MODBUS_REGS) {
        std::cerr << "shared memory '" << shm_do->get_name() << "is to large to be a valid modbus shared memory."
                  << std::endl;
        exit(EX_SOFTWARE);
    }

    if (shm_di->get_size() > MAX_MODBUS_REGS) {
        std::cerr << "shared memory '" << shm_di->get_name() << "' is to large to be a valid modbus shared memory."
                  << std::endl;
        exit(EX_SOFTWARE);
    }

    if (shm_ao->get_size() / 2 > MAX_MODBUS_REGS) {
        std::cerr << "shared memory '" << shm_ao->get_name() << "' is to large to be a valid modbus shared memory."
                  << std::endl;
        exit(EX_SOFTWARE);
    }

    if (shm_ai->get_size() / 2 > MAX_MODBUS_REGS) {
        std::cerr << "shared memory '" << shm_ai->get_name() << "' is to large to be a valid modbus shared memory."
                  << std::endl;
        exit(EX_SOFTWARE);
    }

    if (shm_ao->get_size() % 2) {
        std::cerr << "the size of shared memory '" << shm_ao->get_name() << "' is odd. It is not a valid modbus shm."
                  << std::endl;
        exit(EX_SOFTWARE);
    }

    if (shm_ai->get_size() % 2) {
        std::cerr << "the size of shared memory '" << shm_ai->get_name() << "' is odd. It is not a valid modbus shm."
                  << std::endl;
        exit(EX_SOFTWARE);
    }

    const std::size_t do_elements = shm_do->get_size();
    const std::size_t di_elements = shm_di->get_size();
    const std::size_t ao_elements = shm_ao->get_size() / 2;
    const std::size_t ai_elements = shm_ai->get_size() / 2;

    const int addr_base  = args["address-base"].as<int>();
    const int value_base = args["value-base"].as<int>();

    std::mutex m;  // to ensure that the program is not terminated while it writes to a shared memory

    auto input_thread_func = [&] {
        std::string line;
        while (!terminate && std::getline(std::cin, line)) {
            // parse input
            input_data_t input_data {};
            try {
                parse_input(line, input_data, addr_base, value_base);
            } catch (std::exception &e) {
                std::cerr << "line '" << line << "' discarded: " << e.what() << std::endl;
                continue;
            }

            // write value to target
            std::lock_guard<std::mutex> guard(m);
            switch (input_data.register_type) {
                case input_data_t::register_type_t::DO:
                    if (input_data.address >= do_elements) {
                        std::cerr << "line '" << line << "' discarded: address out of range" << std::endl;
                        break;
                    }
                    shm_do->get_addr<uint8_t *>()[input_data.address] = input_data.value ? 1 : 0;
                    break;
                case input_data_t::register_type_t::DI:
                    if (input_data.address >= di_elements) {
                        std::cerr << "line '" << line << "' discarded: address out of range" << std::endl;
                        break;
                    }
                    shm_di->get_addr<uint8_t *>()[input_data.address] = input_data.value ? 1 : 0;
                    break;
                case input_data_t::register_type_t::AO:
                    if (input_data.address >= ao_elements) {
                        std::cerr << "line '" << line << "' discarded: address out of range" << std::endl;
                        break;
                    }
                    if (input_data.value > std::numeric_limits<uint16_t>::max()) {
                        std::cerr << "line '" << line << "' discarded: value out of range" << std::endl;
                        break;
                    }
                    shm_ao->get_addr<uint16_t *>()[input_data.address] = static_cast<uint16_t>(input_data.value);
                    break;
                case input_data_t::register_type_t::AI:
                    if (input_data.address >= ai_elements) {
                        std::cerr << "line '" << line << "' discarded: address out of range" << std::endl;
                        break;
                    }
                    if (input_data.value > std::numeric_limits<uint16_t>::max()) {
                        std::cerr << "line '" << line << "' discarded: value out of range" << std::endl;
                        break;
                    }
                    shm_ai->get_addr<uint16_t *>()[input_data.address] = static_cast<uint16_t>(input_data.value);
                    break;
            }
        }
        terminate = true;
    };

    // start input thread.
    // a detached thread will be terminated by its destructor as soon as the thread object is out of scope
    // (end of function main)
    std::thread input_thread(input_thread_func);
    input_thread.detach();

    while (!terminate) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::lock_guard<std::mutex> guard(m);  // wait until the thread is not within a critical section
    std::cerr << "Terminating ..." << std::endl;
}
