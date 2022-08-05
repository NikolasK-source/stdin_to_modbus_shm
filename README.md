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
stdin_to_modbus_shm [OPTION...]

  -n, --name-prefix arg   name prefix of the shared memory objects (default: modbus_)
      --address-base arg  Numerical base (radix) that is used for the addresses (see 
                          https://en.cppreference.com/w/cpp/string/basic_string/stoul) (default: 0)
      --value-base arg    Numerical base (radix) that is used for the values (see 
                          https://en.cppreference.com/w/cpp/string/basic_string/stoul) (default: 0)
  -h, --help              print usage


Data input format: reg_type:address:value
    reg_type: modbus register type:                         [do|di|ao|ai]
    address : address of the target register:               [0-65535]
    value   : value that is written to the target register: [0-65535]
              For the registers do and di all numerical values different from 0 are interpreted as 1.
```

## Libraries
This application uses the following libraries:
- cxxopts by jarro2783 (https://github.com/jarro2783/cxxopts)
- libmodbus by Stéphane Raimbault (https://github.com/stephane/libmodbus)
