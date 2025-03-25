#include <opencv2/opencv.hpp>
#include <xmmintrin.h>
#include <iostream>
#include <chrono>


using namespace cv;
using namespace std;

void motionDetectionSerial(const Mat &prevFrame, const Mat &currFrame, Mat &motionFrame) {
    for (int i = 0; i < prevFrame.rows; ++i) {
        for (int j = 0; j < prevFrame.cols; ++j) {
            motionFrame.at<uchar>(i, j) = abs(currFrame.at<uchar>(i, j) - prevFrame.at<uchar>(i, j));
        }
    }
}

void motionDetectionParallel(const Mat &prevFrame, const Mat &currFrame, Mat &motionFrame) {
    int width = prevFrame.cols;
    int height = prevFrame.rows;

    for (int i = 0; i < height; ++i) {
        uchar *pPrev = (uchar*)prevFrame.ptr(i);
        uchar *pCurr = (uchar*)currFrame.ptr(i);
        uchar *pMotion = (uchar*)motionFrame.ptr(i);
        int j = 0;
        for (; j <= width - 16; j += 16) {
            __m128i prevPixels = _mm_loadu_si128((__m128i*)&pPrev[j]);
            __m128i currPixels = _mm_loadu_si128((__m128i*)&pCurr[j]);
            __m128i motionPixels = _mm_abs_epi8(_mm_sub_epi8(currPixels, prevPixels));
            _mm_storeu_si128((__m128i*)&pMotion[j], motionPixels);
        }
        for (; j < width; ++j) {
            pMotion[j] = abs(pCurr[j] - pPrev[j]);
        }
    }
}

int main() {
    // Suppress OpenCV informational logs
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);
    string videoPath = "D:/term7/parallel/ca/ca1/assets/Q4/Q4.mp4";
    VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        cerr << "Error: Could not open video"<< endl;
        return -1;
    }
    Mat prevFrame, currFrame, motionFrame;
    cap >> prevFrame;
    cvtColor(prevFrame, prevFrame, COLOR_BGR2GRAY);
    motionFrame = Mat::zeros(prevFrame.size(), prevFrame.type());

    VideoWriter output("motion_output.mp4", VideoWriter::fourcc('m', 'p', '4', 'v'), 10, prevFrame.size(), false);

    double totalSerialTime = 0, totalParallelTime = 0;
    int frameCount = 0;
    while (true) {
        cap >> currFrame;
        if (currFrame.empty()) break;

        //serial
        cvtColor(currFrame, currFrame, COLOR_BGR2GRAY);
        auto startSerial = chrono::high_resolution_clock::now();
        motionDetectionSerial(prevFrame, currFrame, motionFrame);
        auto endSerial = chrono::high_resolution_clock::now();
        totalSerialTime += chrono::duration<double>(endSerial - startSerial).count();

        //parallel
        auto startParallel = chrono::high_resolution_clock::now();
        motionDetectionParallel(prevFrame, currFrame, motionFrame);
        auto endParallel = chrono::high_resolution_clock::now();
        totalParallelTime += chrono::duration<double>(endParallel - startParallel).count();

        output.write(motionFrame);
        prevFrame = currFrame.clone();
        frameCount++;
    }
    cap.release();
    output.release();
    double avgSpeedup = totalSerialTime / totalParallelTime;
    cout << "Average Speedup: " << avgSpeedup << endl;
    cout << "Motion detection complete. Output saved to 'motion_output.mp4'." << endl;

    return 0;
}
