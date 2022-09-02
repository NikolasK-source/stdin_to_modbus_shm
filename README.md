# STDIN to modbus shared memory

Read instructions from stdin and write them to a modbus shared memory.

## Build
```
git submodule init
git submodule update
mkdir build
cd build
cmake .. -DCMAKE_CXX_COMPILER=$(which clang++) -DCMAKE_BUILD_TYPE=Release -DCLANG_FORMAT=OFF -DCOMPILER_WARNINGS=OFF
cmake --build .
```

As an alternative to the ```git submodule``` commands, the ```--recursive``` option can be used with ```git clone```.

## Use
```
stdin-to-modbus-shm [OPTION...]

  -n, --name-prefix arg   name prefix of the shared memory objects (default: modbus_)
      --address-base arg  Numerical base (radix) that is used for the addresses (see 
                          https://en.cppreference.com/w/cpp/string/basic_string/stoul) (default: 0)
      --value-base arg    Numerical base (radix) that is used for the values (see 
                          https://en.cppreference.com/w/cpp/string/basic_string/stoul) (default: 0)
  -p, --passthrough       write passthrough all executed commands to stdout
      --bash              passthrough as bash script. No effect i '--passthrough' is not set
      --valid-hist        add only valid commands to command history
  -h, --help              print usage
  -v, --verbose           print what is written to the registers
      --version           print version information
      --license           show licenses
      --data-types        show list of supported data type identifiers
      --constants         list string constants that can be used as value


Data input format: reg_type:address:value[:data_type]
    reg_type : modbus register type:           [do|di|ao|ai]
    address  : address of the target register: [0-65535]
               The actual maximum register depends on the size of the modbus shared memory.
    value    : value that is written to the target register
               Some string constants are available. The input format depends on the type of register and data type.
               Type 'help constants' for more details 
               For the registers do and di all numerical values different from 0 are interpreted as 1.
    data_type: an optional data type specifier
               If no data type is specified, exactly one register is written in host byte order.
               Type 'help types' to get a list of supported data type identifiers.
```

## Libraries
This application uses the following libraries:
- cxxopts by jarro2783 (https://github.com/jarro2783/cxxopts)
- libmodbus by Stéphane Raimbault (https://github.com/stephane/libmodbus)
