#include "scanner.hpp"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <vector>

DocumentScanner::DocumentScanner(float width, float height) : output_w(width), output_h(height) {}

cv::Mat DocumentScanner::scanDocument(cv::Mat& input) {
    if(input.empty()) return cv::Mat();
    
    // preprocess the image
    cv::Mat processed = preprocessImg(input);
    
    // find and order corners
    std::vector<cv::Point> corners = reorderCorners(findCorners(processed));

    if(corners.size() != 4) return cv::Mat();
    
    // Apply perspective warp and return result
    return applyWarp(input, corners);
}


cv::Mat DocumentScanner::preprocessImg(cv::Mat& img){
    cv::Mat gray_img, canny_img, blur_img, dilation_img;

    cv::cvtColor(img, gray_img, cv::COLOR_BGR2GRAY);

    cv::GaussianBlur(gray_img, blur_img, cv::Size(3, 3), 3, 0);

    cv::Canny(blur_img, canny_img, 50, 150);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::dilate(canny_img, dilation_img, kernel);

    return dilation_img;
}

std::vector<cv::Point> DocumentScanner::findCorners(cv::Mat& img){
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    // ищем контуры
    cv::findContours(img, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);    

    std::vector<std::vector<cv::Point>> poly_con(contours.size());
    std::vector<cv::Rect> bound_rectangle(contours.size());

    std::vector<cv::Point> biggest_con;
    float max_area = 0.0f;

    // фильтруем объекты минимального размера 
    for (int i = 0; i < contours.size(); i++){
        float area = cv::contourArea(contours[i]);

        if (area > 1000){
            float perimetr = cv::arcLength(contours[i], true);

            // считаем количество curves в задетектированном шейпе
            cv::approxPolyDP(contours[i], poly_con[i], 0.02 * perimetr, true);

            // находим наибольшие квадратные контуры
            if (area > max_area && poly_con[i].size() == 4){
                max_area = area;
                biggest_con = {poly_con[i][0], poly_con[i][1], poly_con[i][2], poly_con[i][3]};
            }            
        }
    }

    return biggest_con;
}

std::vector<cv::Point> DocumentScanner::reorderCorners(std::vector<cv::Point> original_points){
   
    std::vector<cv::Point> new_points;
    std::vector<int> sum_points, sub_points;
    
    for(const auto& point : original_points) {
        sum_points.push_back(point.x + point.y);
        sub_points.push_back(point.x - point.y);
    }

    new_points.push_back(original_points[std::min_element(sum_points.begin(), sum_points.end()) - sum_points.begin()]);
    new_points.push_back(original_points[std::max_element(sub_points.begin(), sub_points.end()) - sub_points.begin()]);
    new_points.push_back(original_points[std::min_element(sub_points.begin(), sub_points.end()) - sub_points.begin()]);
    new_points.push_back(original_points[std::max_element(sum_points.begin(), sum_points.end()) - sum_points.begin()]);

    return new_points;
}  

cv::Mat DocumentScanner::applyWarp(cv::Mat& img, std::vector<cv::Point> points){
    // warping img
    cv::Mat warp_img;

    cv::Point2f src[4] = {points[0], points[1], points[2], points[3]};
    cv::Point2f final[4] = {{0.0f, 0.0f}, {output_w, 0.0f}, {0.0f, output_h}, {output_w, output_h}};

    cv::Mat matrix = cv::getPerspectiveTransform(src, final);
    cv::warpPerspective(img, warp_img, matrix, cv::Point(output_w, output_h));

    // crop
    int crop_val = 10;
    cv::Rect roi(crop_val, crop_val, output_w - crop_val * 2, output_h - crop_val * 2);

    return warp_img(roi);
}

void DocumentScanner::setOutputSize(float width, float height) {
    output_w = width;
    output_h = height;
}