# ESP32-PQSignature-Test

This repository contains the source code for experiments evaluating the performance of post-quantum digital signature algorithms on a LoRa ESP32 microcontroller.
## Overview

The main objectives of the experiments were:
- Benchmarking signing, verification, and key generation latencies for multiple PQC algorithms at different NIST security levels.
- Measuring heap usage
- Assessing real-world communication overhead introduced by larger digital signatures when sent over a wireless connection.

## Algorithms Evaluated

The following PQC algorithms were tested:
- CRYSTALS-Dilithium
- Falcon
- SPHINCS+

## Hardware Platform
All tests were performed on a LoRa ESP32 microcontroller, which features:
- Xtensa dual-core 32-bit LX7 processor
- 240 MHz clock speed
- 512 KB SRAM, 2 MB PSRAM
- 4 MB Flash

## Getting Started

1. **Prerequisites**:
   - ESP-IDF installed and set up on your development machine.
   - Required cryptographic algorithm implementations from the PQClean repository.

2. **Building and Flashing**:
   - Clone the repository.
   - Configure your Wi-Fi credentials in `main/main.c`.
   - Build and flash the firmware using ESP-IDF.

3. **Running Tests**:
   - Once the device is powered on, it will generate keys, perform signing/verification tests, and measure memory usage.
   - For Wi-Fi tests, connect the device to your network and send signing requests via TCP.

4. **Results**:
   - Output will include timing metrics (latency) and memory usage.
   - Modify the code as needed to test different scenarios or adjust parameter levels.

## License

This project is open-source and licensed under the MIT License.
