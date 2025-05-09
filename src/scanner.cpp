#include "../include/scanner.hpp"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <vector>
#include <iostream>

DocumentScanner::DocumentScanner(float width, float height) : output_w(width), output_h(height) {}

cv::Mat DocumentScanner::scanDocument(cv::Mat& input) {
    if(input.empty()) {
        std::cerr << "Error: Input image is empty. Scan not possible." << std::endl;
        return cv::Mat();
    }

    // Preprocess the image
    cv::Mat processed = preprocessImg(input);
    std::cout << "Preprocess: " << (processed.empty() ? "failed" : "success") << std::endl;

    // Find and order corners
    std::vector<cv::Point> corners = reorderCorners(findCorners(processed));
    std::cout << "Found " << corners.size() << " corners." << std::endl;
    assert(corners.size() == 4 && "Corners must be 4 for perspective projection.");

    // Apply perspective warp and return result
    return applyWarp(input, corners);
}

cv::Mat DocumentScanner::preprocessImg(cv::Mat& img) {
    cv::Mat gray_img, canny_img, blur_img, dilation_img;

    // Convert to grayscale
    cv::cvtColor(img, gray_img, cv::COLOR_BGR2GRAY);
    std::cout << "Gray image created." << std::endl;

    cv::GaussianBlur(gray_img, blur_img, cv::Size(3, 3), 3, 0);
    std::cout << "Blurred image created." << std::endl;

    cv::Canny(blur_img, canny_img, 50, 150);
    std::cout << "Canny image created." << std::endl;

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::dilate(canny_img, dilation_img, kernel);
    std::cout << "Dilated image created." << std::endl;

    return dilation_img;
}

std::vector<cv::Point> DocumentScanner::findCorners(cv::Mat& img) {
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    // Ищем контуры
    cv::findContours(img, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);    

    std::vector<std::vector<cv::Point>> poly_con(contours.size());
    std::vector<cv::Rect> bound_rectangle(contours.size());

    std::vector<cv::Point> biggest_con;
    float max_area = 0.0f;

    // фильтруем объекты минимального размера 
    for (int i = 0; i < contours.size(); i++) {
        float area = cv::contourArea(contours[i]);

        if (area > 1000) {
            float perimetr = cv::arcLength(contours[i], true);

            // считаем количество curves в задетектированном шейпе
            cv::approxPolyDP(contours[i], poly_con[i], 0.02 * perimetr, true);

            // находим наибольшие квадратные контуры
            if (area > max_area && poly_con[i].size() == 4) {
                max_area = area;
                biggest_con = {poly_con[i][0], poly_con[i][1], poly_con[i][2], poly_con[i][3]};
            }
        }
    }

    std::cout << "Found " << biggest_con.size() << " corners." << std::endl;
    assert(biggest_con.size() == 4 && "Corners must be 4 for document scan.");
    return biggest_con;
}

std::vector<cv::Point> DocumentScanner::reorderCorners(std::vector<cv::Point> original_points) {
    std::vector<cv::Point> new_points;
    std::vector<int> sum_points, sub_points;

    for(const auto& point : original_points) {
        sum_points.push_back(point.x + point.y);
        sub_points.push_back(point.x - point.y);
    }

    std::cout << "ReorderCorners: original_points size = " << original_points.size() << std::endl;

    if (original_points.size() == 4) {
        new_points.push_back(original_points[std::min_element(sum_points.begin(), sum_points.end()) - sum_points.begin()]);
        new_points.push_back(original_points[std::max_element(sub_points.begin(), sub_points.end()) - sub_points.begin()]);
        new_points.push_back(original_points[std::min_element(sub_points.begin(), sub_points.end()) - sub_points.begin()]);
        new_points.push_back(original_points[std::max_element(sum_points.begin(), sum_points.end()) - sum_points.begin()]);
        std::cout << "Reordered corners: " << new_points.size() << std::endl;
        assert(new_points.size() == 4 && "Reordered corners must be 4 for document scan.");
    } else {
        std::cerr << "ReorderCorners: expected 4 points, got " << original_points.size() << std::endl;
    }

    return new_points;
}  

cv::Mat DocumentScanner::applyWarp(cv::Mat& img, std::vector<cv::Point> points) {
    assert(points.size() == 4 && "Points must be 4 for perspective projection.");

    cv::Mat warp_img;

    cv::Point2f src[4] = {points[0], points[1], points[2], points[3]};
    cv::Point2f final[4] = {{0.0f, 0.0f}, {output_w, 0.0f}, {0.0f, output_h}, {output_w, output_h}};

    cv::Mat matrix = cv::getPerspectiveTransform(src, final);
    cv::warpPerspective(img, warp_img, matrix, cv::Size(output_w, output_h));
    std::cout << "Warp image created with size (" << warp_img.cols << ", " << warp_img.rows << ")." << std::endl;

    // crop
    int crop_val = 10;
    cv::Rect roi(crop_val, crop_val, output_w - crop_val * 2, output_h - crop_val * 2);

    std::cout << "ROI: " << roi << std::endl;

    if (roi.width > 0 && roi.height > 0) {
        warp_img = warp_img(roi);
    } else {
        std::cerr << "ROI is invalid. Document not properly detected." << std::endl;
    }

    assert(!warp_img.empty() && "Warp result is empty. No valid document detected.");

    return warp_img;
}

void DocumentScanner::setOutputSize(float width, float height) {
    output_w = width;
    output_h = height;
}
