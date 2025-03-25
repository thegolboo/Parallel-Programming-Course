#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <immintrin.h> // SIMD SSE instructions

std::string runLengthEncodeSerial(const std::string& input) {
    std::ostringstream encoded;
    int n = input.length();

    for (int i = 0; i < n; ++i) {
        int count = 1;
        while (i < n - 1 && input[i] == input[i + 1]) {
            ++count;
            ++i;
        }
        encoded << input[i] << count;
    }

    return encoded.str();
}

std::string runLengthEncodeSIMD(const std::string& input) {
    int n = input.length();
    if (n == 0) return "";

    std::ostringstream encoded;
    int i = 0;

    while (i <= n - 16) { 
        __m128i chars = _mm_loadu_si128((__m128i*) & input[i]);
        __m128i next_chars = _mm_loadu_si128((__m128i*) & input[i + 1]);
        __m128i result = _mm_cmpeq_epi8(chars, next_chars);
        int mask = _mm_movemask_epi8(result);

        int count = 1;
        int j = 0;
        while (j < 16 && ((mask >> j) & 1)) {
            ++count;
            ++j;
        }

        encoded << input[i] << count;
        i += count;
    }

    while (i < n) {
        int count = 1;
        while (i < n - 1 && input[i] == input[i + 1]) {
            ++count;
            ++i;
        }
        encoded << input[i] << count;
        ++i;
    }

    return encoded.str();
}

float calculateCompressionRatio(const std::string& original, const std::string& compressed) {
    return static_cast<float>(original.size()) / compressed.size();
}

int main() {
    std::string input;
    std::cout << "Enter a string to compress: ";
    std::cin >> input;

    auto start = std::chrono::high_resolution_clock::now();
    std::string compressedSerial = runLengthEncodeSerial(input);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> serialTime = end - start;

    start = std::chrono::high_resolution_clock::now();
    std::string compressedSIMD = runLengthEncodeSIMD(input);
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> simdTime = end - start;

    float compressionRatioSerial = calculateCompressionRatio(input, compressedSerial);
    float compressionRatioSIMD = calculateCompressionRatio(input, compressedSIMD);

    std::cout << "Original String: " << input << std::endl;
    std::cout << "Compressed String (Serial): " << compressedSerial << std::endl;
    std::cout << "Compressed String (Parallel): " << compressedSIMD << std::endl;
    std::cout << "Compression Ratio (Serial): " << compressionRatioSerial << std::endl;
    std::cout << "Compression Ratio (Parallel): " << compressionRatioSIMD << std::endl;
    std::cout << "Serial Time: " << serialTime.count() << " seconds" << std::endl;
    std::cout << "Parallel Time: " << simdTime.count() << " seconds" << std::endl;
    std::cout << "Speedup (Serial Time / Parallel Time): " << serialTime.count() / simdTime.count() << std::endl;

    return 0;
}
