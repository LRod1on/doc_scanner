#ifndef SCANNER_HPP
#define SCANNER_HPP

#pragma once
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

class DocumentScanner {
public:
    DocumentScanner() : output_w(420.0f), output_h(596.0f) {};
    explicit DocumentScanner(float width, float height);
    ~DocumentScanner() = default;
    cv::Mat scanDocument(cv::Mat& input);
    void setOutputSize(float width, float height);

private:
    float output_w = 420;
    float output_h = 596;

    cv::Mat preprocessImg(cv::Mat& img);
    std::vector<cv::Point> findCorners(cv::Mat& img);
    std::vector<cv::Point> reorderCorners(std::vector<cv::Point> corners);
    cv::Mat applyWarp(cv::Mat& img, std::vector<cv::Point> corners);
};

#endif