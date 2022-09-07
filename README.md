# libSNAP

**libSNAP** is an open source C/C++ library that implements most features of the
**Scaleable Node Address Protocol (SNAP)**.

Official documentation: https://lucasjadilo.github.io/libSNAP_docs/

## What is SNAP?

SNAP is a free, simple and generic network protocol designed by the Swedish
company **High Tech Horizon (HTH)** to be used originally with their Power Line
Modem PLM-24.

The goal was to have a scalable protocol that could be easily implemented in
small microcontrollers with very limited computing and memory resources, but
also could be used in larger systems. SNAP supports different packet lengths and
different protocol complexity. Since SNAP is scalable, both simple (cheap) and
sophisticated nodes can communicate with each other in the network.

**Protocol features:**
- Easy to learn, use and implement;
- Free and open network protocol;
- Up to 16.7 million node addresses;
- Up to 24 protocol specific flags;
- Optional ACK/NAK request;
- Optional command mode;
- 8 different error detection methods (Checksum, CRC, FEC, etc);
- Can be used in master/slave and/or peer-to-peer;
- Supports broadcast messages;
- Media independent (RF, RS-485, PLC, IR, Inter IC, etc);
- Works with simplex, half-duplex and full-duplex links;
- Header is scalable from 3 to 12 bytes;
- User-specified number of preamble bytes;
- Works with synchronous and asynchronous communication.

The homepage of the protocol is http://www.hth.com/snap/. Unfortunately, the
website has been down for a long time. Hence, there is no official source online
for the protocol. Fortunately, you can find a copy of the last revision of the
[protocol specification](https://github.com/LucasJadilo/libSNAP/blob/main/doc/snap_v1.00_rev1.04.pdf) in this repository.

## Library features

This library is designed to be portable and fairly lightweight, suitable for
most architectures, including 8-bit microcontrollers. The table below shows the
resulting object file size when compiling the library for the following
architectures: 8-bit AVR, 16-bit MSP430, 32-bit ARM Cortex-M0, and x86_64
(Windows).

| Compiler          | Version | Command line                                      | Object size (bytes) |
|:-----------------:|:-------:|:-------------------------------------------------:|:-------------------:|
| avr-gcc           | 7.3.0   | `avr-gcc -mmcu=atmega328p -c -O2 snap.c`          | 6.976               |
| msp430-elf-gcc    | 9.3.1   | `msp430-elf-gcc -mmcu=msp430fr5969 -c -O2 snap.c` | 8.592               |
| arm-none-eabi-gcc | 10.3.1  | `arm-none-eabi-gcc -mcpu=cortex-m0 -c -O2 snap.c` | 4.016               |
| gcc (MinGW-W64)   | 12.2.0  | `gcc -c -O2 snap.c`                               | 6.358               |

This library supports most of the frame formats described in the protocol
specification, including:
- All the destination address sizes (up to 3 bytes);
- All the source address sizes (up to 3 bytes);
- Any number of protocol specific flags (up to 24 bits);
- All the payload sizes (up to 512 bytes);
- Most of the error detection methods: 8-bit checksum, 8-bit CRC, 16-bit CRC,
32-bit CRC, and a user-defined hash function.

Each CRC function can be individually configured at compilation time to maximize
performance or minimize memory usage. If the macro `SNAP_CRC8_TABLE` is defined
(by command line), the 8-bit CRC algorithm will use a lookup table to speed up
the calculation, thus using more memory. The same logic applies to macros
`SNAP_CRC16_TABLE` and `SNAP_CRC32_TABLE` regarding 16-bit CRC and 32-bit CRC
algorithms, respectively.

There is no hardware-related code (e.g. serial port handling, etc). Frame
transmission and reception through serial ports must be handled outside the
library.

The library provides functions for frame decoding, encapsulation, and
decapsulation. The format of the frame is selected automatically based on the
header bytes.

Despite the efforts to cover all the protocol features, the library has some
limitations:
- Preamble detection is not supported. All bytes received before the sync byte
  will be ignored.
- The **user-specified NDB** (NDB = 15) is not supported. It produces the same
  result as NDB = 0.
- The only error detection methods supported are those that append a hash value
  into the frame, like the checksum/CRC options. The other methods will produce
  the same result as EDM = 0.
  - The method **3 times re-transmission** (EDM = 1) should be handled outside
    the library, probably closer to the hardware layer.
  - The method **FEC** (EDM = 6) is not supported by the library because it is
    not completely defined in the protocol specification.

## How to use

The library consists of only a header file (`src/snap.h`) and a source file
(`src/snap.c`). To use it in your projects, you must include the header file
in the sources that will need it, and compile the library source file along with
your other sources.

There are some code examples in the folder `src/examples/` that demonstrate
the main features of the library:
- Example 1: Frame encapsulation;
- Example 2: Frame decoding and decapsulation;
- Example 3: User-defined hash function (CRC-24/OPENPGP);
- Example 4: Function-like macros.

Every function of the library was tested using the
[Unity](https://github.com/ThrowTheSwitch/Unity) framework. You can find all the
test cases in `test/test_snap.c`.

This project has only one `makefile`, which can be used to build and run all
the examples and unit tests. It is necessary to have **GNU Make** and **GCC**
installed. Upon compilation, the folder `build/` will be created with all the
object files and executables. The available commands are listed below:
- `make all`: Builds all the examples and unit tests;
- `make clean`: Deletes the folder `build/` and everything in it;
- `make test`: Builds and runs the unit tests;
- `make exampleN`: Builds and runs the code example *N* (e.g. `make example1`
runs the code example 1);
- `make doc`: Builds the whole HTML documentation of the library (requires
  **Doxygen** and **Graphviz** installed);
- `make open-doc`: Opens the HTML documentation on the browser;
- `make clean-doc`: Deletes the HTML documentation.
