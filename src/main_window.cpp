#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING

#include "../include/scanner.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <ctime> 
#include <iostream>
// 2. Подключите правильный заголовочный файл
#include <filesystem>

GLuint webcam_tex = 0;    // Идентификатор текстуры веб-камеры
GLuint scan_tex = 0;       // Идентификатор текстуры скана
cv::Mat current_scan;      // Текущий отсканированный документ
std::string save_path = "scans/"; // Путь сохранения по умолчанию
static char path_buf[256] = "scans/"; // Буфер для пути из GUI


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

// Конвертация OpenCV Mat в OpenGL текстуру
void mat_to_texture(const cv::Mat& image, GLuint& tex_id, bool flip = true) {
    if (image.empty()) return;

    // --- Исправлено преобразование цвета ---
    cv::Mat display_image;
    cv::cvtColor(image, display_image, cv::COLOR_BGR2RGB); // OpenCV (BGR) -> OpenGL (RGB)
    
    if (flip) cv::flip(display_image, display_image, 0);

    if (tex_id == 0) {
        // Гарантированная инициализация текстуры
        glGenTextures(1, &tex_id);
        glBindTexture(GL_TEXTURE_2D, tex_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 
                    display_image.cols, display_image.rows, 
                    0, GL_RGB, GL_UNSIGNED_BYTE, display_image.data);
    } else {
        // Оптимизация: повторное использование существующей текстуры 
        update_texture(tex_id, image);
    }
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

    // Настройка стилей ImGui
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Фон #FFFFFF
    style.Colors[ImGuiCol_Button] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f); // Кнопки #D9D9D9
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.65f, 0.65f, 0.65f, 1.0f);
    style.Colors[ImGuiCol_Text] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f); // Текст

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
        update_texture(webcam_tex, frame);

        // Окно интерфейса
        ImGui::Begin("Document Scanner", nullptr, 
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | 
                    ImGuiWindowFlags_NoScrollbar);
        
        // Веб-камера (60% ширины окна)
        ImGui::BeginChild("Webcam", ImVec2(ImGui::GetWindowWidth() * 0.6f, 0), true);
        ImGui::Text("Webcam Feed");
        ImGui::Image(
            (ImTextureID)(webcam_tex),
            ImVec2(ImGui::GetContentRegionAvail().x, 480), 
            ImVec2(0, 1), ImVec2(1, 0)
        );
        ImGui::EndChild();

        // Панель управления (40% ширины окна)
        ImGui::SameLine();
        ImGui::BeginChild("Controls", ImVec2(0, 0), true);
        
        // Кнопки   
        ImGui::Dummy(ImVec2(0, 20));
        if(ImGui::Button("Capture", ImVec2(-1, 40))) {
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
        

        ImGui::Dummy(ImVec2(0, 20));
        ImGui::InputText("Save Path", path_buf, IM_ARRAYSIZE(path_buf));
        save_path = std::string(path_buf); // Синхронизация пути

        ImGui::Dummy(ImVec2(0, 20));
        if (ImGui::Button("Save Scan", ImVec2(-1, 40)) && !current_scan.empty()) {
            // Нормализация пути
            std::filesystem::path dir_path = std::filesystem::path(save_path).lexically_normal();
            
            if (!std::filesystem::exists(dir_path)) {
                std::filesystem::create_directories(dir_path);
            }

            std::filesystem::path filename = dir_path / ("scan_" + std::to_string(time(nullptr)) + ".jpg");
            
            if (!cv::imwrite(filename.string(), current_scan)) {
                std::cerr << "Save failed: " << filename << std::endl;
            }
        }

        // Превью скана
        ImGui::Dummy(ImVec2(0, 20));
        ImGui::Text("Scanned Document");
        ImGui::Image(
            (ImTextureID)(scan_tex),
            ImVec2(400, 600), 
            ImVec2(0, 0), ImVec2(1, 1)
        );

        ImGui::EndChild();
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
