#include "../include/scanner.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <ctime> 
#include <iostream>

// Texture ID для отображения изображений
GLuint webcam_tex = 0;
GLuint scan_tex = 0;
cv::Mat current_scan;
std::string save_path = "scans/";

// Конвертация OpenCV Mat в OpenGL текстуру
void mat_to_texture(const cv::Mat& image, GLuint& tex_id, bool flip = true) {
    if (image.empty()) return;

    cv::Mat display_image;
    cv::cvtColor(image, display_image, cv::COLOR_BGR2RGB);
    
    if (flip) cv::flip(display_image, display_image, 0);

    if (tex_id == 0) {
        glGenTextures(1, &tex_id);
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, display_image.cols, display_image.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, display_image.data);
}

// Обновление текстуры
void update_texture(GLuint& tex_id, const cv::Mat& image) {
    if (image.empty() || tex_id == 0) return;
    
    cv::Mat display_image;
    cv::cvtColor(image, display_image, cv::COLOR_BGR2RGB);
    cv::flip(display_image, display_image, 0);

    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 
                   display_image.cols, display_image.rows,
                   GL_RGB, GL_UNSIGNED_BYTE, display_image.data);
}

int main() {

    // Инициализация GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Настройка GLFW для x64
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Создание окна
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Document Scanner", NULL, NULL);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create GLFW window" << std::endl;
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Включение вертикальной синхронизации

    // Инициализация ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    
    // Инициализация ImGui с GLFW и OpenGL
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // Инициализация сканера и веб-камеры
    DocumentScanner scanner;
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open webcam" << std::endl;
        return -1;
    }
    cv::Mat frame;

    // Создание текстур
    cap >> frame;
    mat_to_texture(frame, webcam_tex);
    mat_to_texture(cv::Mat::zeros(600, 400, CV_8UC3), scan_tex);

    /*// Тесты - Вывод ID текстур
    std::cout << "webcam_tex: " << webcam_tex << std::endl;
    std::cout << "scan_tex: " << scan_tex << std::endl;
    std::cout << "current_scan: " << (current_scan.empty() ? "empty" : "not empty") << std::endl;*/

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Начало нового кадра ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Захват кадра с веб-камеры
        cap >> frame;
        if (!frame.empty()) {
            mat_to_texture(frame, webcam_tex);
        }

        // Основное окно
        ImGui::Begin("Document Scanner", nullptr, 
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        
        // Отображение веб-камеры
        ImGui::Text("Webcam Feed");
        ImGui::Image((ImTextureID)webcam_tex, ImVec2(640, 480), ImVec2(0, 1), ImVec2(1, 0));

        // Кнопки
        ImGui::SameLine();
        ImGui::BeginGroup();
        
        // Кнопка захвата
        if (ImGui::Button("Capture", ImVec2(100, 40))) {
            cv::Mat processed;
            cv::cvtColor(frame, processed, cv::COLOR_BGR2RGB);
            cv::imshow("image", processed);
            current_scan = scanner.scanDocument(processed);
            cv::imshow("image2", current_scan);
            std::cout << "current_scan empty: " << current_scan.empty() << std::endl;
            if (!current_scan.empty()) {
                update_texture(scan_tex, current_scan);
            }
        }

        // Сохранение пути
        char buffer[256] = {0};
        strncpy(buffer, save_path.c_str(), sizeof(buffer));
        if (ImGui::InputText("Save Path", buffer, sizeof(buffer))) {
            save_path = std::string(buffer);
        }
        
        // Кнопка сохранения скана
        if (ImGui::Button("Save Scan", ImVec2(100, 40)) && !current_scan.empty()) {
            std::string filename = save_path + "scan_" + std::to_string(time(nullptr)) + ".jpg";
            cv::imwrite(filename, current_scan);
        }

        // Превью скана
        ImGui::Text("Scanned Document");
        ImGui::Image((ImTextureID)scan_tex, ImVec2(400, 600), ImVec2(0, 1), ImVec2(1, 0));
        
        ImGui::EndGroup();
        ImGui::End();

        // Рендеринг
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Очистка
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
