#include <iostream>
#include <cstdint>
#include <iomanip>

// Define a union to represent 128-bit registers
union Register128 {
    uint8_t  u8[16];  // 16 unsigned 8-bit values (bytes)
    int8_t   i8[16];  // 16 signed 8-bit values (bytes)
    uint16_t u16[8];  // 8 unsigned 16-bit values (words)
    int16_t  i16[8];  // 8 signed 16-bit values (words)
    uint32_t u32[4];  // 4 unsigned 32-bit values (double words)
    int32_t  i32[4];  // 4 signed 32-bit values (double words)
    uint64_t u64[2];  // 2 unsigned 64-bit values (quad words)
    int64_t  i64[2];  // 2 signed 64-bit values (quad words)
};

void printRegister(const Register128& reg, const std::string& type) {
    if (type == "u8") {
        std::cout << "Unsigned 8-bit values: ";
        for (int i = 0; i < 16; ++i) {
            std::cout << std::setw(3) << static_cast<int>(reg.u8[i]) << " ";
        }
    } else if (type == "i8") {
        std::cout << "Signed 8-bit values: ";
        for (int i = 0; i < 16; ++i) {
            std::cout << std::setw(3) << static_cast<int>(reg.i8[i]) << " ";
        }
    } else if (type == "u16") {
        std::cout << "Unsigned 16-bit values: ";
        for (int i = 0; i < 8; ++i) {
            std::cout << reg.u16[i] << " ";
        }
    } else if (type == "i16") {
        std::cout << "Signed 16-bit values: ";
        for (int i = 0; i < 8; ++i) {
            std::cout << reg.i16[i] << " ";
        }
    } else if (type == "u32") {
        std::cout << "Unsigned 32-bit values: ";
        for (int i = 0; i < 4; ++i) {
            std::cout << reg.u32[i] << " ";
        }
    } else if (type == "i32") {
        std::cout << "Signed 32-bit values: ";
        for (int i = 0; i < 4; ++i) {
            std::cout << reg.i32[i] << " ";
        }
    } else if (type == "u64") {
        std::cout << "Unsigned 64-bit values: ";
        for (int i = 0; i < 2; ++i) {
            std::cout << reg.u64[i] << " ";
        }
    } else if (type == "i64") {
        std::cout << "Signed 64-bit values: ";
        for (int i = 0; i < 2; ++i) {
            std::cout << reg.i64[i] << " ";
        }
    }
    std::cout << std::endl;
}

int main() {
    // Example of using the Register128 union
    Register128 reg;

    // Set values for demonstration
    for (int i = 0; i < 16; ++i) {
        reg.u8[i] = i;
    }

    // Print values in different formats
    printRegister(reg, "u8");
    printRegister(reg, "i8");
    printRegister(reg, "u16");
    printRegister(reg, "i16");
    printRegister(reg, "u32");
    printRegister(reg, "i32");
    printRegister(reg, "u64");
    printRegister(reg, "i64");

    return 0;
}
