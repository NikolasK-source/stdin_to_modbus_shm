/*
 * Copyright (C) 2021-2022 Nikolas Koesling <nikolas@koesling.info>.
 * This program is free software. You can redistribute it and/or modify it under the terms of the GPLv3 License.
 */

#include "InputParser.hpp"
#include "license.hpp"
#include "readline.hpp"

#include "cxxsemaphore.hpp"
#include "cxxshm.hpp"
#include "generated/version_info.hpp"
#include <array>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cxxendian/endian.hpp>
#include <cxxopts.hpp>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sys/ioctl.h>
#include <sysexits.h>
#include <thread>
#include <unistd.h>

//! maximum number of modbus registers
static constexpr std::size_t MAX_MODBUS_REGS = 0x10000;

//! minimum time to sleep (bash script passthrough)
static constexpr double MIN_BASH_SLEEP = 0.1;

//! number of digits that have to be printed for bash sleep instructions
constexpr int SLEEP_DIGITS = 1;

//* value to increment error counter if semaphore could not be acquired
static constexpr long SEMAPHORE_ERROR_INC = 10;

//* value to decrement error counter if semaphore could be acquired
static constexpr long SEMAPHORE_ERROR_DEC = 1;

//* maximum value of semaphore error counter
static constexpr long SEMAPHORE_ERROR_MAX = 100;

constexpr std::array<int, 10> TERM_SIGNALS = {SIGINT,
                                              SIGTERM,
                                              SIGHUP,
                                              SIGIO,  // should not happen
                                              SIGPIPE,
                                              SIGPOLL,  // should not happen
                                              SIGPROF,  // should not happen
                                              SIGUSR1,
                                              SIGUSR2,
                                              SIGVTALRM};

/*! \brief main function
 *
 * @param argc number of arguments
 * @param argv arguments as char* array
 * @return exit code
 */
int main(int argc, char **argv) {
    const std::string exe_name = std::filesystem::path(*argv).filename().string();
    cxxopts::Options  options(exe_name, "Read instructions from stdin and write them to a Modbus shared memory");

    auto exit_usage = [&exe_name]() {
        std::cerr << "Use '" << exe_name << " --help' for more information.\n";
        exit(EX_USAGE);
    };

    // establish signal handler
    static volatile bool terminate = false;
    struct sigaction     term_sa {};
    term_sa.sa_handler = [](int) { terminate = true; };
    term_sa.sa_flags   = SA_RESTART;
    sigemptyset(&term_sa.sa_mask);
    for (const auto SIGNO : TERM_SIGNALS) {
        if (sigaction(SIGNO, &term_sa, nullptr)) {
            perror("Failed to establish signal handler");
            return EX_OSERR;
        }
    }

    // all command line arguments
    options.add_options("shared memory")("n,name-prefix",
                                         "name prefix of the shared memory objects",
                                         cxxopts::value<std::string>()->default_value("modbus_"));
    options.add_options("settings")("address-base",
                                    "Numerical base (radix) that is used for the addresses (see "
                                    "https://en.cppreference.com/w/cpp/string/basic_string/stoul)",
                                    cxxopts::value<int>()->default_value("0"));
    options.add_options("settings")("value-base",
                                    "Numerical base (radix) that is used for the values (see "
                                    "https://en.cppreference.com/w/cpp/string/basic_string/stoul)",
                                    cxxopts::value<int>()->default_value("0"));
    options.add_options("settings")("p,passthrough", "write passthrough all executed commands to stdout");
    options.add_options("settings")("bash", "passthrough as bash script. No effect i '--passthrough' is not set");
    options.add_options("settings")("valid-hist", "add only valid commands to command history");
    options.add_options("other")("h,help", "print usage");
    options.add_options("other")("v,verbose", "print what is written to the registers");
    options.add_options("version information")("version", "print version and exit");
    options.add_options("version information")("longversion",
                                               "print version (including compiler and system info) and exit");
    options.add_options("version information")("shortversion", "print version (only version string) and exit");
    options.add_options("version information")("git-hash", "print git hash");
    options.add_options("other")("license", "show licenses");
    options.add_options("other")("license-full", "show licences (full license text)");
    options.add_options("other")("data-types", "show list of supported data type identifiers");
    options.add_options("other")("constants", "list string constants that can be used as value");
    options.add_options("shared memory")(
            "semaphore",
            "protect the shared memory with an existing named semaphore against simultaneous access",
            cxxopts::value<std::string>());
    options.add_options("shared memory")("semaphore-timeout",
                                         "maximum time (in seconds) to wait for semaphore (default: 0.1)",
                                         cxxopts::value<double>()->default_value("0.1"));
    options.add_options("shared_memory")(
            "pid",
            "terminate application if application with given pid is terminated. Provide "
            "the pid of the Modbus client to terminate when the Modbus client is terminated.",
            cxxopts::value<pid_t>());

    // parse arguments
    cxxopts::ParseResult args;
    try {
        args = options.parse(argc, argv);
    } catch (cxxopts::exceptions::parsing::exception &e) {
        std::cerr << "Failed to parse arguments: " << e.what() << '.' << '\n';
        exit_usage();
    }

    auto print_format = [](bool help = false) {
        std::cout << "Data input format: reg_type:address:value[:data_type]" << '\n';
        std::cout << "    reg_type : modbus register type:           [do|di|ao|ai]" << '\n';
        std::cout << "    address  : address of the target register: [0-" << MAX_MODBUS_REGS - 1 << "]" << '\n';
        std::cout << "               The actual maximum register depends on the size of the Modbus shared memory."
                  << '\n';
        std::cout << "    value    : value that is written to the target register" << '\n';
        std::cout << "               Some string constants are available. The input format depends on the type of "
                     "register and data type."
                  << '\n';
        if (help) std::cout << "               Use --constants for more details.";
        else
            std::cout << "               Type 'help constants' for more details ";
        std::cout << '\n';
        std::cout << "               For the registers do and di all numerical values different from 0 are interpreted "
                     "as 1."
                  << '\n';
        std::cout << "    data_type: an optional data type specifier" << '\n';
        std::cout << "               If no data type is specified, exactly one register is written in host byte order."
                  << '\n';
        if (help)
            std::cout << "               Use --data-types to get a list of supported data type identifiers." << '\n';
        else
            std::cout << "               Type 'help types' to get a list of supported data type identifiers." << '\n';
    };

    // print usage
    if (args.count("help")) {
        static constexpr std::size_t MIN_HELP_SIZE = 80;
        if (isatty(STDIN_FILENO)) {
            struct winsize w {};
            if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != -1) {  // NOLINT
                options.set_width(std::max(static_cast<decltype(w.ws_col)>(MIN_HELP_SIZE), w.ws_col));
            }
        } else {
            options.set_width(MIN_HELP_SIZE);
        }
        std::cout << options.help() << '\n';
        std::cout << '\n';
        print_format(true);
        std::cout << '\n';
        std::cout << "This application uses the following libraries:" << '\n';
        std::cout << "  - cxxopts by jarro2783 (https://github.com/jarro2783/cxxopts)" << '\n';
        std::cout << "  - cxxshm (https://github.com/NikolasK-source/cxxshm)" << '\n';
        std::cout << "  - cxxendian (https://github.com/NikolasK-source/cxxendian)" << '\n';
        std::cout << "  - GNU Readline (http://git.savannah.gnu.org/cgit/readline.git/)" << '\n';
        return EX_OK;
    }

    // print version
    if (args.count("shortversion")) {
        std::cout << PROJECT_VERSION << '\n';
        return EX_OK;
    }

    if (args.count("version")) {
        std::cout << PROJECT_NAME << ' ' << PROJECT_VERSION << '\n';
        return EX_OK;
    }

    if (args.count("longversion")) {
        std::cout << PROJECT_NAME << ' ' << PROJECT_VERSION << '\n';
        std::cout << "   compiled with " << COMPILER_INFO << '\n';
        std::cout << "   on system " << SYSTEM_INFO << '\n';
        std::cout << "   from git commit " << RCS_HASH << '\n';
        return EX_OK;
    }

    if (args.count("git-hash")) {
        std::cout << RCS_HASH << '\n';
        return EX_OK;
    }

    // print licenses
    if (args.count("license")) {
        print_licenses(std::cout, false);
        return EX_OK;
    }

    if (args.count("license-full")) {
        print_licenses(std::cout, true);
        return EX_OK;
    }

    auto print_data_types = []() {
        std::cout << "Supported data types:" << '\n';
        std::cout << "  - Float:" << '\n';
        std::cout << "      - 32 Bit:" << '\n';
        std::cout << "          - f32_abcd, f32_big, f32b                32-Bit floating point   in big endian" << '\n';
        std::cout << "          - f32_dcba, f32_little, f32l             32-Bit floating point   in little endian"
                  << '\n';
        std::cout << "          - f32_cdab, f32_big_rev, f32br           32-Bit floating point   in big endian,     - "
                     "reversed register order"
                  << '\n';
        std::cout << "          - f32_badc, f32_little_rev, f32lr        32-Bit floating point   in little endian,  - "
                     "reversed register order"
                  << '\n';
        std::cout << "      - 64 Bit:" << '\n';
        std::cout << "          - f64_abcdefgh, f64_big, f64b            64-Bit floating point   in big endian" << '\n';
        std::cout << "          - f64_ghefcdab, f64_little, f64l         64-Bit floating point   in little endian"
                  << '\n';
        std::cout << "          - f64_badcfehg, f64_big_rev, f64br       64-Bit floating point   in big endian,     - "
                     "reversed register order"
                  << '\n';
        std::cout << "          - f64_hgfedcba, f64_little_rev, f64lr    64-Bit floating point   in little endian,  - "
                     "reversed register order"
                  << '\n';
        std::cout << "  - Int:" << '\n';
        std::cout << "      - 8 Bit:" << '\n';
        std::cout << "          - u8_lo                                  8-Bit unsigned integer   written to low  byte "
                     "of register"
                  << '\n';
        std::cout << "          - u8_hi                                  8-Bit unsigned integer   written to high byte "
                     "of register"
                  << '\n';
        std::cout << "          - i8_lo                                  8-Bit   signed integer   written to low  byte "
                     "of register"
                  << '\n';
        std::cout << "          - i8_hi                                  8-Bit   signed integer   written to high byte "
                     "of register"
                  << '\n';
        std::cout << "      - 16 Bit" << '\n';
        std::cout << "          - u16_ab, u16_big, u16b                  16-Bit unsigned integer in big endian" << '\n';
        std::cout << "          - i16_ab, i16_big, i16b                  16-Bit signed integer   in big endian" << '\n';
        std::cout << "          - u16_ba, u16_little, u16l               16-Bit unsigned integer in little endian"
                  << '\n';
        std::cout << "          - i16_ba, i16_little, i16l               16-Bit signed integer   in little endian"
                  << '\n';
        std::cout << "      - 32 Bit:" << '\n';
        std::cout << "          - u32_abcd, u32_big, u32b                32-Bit unsigned integer in big endian" << '\n';
        std::cout << "          - i32_abcd, i32_big, i32b                32-Bit   signed integer in big endian" << '\n';
        std::cout << "          - u32_dcba, u32_little, u32l             32-Bit unsigned integer in little endian"
                  << '\n';
        std::cout << "          - i32_dcba, i32_little, i32l             32-Bit   signed integer in little endian"
                  << '\n';
        std::cout << "          - u32_cdab, u32_big_rev, u32br           32-Bit unsigned integer in big endian,     - "
                     "reversed register order"
                  << '\n';
        std::cout << "          - i32_cdab, i32_big_rev, i32br           32-Bit   signed integer in big endian,     - "
                     "reversed register order"
                  << '\n';
        std::cout << "          - u32_badc, u32_little_rev, u32lr        32-Bit unsigned integer in little endian,  - "
                     "reversed register order"
                  << '\n';
        std::cout << "          - i32_badc, i32_little_rev, i32lr        32-Bit   signed integer in little endian,  - "
                     "reversed register order"
                  << '\n';
        std::cout << "      - 64 Bit:" << '\n';
        std::cout << "          - u64_abcdefgh, u64_big, u64b            64-Bit unsigned integer in big endian" << '\n';
        std::cout << "          - i64_abcdefgh, i64_big, i64b            64-Bit   signed integer in big endian" << '\n';
        std::cout << "          - u64_hgfedcba, u64_little, u64l         64-Bit unsigned integer in little endian"
                  << '\n';
        std::cout << "          - i64_hgfedcba, i64_little, i64l         64-Bit   signed integer in little endian"
                  << '\n';
        std::cout << "          - u64_ghefcdab, u64_big_rev, u64br       64-Bit unsigned integer in big endian      - "
                     "reversed register order"
                  << '\n';
        std::cout << "          - i64_ghefcdab, i64_big_rev, i64br       64-Bit   signed integer in big endian      - "
                     "reversed register order"
                  << '\n';
        std::cout << "          - u64_badcfehg, u64_little_rev, u64lr    64-Bit unsigned integer in little endian,  - "
                     "reversed register order"
                  << '\n';
        std::cout << "          - i64_badcfehg, i64_little_rev, i64lr    64-Bit   signed integer in little endian,  - "
                     "reversed register order"
                  << '\n';
        std::cout << '\n';
        std::cout << "Note: The endianness refers to the layout of the data in the shared memory and may differ from "
                     "the definition of the Modbus Server"
                  << '\n';
        std::cout << "      definition of the endianness." << '\n';
    };

    // data type identifiers
    if (args.count("data-types")) {
        print_data_types();
        return EX_OK;
    }

    auto print_constants = []() {
        std::cout << "Known string constants:" << '\n';
        std::cout << "  true      1" << '\n';
        std::cout << "  one       1" << '\n';
        std::cout << "  high      1" << '\n';
        std::cout << "  active    1" << '\n';
        std::cout << "  on        1" << '\n';
        std::cout << "  enabled   1" << '\n';
        std::cout << "  false     0" << '\n';
        std::cout << "  zero      0" << '\n';
        std::cout << "  low       0" << '\n';
        std::cout << "  inactive  0" << '\n';
        std::cout << "  off       0" << '\n';
        std::cout << "  off       0" << '\n';
        std::cout << "  pi        " << InputParser::PI << '\n';
        std::cout << "  -pi      " << InputParser::NPI << '\n';
        std::cout << "  sqrt2     " << InputParser::SQRT2 << '\n';
        std::cout << "  sqrt3     " << InputParser::SQRT3 << '\n';
        std::cout << "  phi       " << InputParser::PHI << '\n';
        std::cout << "  ln2       " << InputParser::LN2 << '\n';
        std::cout << "  e         " << InputParser::E << '\n';
    };

    // print licenses
    if (args.count("constants")) {
        print_constants();
        return EX_OK;
    }

    const bool VERBOSE          = args.count("verbose");
    const bool PASSTHROUGH      = args.count("passthrough");
    const bool PASSTHROUGH_BASH = args.count("bash");
    const bool INTERACTIVE      = isatty(STDIN_FILENO) == 1;  // enable command history if input is tty
    const bool VALID_HIST       = args.count("valid-hist");

    std::unique_ptr<Readline> readline;
    if (INTERACTIVE) { readline = std::make_unique<Readline>(); }

    const std::string REGISTER_ENDIAN = endian::HostEndianness.isBig() ? "u16b" : "u16l";

    // open shared memory objects
    const auto &name_prefix = args["name-prefix"].as<std::string>();

    std::unique_ptr<cxxshm::SharedMemory> shm_do;
    std::unique_ptr<cxxshm::SharedMemory> shm_di;
    std::unique_ptr<cxxshm::SharedMemory> shm_ao;
    std::unique_ptr<cxxshm::SharedMemory> shm_ai;

    try {
        shm_do = std::make_unique<cxxshm::SharedMemory>(name_prefix + "DO");
        shm_di = std::make_unique<cxxshm::SharedMemory>(name_prefix + "DI");
        shm_ao = std::make_unique<cxxshm::SharedMemory>(name_prefix + "AO");
        shm_ai = std::make_unique<cxxshm::SharedMemory>(name_prefix + "AI");
    } catch (const std::system_error &e) {
        std::cerr << e.what() << '\n';
        return EX_OSERR;
    }

    // check shared mem
    if (shm_do->get_size() > MAX_MODBUS_REGS) {
        std::cerr << "shared memory '" << shm_do->get_name() << "is to large to be a valid Modbus shared memory."
                  << '\n';
        return EX_SOFTWARE;
    }

    if (shm_di->get_size() > MAX_MODBUS_REGS) {
        std::cerr << "shared memory '" << shm_di->get_name() << "' is to large to be a valid Modbus shared memory."
                  << '\n';
        return EX_SOFTWARE;
    }

    if (shm_ao->get_size() / 2 > MAX_MODBUS_REGS) {
        std::cerr << "shared memory '" << shm_ao->get_name() << "' is to large to be a valid Modbus shared memory."
                  << '\n';
        return EX_SOFTWARE;
    }

    if (shm_ai->get_size() / 2 > MAX_MODBUS_REGS) {
        std::cerr << "shared memory '" << shm_ai->get_name() << "' is to large to be a valid Modbus shared memory."
                  << '\n';
        return EX_SOFTWARE;
    }

    if (VERBOSE) {
        std::cerr << "DO registers: " << shm_do->get_size() << '\n';
        std::cerr << "DI registers: " << shm_di->get_size() << '\n';
        std::cerr << "AO registers: " << shm_ao->get_size() / 2 << '\n';
        std::cerr << "AI registers: " << shm_ai->get_size() / 2 << '\n';
    }

    if (shm_ao->get_size() % 2) {
        std::cerr << "the size of shared memory '" << shm_ao->get_name() << "' is odd. It is not a valid Modbus shm."
                  << '\n';
        return EX_SOFTWARE;
    }

    if (shm_ai->get_size() % 2) {
        std::cerr << "the size of shared memory '" << shm_ai->get_name() << "' is odd. It is not a valid Modbus shm."
                  << '\n';
        return EX_SOFTWARE;
    }

    const std::size_t do_elements = shm_do->get_size();
    const std::size_t di_elements = shm_di->get_size();
    const std::size_t ao_elements = shm_ao->get_size() / 2;
    const std::size_t ai_elements = shm_ai->get_size() / 2;

    const int addr_base  = args["address-base"].as<int>();
    const int value_base = args["value-base"].as<int>();

    std::mutex m;  // to ensure that the program is not terminated while it writes to a shared memory

    std::unique_ptr<cxxsemaphore::Semaphore> semaphore;
    long                                     semaphore_error_counter = 0;
    if (args.count("semaphore")) {
        try {
            semaphore = std::make_unique<cxxsemaphore::Semaphore>(args["semaphore"].as<std::string>());
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
            return EX_SOFTWARE;
        }
    } else {
        std::cerr << "WARNING: No semaphore specified.\n"
                     "         Concurrent access to the shared memory is possible.\n"
                     "         This can result in CORRUPTED DATA.\n"
                     "         Use --semaphore to specify a semaphore that is provided by the Modbus client.\n";
        std::cerr << std::flush;
    }

    const double SEMAPHORE_TIMEOUT_S = args["semaphore-timeout"].as<double>();
    if (SEMAPHORE_TIMEOUT_S < 0.000'001) {
        std::cerr << "semaphore-timeout: invalid value" << '\n';
        return EX_USAGE;
    }

    double         modf_dummy {};
    const timespec SEMAPHORE_MAX_TIME = {
            static_cast<time_t>(args["semaphore-timeout"].as<double>()),
            static_cast<suseconds_t>(std::modf(SEMAPHORE_TIMEOUT_S, &modf_dummy) * 1'000'000),
    };

    // modbus client pid
    pid_t mb_client_pid     = 0;
    bool  use_mb_client_pid = false;
    if (args.count("pid")) {
        mb_client_pid     = args["pid"].as<pid_t>();
        use_mb_client_pid = true;
    } else {
        std::cerr << "WARNING: No Modbus client pid provided.\n"
                     "         Terminating the Modbus client application WILL NOT result in the termination of this "
                     "application.\n"
                     "         This application WILL NOT connect to the shared memory of a restarted Modbus client.\n"
                     "         Use --pid to specify the pid of the Modbus client.\n"
                     "         Command line example: --pid $(pidof modbus-tcp-client-shm)\n"
                  << std::flush;
    }

    std::cout << std::fixed;

    auto last_time  = std::chrono::steady_clock::now();
    auto bash_sleep = [&last_time]() {
        auto this_time  = std::chrono::steady_clock::now();
        auto ms         = std::chrono::duration_cast<std::chrono::milliseconds>(this_time - last_time).count();
        auto sleep_time = static_cast<double>(ms) / 1000.0;
        if (sleep_time > MIN_BASH_SLEEP) {
            last_time = this_time;
            std::cout << "sleep " << std::setprecision(SLEEP_DIGITS) << sleep_time << std::endl;  // NOLINT
        }
    };

    auto input_thread_func = [&] {
        while (!terminate) {
            std::string line;
            if (INTERACTIVE) {
                try {
                    line = readline->get_line(">>> ");
                } catch (const std::runtime_error &) {
                    // eof
                    break;
                }

                if (line == "exit") break;

                if (line == "help") {
                    std::cout << "usage: help {format, constants, types}" << '\n';
                    std::cout << '\n';
                    std::cout << "    Type 'exit' to exit the application." << std::endl;  // NOLINT
                    continue;
                }

                if (line == "help format") {
                    print_format(false);
                    add_history(line.c_str());
                    continue;
                }

                if (line == "help constants") {
                    print_constants();
                    add_history(line.c_str());
                    continue;
                }

                if (line == "help types") {
                    print_data_types();
                    add_history(line.c_str());
                    continue;
                }

                if (!line.empty() && !VALID_HIST) add_history(line.c_str());
            } else {
                if (!std::getline(std::cin, line)) break;
            }


            // parse input
            std::vector<InputParser::Instruction> instructions;
            try {
                instructions = InputParser::parse(line, addr_base, value_base, VERBOSE);
            } catch (std::exception &e) {
                std::cerr << "line '" << line << "' discarded: " << e.what() << std::endl;  // NOLINT
                continue;
            }

            if (INTERACTIVE && VALID_HIST) add_history(line.c_str());

            // write value to target
            std::lock_guard<std::mutex> guard(m);

            if (semaphore) {
                while (!semaphore->wait(SEMAPHORE_MAX_TIME)) {
                    std::cerr << " WARNING: Failed to acquire semaphore '" << semaphore->get_name() << "' within "
                              << SEMAPHORE_TIMEOUT_S << "s." << std::endl;  // NOLINT

                    semaphore_error_counter += SEMAPHORE_ERROR_INC;

                    if (semaphore_error_counter >= SEMAPHORE_ERROR_MAX) {
                        std::cerr << "ERROR: Repeatedly failed to acquire the semaphore\n";
                        return EX_SOFTWARE;
                    }
                }

                semaphore_error_counter -= SEMAPHORE_ERROR_DEC;
                if (semaphore_error_counter < 0) semaphore_error_counter = 0;
            }

            for (auto &input_data : instructions) {
                switch (input_data.register_type) {
                    case InputParser::Instruction::register_type_t::DO: {
                        if (input_data.address >= do_elements) {
                            std::cerr << "line '" << line << "' discarded: address out of range"
                                      << std::endl;  // NOLINT
                            break;
                        }
                        uint8_t value                                     = input_data.value ? 1 : 0;
                        shm_do->get_addr<uint8_t *>()[input_data.address] = value;
                        if (VERBOSE) {
                            std::cerr << "> write " << std::hex << "0x" << std::setw(2) << std::setfill('0') << value
                                      << " to DO @0x" << std::setw(4) << input_data.address << std::endl;  // NOLINT
                        }

                        if (PASSTHROUGH) {
                            if (PASSTHROUGH_BASH) {
                                bash_sleep();
                                std::cout << "echo '";
                            }
                            std::cout << "do:" << input_data.address << ':' << static_cast<int>(value);
                            if (PASSTHROUGH_BASH) std::cout << "'";
                            std::cout << std::endl;  // NOLINT
                        }

                        break;
                    }
                    case InputParser::Instruction::register_type_t::DI: {
                        if (input_data.address >= di_elements) {
                            std::cerr << "line '" << line << "' discarded: address out of range"
                                      << std::endl;  // NOLINT
                            break;
                        }
                        uint8_t value                                     = input_data.value ? 1 : 0;
                        shm_di->get_addr<uint8_t *>()[input_data.address] = value;

                        if (VERBOSE) {
                            std::cerr << "> write " << std::hex << "0x" << std::setw(2) << std::setfill('0') << value
                                      << " to DI @0x" << std::setw(4) << input_data.address << std::endl;  // NOLINT
                        }

                        if (PASSTHROUGH) {
                            if (PASSTHROUGH_BASH) {
                                bash_sleep();
                                std::cout << "echo '";
                            }
                            std::cout << "di:" << input_data.address << ':' << static_cast<int>(value);
                            if (PASSTHROUGH_BASH) std::cout << "'";
                            std::cout << std::endl;  // NOLINT
                        }

                        break;
                    }
                    case InputParser::Instruction::register_type_t::AO:
                        if (input_data.address >= ao_elements) {
                            std::cerr << "line '" << line << "' discarded: address out of range"
                                      << std::endl;  // NOLINT
                            break;
                        }
                        shm_ao->get_addr<uint16_t *>()[input_data.address] = static_cast<uint16_t>(input_data.value);

                        if (VERBOSE) {
                            std::cerr << "> write " << std::hex << "0x" << std::setw(4) << std::setfill('0')
                                      << input_data.value << " to AO @0x" << std::setw(4) << input_data.address
                                      << std::endl;  // NOLINT
                        }

                        if (PASSTHROUGH) {
                            if (PASSTHROUGH_BASH) {
                                bash_sleep();
                                std::cout << "echo '";
                            }
                            std::cout << "ao:" << input_data.address << ':' << static_cast<uint16_t>(input_data.value)
                                      << ':' << REGISTER_ENDIAN;
                            if (PASSTHROUGH_BASH) std::cout << "'";
                            std::cout << std::endl;  // NOLINT
                        }
                        break;
                    case InputParser::Instruction::register_type_t::AI:
                        if (input_data.address >= ai_elements) {
                            std::cerr << "line '" << line << "' discarded: address out of range"
                                      << std::endl;  // NOLINT
                            break;
                        }
                        shm_ai->get_addr<uint16_t *>()[input_data.address] = static_cast<uint16_t>(input_data.value);

                        if (VERBOSE) {
                            std::cerr << "> write " << std::hex << "0x" << std::setw(4) << std::setfill('0')
                                      << input_data.value << " to AI @0x" << std::setw(4) << input_data.address
                                      << std::endl;  // NOLINT
                        }

                        if (PASSTHROUGH) {
                            if (PASSTHROUGH_BASH) {
                                bash_sleep();
                                std::cout << "echo '";
                            }
                            std::cout << "ai:" << input_data.address << ':' << static_cast<uint16_t>(input_data.value)
                                      << ':' << REGISTER_ENDIAN;
                            if (PASSTHROUGH_BASH) std::cout << "'";
                            std::cout << std::endl;  // NOLINT
                        }
                        break;
                }
            }

            if (semaphore && semaphore->is_acquired()) semaphore->post();
        }

        rl_clear_history();
        terminate = true;
        return EX_OK;
    };

    // start input thread.
    // a detached thread will be terminated by its destructor as soon as the thread object is out of scope
    // (end of function main)
    std::thread input_thread(input_thread_func);
    input_thread.detach();

    while (!terminate) {
        if (use_mb_client_pid) {
            // check if modbus client is still alive
            int tmp = kill(mb_client_pid, 0);
            if (tmp == -1) {
                if (errno == ESRCH) {
                    std::cerr << "Modbus client (pid=" << mb_client_pid << ") no longer alive.\n" << std::flush;
                } else {
                    perror("failed to send signal to the Modbus client");
                }
                terminate = true;
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // NOLINT
    }

    std::lock_guard<std::mutex> guard(m);  // wait until the thread is not within a critical section
    if (INTERACTIVE) std::cerr << "\nTerminating ..." << std::endl;  // NOLINT
}
