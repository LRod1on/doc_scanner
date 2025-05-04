#include "MainWindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QImage>
#include <QPixmap>
#include <opencv2/videoio.hpp>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupWebcam();
    
    // Connect UI signals
    connect(ui->captureButton, &QPushButton::clicked, this, &MainWindow::captureFrame);
    connect(ui->saveButton, &QPushButton::clicked, this, &MainWindow::saveScan);
    connect(ui->directoryButton, &QPushButton::clicked, this, &MainWindow::selectOutputDirectory);
}

MainWindow::~MainWindow()
{
    delete ui;
    webcam.release();
}

void MainWindow::setupWebcam()
{
    webcam.open(0);
    if(!webcam.isOpened()) {
        QMessageBox::critical(this, "Camera Error", "Could not access webcam");
        return;
    }
    
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateWebcamFeed);
    timer->start(33); // ~30 FPS
}

void MainWindow::updateWebcamFeed()
{
    webcam >> currentFrame;
    if(currentFrame.empty()) return;
    
    // Convert color space from BGR to RGB
    cv::cvtColor(currentFrame, currentFrame, cv::COLOR_BGR2RGB);
    updateDisplay(currentFrame);
}

void MainWindow::updateDisplay(cv::Mat& frame)
{
    QImage qimg(frame.data,
                frame.cols,
                frame.rows,
                frame.step,
                QImage::Format_RGB888);
    
    QPixmap pixmap = QPixmap::fromImage(qimg);
    ui->videoLabel->setPixmap(pixmap.scaled(ui->videoLabel->size(),
                                           Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation));
}

void MainWindow::captureFrame()
{
    if(currentFrame.empty()) return;
    
    // Convert back to BGR for processing
    cv::Mat processingFrame;
    cv::cvtColor(currentFrame, processingFrame, cv::COLOR_RGB2BGR);
    
    scannedDocument = scanner.scanDocument(processingFrame);
    
    if(!scannedDocument.empty()) {
        cv::cvtColor(scannedDocument, scannedDocument, cv::COLOR_BGR2RGB);
        updateDisplay(scannedDocument);
        ui->statusBar->showMessage("Document captured!", 3000);
    } else {
        QMessageBox::warning(this, "Capture Error", "No document detected");
    }
}

void MainWindow::saveScan()
{
    if(scannedDocument.empty()) {
        QMessageBox::warning(this, "Save Error", "No document to save");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this,
        "Save Document Scan",
        outputPath + "/scan.jpg",
        "JPEG Images (*.jpg *.jpeg)");
    
    if(!fileName.isEmpty()) {
        cv::Mat saveImage;
        cv::cvtColor(scannedDocument, saveImage, cv::COLOR_RGB2BGR);
        if(cv::imwrite(fileName.toStdString(), saveImage)) {
            ui->statusBar->showMessage("Saved to: " + fileName, 5000);
        } else {
            QMessageBox::critical(this, "Save Error", "Failed to save image");
        }
    }
}

void MainWindow::selectOutputDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        "Select Save Directory",
        QDir::homePath(),
        QFileDialog::ShowDirsOnly);
    
    if(!dir.isEmpty()) {
        outputPath = dir;
        ui->statusBar->showMessage("Save directory: " + dir, 3000);
    }
}