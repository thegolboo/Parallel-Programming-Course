#include <immintrin.h>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>

const int NUM_ELEMENTS = 1 << 20; // 2^20 elements
const float Z_THRESHOLD = 2.5f;

float calculateMean(const std::vector<float>& data) {
    float sum = 0.0f;
    for (float x : data) {
        sum += x;
    }
    return sum / data.size();
}

float calculateStandardDeviation(const std::vector<float>& data, float mean) {
    float sum = 0.0f;
    for (float x : data) {
        sum += (x - mean) * (x - mean);
    }
    return std::sqrt(sum / data.size());
}

int countOutliersSerial(const std::vector<float>& data, float mean, float stddev) {
    int outliers = 0;
    for (float x : data) {
        float z = (x - mean) / stddev;
        if (std::abs(z) > Z_THRESHOLD) {
            outliers++;
        }
    }
    return outliers;
}

int countOutliersParallel(const std::vector<float>& data, float mean, float stddev) {
    int outliers = 0;
    __m128 mean_vec = _mm_set1_ps(mean);
    __m128 stddev_vec = _mm_set1_ps(stddev);
    __m128 threshold_vec = _mm_set1_ps(Z_THRESHOLD);
    __m128 neg_threshold_vec = _mm_set1_ps(-Z_THRESHOLD);

    for (size_t i = 0; i < data.size(); i += 4) {
        __m128 x_vec = _mm_loadu_ps(&data[i]);
        __m128 z_vec = _mm_div_ps(_mm_sub_ps(x_vec, mean_vec), stddev_vec);
        __m128 mask_upper = _mm_cmpgt_ps(z_vec, threshold_vec);
        __m128 mask_lower = _mm_cmplt_ps(z_vec, neg_threshold_vec);
        __m128 mask_outlier = _mm_or_ps(mask_upper, mask_lower);

        outliers += _mm_movemask_ps(mask_outlier);
    }

    return outliers;
}

int main() {
    std::vector<float> data(NUM_ELEMENTS);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    for (float& x : data) {
        x = dist(gen);
    }
    /*std::vector<float> data(NUM_ELEMENTS, 0.5f);
    for (int i = 0; i < NUM_ELEMENTS; i++) {
        if (i % (NUM_ELEMENTS / 100) == 0) {
            data[i] = 3.0f; // Positive outlier
        } else if (i % (NUM_ELEMENTS / 200) == 0) {
            data[i] = -2.0f; // Negative outlier
        } else {
            // Add some small noise around 0.5 to simulate normal distribution
            data[i] += ((rand() % 100) / 1000.0f) - 0.05f;
        }
    }*/

    float mean = calculateMean(data);
    float stddev = calculateStandardDeviation(data, mean);

    //serial
    auto start = std::chrono::high_resolution_clock::now();
    int serialOutliers = countOutliersSerial(data, mean, stddev);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> serialTime = end - start;

    //parallel
    start = std::chrono::high_resolution_clock::now();
    int parallelOutliers = countOutliersParallel(data, mean, stddev);
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> parallelTime = end - start;

    std::cout << "Serial Outliers: " << serialOutliers << "\n";
    std::cout << "Parallel Outliers: " << parallelOutliers << "\n";
    std::cout << "Serial Time: " << serialTime.count() << " seconds\n";
    std::cout << "Parallel Time: " << parallelTime.count() << " seconds\n";
    std::cout << "Speedup: " << serialTime.count() / parallelTime.count() << "\n";

    return 0;
}
