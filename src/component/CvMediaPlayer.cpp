/**
 * Created by Jiajun Chen on 2021/12/28.
 */

#include "CvMediaPlayer.h"

#include <utility>
#include<QUrl>
#include<QDebug>
#include "util/CvImageUtil.h"
#include"controller/MediaController.h"
#include "service/CvBaseService.h"

CvMediaPlayer::CvMediaPlayer(QQuickItem *parent) : QQuickFramebufferObject(parent) {

    frameCache = new std::stack<QImage *>();
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(run()));
    openCamera();

}

QQuickFramebufferObject::Renderer *CvMediaPlayer::createRenderer() const {

    auto renderer = new CvMediaPlayerRenderer();
    renderer->setWindow(window());
    renderer->setFrameCache(frameCache);
    return renderer;

}

void CvMediaPlayer::run() {

    // 【修改】只有当摄像头或视频打开时，才去读取下一帧
    // 图片模式下 capture 没打开，不要调用 updateFrame()，保留原来的 frame
    if (capture.isOpened()) {
        updateFrame();
    }

    // 【新增】如果 frame 是空的，直接返回，防止崩溃
    if (frame.empty()) {
        return;
    }

    CvFrameController::getInstance().processImageByService(frame);
    cvMat2QImage(frame);
    update();
}

void CvMediaPlayer::openPicture(QString path) {

    // 【修改】不要调用 stop()！
    // stop(); 

    // 1. 处理路径
    if (path.startsWith("file:///")) {
        path = path.mid(8);
    }
    // 如果还是不行，取消下面这行的注释，强制写死路径测试
    // path = "D:/OpenCV_test/bb.jpg";

    // 2. 读取图片
    frame = CvImageUtil::loadCvImage(std::move(path));

    if (frame.empty()) {
        qDebug() << "Failed to open picture";
        return;
    }

    // 【新增】关键！释放 capture，告诉 run() 函数现在是图片模式，别去读视频了
    capture.release();

    srcWidth = frame.cols;
    srcHeight = frame.rows;

    // 更新分辨率 (你之前的代码)
    MediaController::getInstance().setImageWidth(srcWidth);
    MediaController::getInstance().setImageHeigth(srcHeight);

    // 3. 【新增】启动定时器，让画面动起来
    if (timer) {
        timer->start(30); // 30毫秒刷新一次
    }

    CvFrameController::getInstance().processImageByService(frame);
    cvMat2QImage(frame);
    update();
}

void CvMediaPlayer::openWebPicture(const QString &path) {

    stop();
    capture.open(path.toStdString());
    if (!capture.isOpened()) {
        qDebug() << "Failed to open video";
        return;
    }
    initializeCam();
    run();
}


/**
 * 差进度条
 * @param path
 */
void CvMediaPlayer::openVideo(QString path) {

    stop();
    path.replace("file:///", "");

    capture.open(path.toStdString());
    if (!capture.isOpened()) {
        qDebug() << "Failed to open video";
        return;
    }
    initializeCam();
    updateFrame();
    cvMat2QImage(frame);
    timer->start(TIMER_TIMEOUT);

}


void CvMediaPlayer::openCamera() {

    stop();
    capture.open(0);

    if (!capture.isOpened()) {
        qDebug() << "Failed to open camera";
        return;
    }

    initializeCam();
    updateFrame();
    cvMat2QImage(frame);
    timer->start(TIMER_TIMEOUT);

}

void CvMediaPlayer::openWebCam(const QString &ip) {

    stop();
    capture.open(ip.toStdString());

    if (!capture.isOpened()) {
        qDebug() << "Failed to open camera";
        return;
    }

    initializeCam();
    updateFrame();
    cvMat2QImage(frame);
    timer->start(TIMER_TIMEOUT);

}

QString CvMediaPlayer::savePicture(QString path) {

    path.replace("file:///", "");

    timer->stop();

    QString uuid = QUuid::createUuid().toString().
            replace("{", "").
            replace("}", "").
            replace("-", "");

    cv::Mat temp;

    //  correct color
    cv::cvtColor(frame, temp, cv::COLOR_BGR2RGB);

    cv::imwrite(path.toStdString() + "/" + uuid.toStdString() + ".jpg", temp);

    qDebug() << "success to save " + uuid + ".jpg";

    timer->start(TIMER_TIMEOUT);

    return uuid + ".jpg";
}


void CvMediaPlayer::stop() {

    timer->stop();
    capture.release();

}

void CvMediaPlayer::initializeCam() {


    // Try to set 1280 X 720
    capture.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    capture.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
    capture.set(cv::CAP_PROP_FPS, 10);

    // Retrieve the width and height of the captured frame
    srcWidth = (int) capture.get(cv::CAP_PROP_FRAME_WIDTH);
    srcHeight = (int) capture.get(cv::CAP_PROP_FRAME_HEIGHT);

    qDebug() << "capture size=" << srcWidth << "x" << srcHeight;

    //Be determined by interrupt timer
    fps = 1000 / TIMER_TIMEOUT;

}

void CvMediaPlayer::updateFrame() {

    if (!capture.isOpened()) {
        qDebug() << "Failed to open camera";
        return;
    }

    capture >> frame;

    if (frame.empty()) {
        qDebug() << "Failed to retrieve frame";
        return;
    }
}

void CvMediaPlayer::cvMat2QImage(const cv::Mat &src) {
    if (src.empty()) return;

    QImage* image;
    cv::Mat tempMat; // 【新增】用临时变量

    // 【修改】根据通道数转换，避免 const 报错
    if (src.channels() == 3) {
        cv::cvtColor(src, tempMat, cv::COLOR_BGR2RGB);
        image = new QImage((const unsigned char*)(tempMat.data), tempMat.cols, tempMat.rows, tempMat.step, QImage::Format_RGB888);
    }
    else if (src.channels() == 1) {
        cv::cvtColor(src, tempMat, cv::COLOR_GRAY2RGB);
        image = new QImage((const unsigned char*)(tempMat.data), tempMat.cols, tempMat.rows, tempMat.step, QImage::Format_RGB888);
    }
    else {
        image = new QImage((const unsigned char*)(src.data), src.cols, src.rows, src.step, QImage::Format_RGBA8888);
    }

    // 缓存逻辑 (保持你原有的)
    if (!frameCache) {
        frameCache = new std::stack<QImage*>(); // 防止未初始化崩溃
    }
    if (!frameCache->empty()) {
        delete frameCache->top();
        frameCache->pop();
    }
    frameCache->push(image);
}







