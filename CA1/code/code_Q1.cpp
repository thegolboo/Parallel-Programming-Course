#include <opencv2/opencv.hpp>
#include <xmmintrin.h>
#include <iostream>
#include <chrono>

void blendSerial(cv::Mat &image, const cv::Mat &logo) {
    for (int y = 0; y < logo.rows; y++) {
        for (int x = 0; x < logo.cols; x++) {
            for (int c = 0; c < 3; c++) {
                image.at<cv::Vec3b>(y, x)[c] = 
                    cv::saturate_cast<uchar>(image.at<cv::Vec3b>(y, x)[c] + 0.625 * logo.at<cv::Vec3b>(y, x)[c]);
            }
        }
    }
}

void blendParallel(cv::Mat &image, const cv::Mat &logo) {
    const float blendFactor = 0.625f;
    __m128 factor = _mm_set1_ps(blendFactor);

    for (int y = 0; y < logo.rows; y++) {
        uchar* imgRow = image.ptr<uchar>(y);
        const uchar* logoRow = logo.ptr<uchar>(y);

        for (int x = 0; x < logo.cols * 3; x += 4) {
            __m128 imgPixels = _mm_cvtepi32_ps(_mm_cvtepu8_epi32(_mm_loadu_si32(&imgRow[x])));
            __m128 logoPixels = _mm_cvtepi32_ps(_mm_cvtepu8_epi32(_mm_loadu_si32(&logoRow[x])));
            __m128 blendedPixels = _mm_add_ps(imgPixels, _mm_mul_ps(logoPixels, factor));
            __m128i result = _mm_cvtps_epi32(blendedPixels);
            result = _mm_packus_epi32(result, result);
            result = _mm_packus_epi16(result, result);
            _mm_storeu_si32(&imgRow[x], result);
        }
    }
}

int main() {
    cv::Mat image = cv::imread("D:/term7/parallel/ca/ca1/assets/Q1/front2.png");
    cv::Mat logo = cv::imread("D:/term7/parallel/ca/ca1/assets/Q1/logo3.png");

    if (image.empty() || logo.empty()) {
        std::cerr << "Error loading images" << std::endl;
        return -1;
    }
    if (logo.cols > image.cols || logo.rows > image.rows) {
        cv::resize(logo, logo, cv::Size(image.cols, image.rows));
    }
    cv::Mat serialImage = image.clone();
    cv::Mat parallelImage = image.clone();

    //serial blending
    auto start = std::chrono::high_resolution_clock::now();
    blendSerial(serialImage, logo);
    auto end = std::chrono::high_resolution_clock::now();
    double serialTime = std::chrono::duration<double, std::milli>(end - start).count();
    
    //parallel blending
    start = std::chrono::high_resolution_clock::now();
    blendParallel(parallelImage, logo);
    end = std::chrono::high_resolution_clock::now();
    double parallelTime = std::chrono::duration<double, std::milli>(end - start).count();

    //speedup
    std::cout << "Serial Time: " << serialTime << " ms" << std::endl;
    std::cout << "Parallel Time: " << parallelTime << " ms" << std::endl;
    std::cout << "Speedup: " << serialTime / parallelTime << std::endl;

    cv::imwrite("D:/term7/parallel/ca/ca1/assets/Q1/blended_image_serial.png", serialImage);
    cv::imwrite("D:/term7/parallel/ca/ca1/assets/Q1/blended_image_parallel.png", parallelImage);

    return 0;
}
