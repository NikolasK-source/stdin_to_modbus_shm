# STDIN to Modbus Shared Memory

This application reads commands from stdin and writes them to the shared memory created by one of the shared memory modbus clients (
[TCP](https://nikolask-source.github.io/modbus_tcp_client_shm/)/
[RTU](https://nikolask-source.github.io/modbus_rtu_client_shm/)).


## Use the Application
The application can be started, after starting one of the modbus clients with standard shared memory prefix, without any further arguments.

If the shared memory name prefix has been changed, it can be adjusted via the argument ```----name-prefix```.

After starting, the application reads commands from stdin. 
There is one command per line.

The application terminates as soon as no more input data can be read.

### Input format
The commands for the application have the following format:
```
register_type:register_address:value[:data_type]
```

```register_type``` specifies into which register the value should be written.
The following values are possible (case is ignored):
- do
- di
- ao
- ai

```register_address``` specifies the address of the register

```value``` specifies the value that is written.  
The representation depends on the type of modbus register and data type.
For hex and octal numbers the same notation as in C/C++ is used.
Some string constants are available.

```data_type``` optionally specifies a data type.  
If no data type is specified, one register is written in host byte order.  
The following data types are possible:
- Float:
  - 32 Bit:
    - f32_abcd, f32_big, **f32b**  
      32-Bit floating point, big endian
    - f32_dcba, f32_little, **f32l**  
      32-Bit floating point, little endian
    - f32_cdab, f32_big_rev, **f32br**  
      32-Bit floating point, big endian, reversed register order
    - f32_badc, f32_little_rev, **f32lr**  
      32-Bit floating point, little endian, reversed register order
  - 64 Bit:
    - f64_abcdefgh, f64_big, **f64b**  
      64-Bit floating point, big endian
    - f64_ghefcdab, f64_little, **f64l**  
      64-Bit floating point, little endian
    - f64_badcfehg, f64_big_rev, **f64br**  
      64-Bit floating point, big endian, reversed register order
    - f64_hgfedcba, f64_little_rev, **f64lr**  
      64-Bit floating point, little endian, reversed register order
- Integer:
  - 8 Bit:
    - Signed:
      - **i8_lo**  
        8-Bit signed integer written to low byte of register.
        The other byte is set to ```0```
      - **i8_hi**  
        8-Bit signed integer written to high byte of register.
        The other byte is set to ```0```
    - Unsigned:
      - **u8_lo**  
        8-Bit unsigned integer written to low byte of register.
        The other byte is set to ```0```
      - **u8_hi**  
        8-Bit unsigned integer written to high byte of register.
        The other byte is set to ```0```
  - 16 Bit:
    - Signed:
      - i16_ab, i16_big, **i16b**  
        16-Bit signed integer, big endian
      - i16_ba, i16_little, **i16l**  
        16-Bit signed integer, little endian
    - Unsigned:
      - u16_ab, u16_big, **u16b**  
        16-Bit unsigned integer, big endian
      - u16_ba, u16_little, **u16l**  
        16-Bit unsigned integer, little endian
  - 32 Bit:
    - Signed:
      - i32_abcd, i32_big, **i32b**  
        32-Bit signed integer, big endian
      - i32_dcba, i32_little, **i32l**  
        32-Bit signed integer, little endian
      - i32_cdab, i32_big_rev, **i32br**  
        32-Bit signed integer, big endian, reversed register order
      - i32_badc, i32_little_rev, **i32lr**  
        32-Bit signed integer, little endian, reversed register order
    - Unsigned:
      - u32_abcd, u32_big, **u32b**  
        32-Bit unsigned integer, big endian
      - u32_dcba, u32_little, **u32l**  
        32-Bit unsigned integer, little endian
      - u32_cdab, u32_big_rev, **u32br**  
        32-Bit unsigned integer, big endian, reversed register order
      - u32_badc, u32_little_rev, **u32lr**  
        32-Bit unsigned integer, little endian, reversed register order
  - 64 Bit:
    - Signed:
      - i64_abcdefgh, i64_big, **i64b**  
        64-Bit signed integer, big endian
      - i64_hgfedcba, i64_little, **i64l**  
        64-Bit signed integer, little endian
      - i64_ghefcdab, i64_big_rev, **i64br**  
        64-Bit signed integer, big endian, reversed register order
      - i64_badcfehg, i64_little_rev, **i64lr**  
        64-Bit signed integer, little endian, reversed register order
    - Unsigned:
      - u64_abcdefgh, u64_big, **u64b**  
        64-Bit unsigned integer, big endian
      - u64_hgfedcba, u64_little, **u64l**  
        64-Bit unsigned integer, little endian
      - u64_ghefcdab, u64_big_rev, **u64br**  
        64-Bit unsigned integer, big endian, reversed register order
      - u64_badcfehg, u64_little_rev, **u64lr**  
        64-Bit unsigned integer, little endian, reversed register order

> **Note**:  
The endianness refers to the layout of the data in the shared memory and may differ from the Modbus Server's 
definition of the endianness.

### Command Passthrough
By using the option ```--passthrough```, all valid inputs are written to stdout.
By additionally enabling the option ```--bash```, the output is created as a bash script that reproduces the inputs
(including the timing).

## Install

### Using the Arch User Repository (recommended for Arch based Linux distributions)
The application is available as [stdin-to-modbus-shm](https://aur.archlinux.org/packages/stdin-to-modbus-shm) in the [Arch User Repository](https://aur.archlinux.org/).
See the [Arch Wiki](https://wiki.archlinux.org/title/Arch_User_Repository) for information about how to install AUR packages.


### Using the Modbus Collection Flatpak Package: Shared Memory Modbus (recommended)
[SHM-Modbus](https://nikolask-source.github.io/SHM_Modbus/) is a collection of the shared memory modbus tools.
It is available as flatpak and published on flathub as ```network.koesling.shm-modbs```.


### Using the Standalone Flatpak package
The flatpak package can be installed via the .flatpak file.
This can be downloaded from the GitHub [projects release page](https://github.com/NikolasK-source/stdin_to_modbus_shm/releases):

```
flatpak install --user stdin-to-modbus-shm.flatpak
```

The application is then executed as follows:
```
flatpak run network.koesling.stdin-to-modbus-shm
```

To enable calling with ```stdin-to-modbus-shm``` [this script](https://gist.github.com/NikolasK-source/3ee7d76bf59551dd09dac892d32f1ea0) can be used.
In order to be executable everywhere, the path in which the script is placed must be in the ```PATH``` environment variable.


### Build from Source

The following packages are required for building the application:
- cmake
- clang or gcc

Use the following commands to build the application:
```
git clone --recursive https://github.com/NikolasK-source/stdin_to_modbus_shm.git
cd stdin_to_modbus_shm
mkdir build
cmake -B build . -DCMAKE_BUILD_TYPE=Release -DCLANG_FORMAT=OFF -DCOMPILER_WARNINGS=OFF
cmake --build build
```

The binary is located in the build directory.


## Links to related projects

### General Shared Memory Tools
- [Shared Memory Dump](https://nikolask-source.github.io/dump_shm/)
- [Shared Memory Write](https://nikolask-source.github.io/write_shm/)
- [Shared Memory Random](https://nikolask-source.github.io/shared_mem_random/)

### Modbus Clients
- [RTU](https://nikolask-source.github.io/modbus_rtu_client_shm/)
- [TCP](https://nikolask-source.github.io/modbus_tcp_client_shm/)

### Modbus Shared Memory Tools
- [STDIN to Modbus](https://nikolask-source.github.io/stdin_to_modbus_shm/)
- [Float converter](https://nikolask-source.github.io/modbus_conv_float/)


## License

MIT License

Copyright (c) 2021-2022 Nikolas Koesling

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

