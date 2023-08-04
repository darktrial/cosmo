#include <QScrollBar>
#include <QSettings>
#include <QFileDialog>
#include <QFile>
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "rtspClientHelper.hh"
extern "C"
{
#include "ffmpeg/include/libavformat/avformat.h"
#include "ffmpeg/include/libavcodec/avcodec.h"
}
#define lengthOfTime    32
#define lengthOfSize    32
#define legnthofStatics 64
AVCodecContext *pCodecCtx = NULL;
rtspPlayer *player=NULL;
typedef struct _timeRecords
{
    long long starttime;
    long long endtime;
    int numberOfFrames;
    int sizeOfFrames;
} timeRecords;
timeRecords tr;

bool isIFrame(AVPacket *packet)
{
    int got_frame;
    if (!pCodecCtx)
        return false;
    AVFrame *frame = av_frame_alloc();

    avcodec_send_packet(pCodecCtx, packet);
    got_frame = avcodec_receive_frame(pCodecCtx, frame);

    bool isIframe = false;
    if (got_frame != AVERROR(EAGAIN) && got_frame != AVERROR_EOF)
    {
        isIframe = frame->pict_type == AV_PICTURE_TYPE_I;
    }
    av_frame_free(&frame);
    return isIframe;
}

long long getCurrentTimeMicroseconds()
{
#ifdef _WIN32
    FILETIME currentTime;
    GetSystemTimePreciseAsFileTime(&currentTime);

    ULARGE_INTEGER uli;
    uli.LowPart = currentTime.dwLowDateTime;
    uli.HighPart = currentTime.dwHighDateTime;

    return uli.QuadPart / 10LL;
#else
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);

    return (long long)currentTime.tv_sec * 1000000LL + currentTime.tv_usec;
#endif
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    QStringList headers;
    headers << "Codec" << "Frame type" << "Frame size"<<"Presentation Time";
    ui->setupUi(this);
    QHeaderView *hheading=ui->tableWidget->horizontalHeader();
    QHeaderView *vheading=ui->tableWidget->verticalHeader();
    hheading->setSectionResizeMode(QHeaderView::Stretch);
    vheading->setVisible(false);
    ui->tableWidget->setColumnCount(4);
    ui->tableWidget->setHorizontalHeaderLabels(headers);

    if (QFile("rtsp_client_def.ini").exists())
    {
        QSettings settings("rtsp_client_def.ini", QSettings::IniFormat);
        QString filename=settings.value("current_config","").toString();
        QSettings rtspSetting(filename,QSettings::IniFormat);
        QString rtspUrl=rtspSetting.value("rtsp_url","").toString();
        QString username=rtspSetting.value("username","").toString();
        QString password=rtspSetting.value("password","").toString();
        ui->urlText->setText(rtspUrl);
        ui->usernameText->setText(username);
        ui->passwordText->setText(password);
    }
    else ui->statusbar->showMessage("config not found");

}

MainWindow::~MainWindow()
{
    delete ui;
}

void addItemToTable(const char *codecName, const char *frameType, const char *frameSize, const char *presetationTime, void *privateData)
{

    Ui::MainWindow *ui=(Ui::MainWindow *)privateData;
    QScrollBar *scrollbar=ui->tableWidget->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
    ui->tableWidget->insertRow(ui->tableWidget->rowCount());
    QTableWidgetItem *codec=new QTableWidgetItem(codecName);
    codec->setTextAlignment(Qt::AlignCenter);
    QTableWidgetItem *frame_type=new QTableWidgetItem(frameType);
    codec->setTextAlignment(Qt::AlignCenter);
    QTableWidgetItem *frame_size=new QTableWidgetItem(frameSize);
    codec->setTextAlignment(Qt::AlignCenter);
    QTableWidgetItem *presentation_time=new QTableWidgetItem(presetationTime);
    codec->setTextAlignment(Qt::AlignCenter);

    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,0,codec);
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,1,frame_type);
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,2,frame_size);
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,3,presentation_time);
    if (ui->tableWidget->rowCount()>150)
        ui->tableWidget->removeRow(1);
}


void onFrameArrival(unsigned char *videoData, const char *codecName, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, void *privateData)
{
    char uSecsStr[lengthOfTime];
    char videoSize[lengthOfSize];
    char statics[legnthofStatics];
    u_int8_t start_code[4] = {0x00, 0x00, 0x00, 0x01};
    u_int8_t *frameData;
    AVPacket packet;
    Ui::MainWindow *ui=(Ui::MainWindow *)privateData;

    snprintf(uSecsStr, lengthOfTime, "%d.%06u",  (int)presentationTime.tv_sec, (unsigned)presentationTime.tv_usec);
    snprintf(videoSize,lengthOfSize,"%d", frameSize);
    if (strcmp(codecName, "JPEG") == 0)
    {

        if (tr.numberOfFrames == 0)
        {
            tr.starttime = getCurrentTimeMicroseconds();
            tr.numberOfFrames++;
            tr.sizeOfFrames = frameSize;
        }
        if (tr.numberOfFrames >= 30)
        {
            tr.endtime = getCurrentTimeMicroseconds();
            float fps= (tr.numberOfFrames * 1000000.0) / (tr.endtime - tr.starttime);
            float bitrate=(tr.sizeOfFrames) / ((tr.endtime - tr.starttime) / 1000000.0) / 1024;
            snprintf(statics,legnthofStatics,"FPS:%.3f        bit rate:%.3f KB/s",fps,bitrate);
            ui->statusbar->showMessage(statics);
            tr.starttime =tr.endtime;
            tr.numberOfFrames=1;
            tr.sizeOfFrames=frameSize;
            tr.numberOfFrames=1;
        }
        tr.numberOfFrames++;
        tr.sizeOfFrames+=frameSize;
        addItemToTable("JPEG","I",videoSize,uSecsStr, privateData);
        return;
    }
    frameData = (u_int8_t *)malloc(frameSize + 4);
    memcpy(frameData, start_code, 4);
    memcpy(frameData + 4, videoData, frameSize);
    av_new_packet(&packet, frameSize + 4);
    packet.data = frameData;
    packet.size = frameSize + 4;
    bool isIframe = isIFrame(&packet);
    if (isIframe)
    {
        /*std::cout << " codec:" << codecName << " I-Frame "
                  << " size:" << frameSize << " bytes "
                  << "presentation time:" << (int)presentationTime.tv_sec << "." << uSecsStr << "\n";*/
        addItemToTable(codecName,"I",videoSize,uSecsStr, privateData);
        if (tr.starttime == 0)
        {
            tr.starttime = getCurrentTimeMicroseconds();
            tr.numberOfFrames++;
            tr.sizeOfFrames = frameSize;
        }
        else
        {
            tr.endtime = getCurrentTimeMicroseconds();
            //std::cout << "FPS:" << (tr.numberOfFrames * 1000000.0) / (tr.endtime - tr.starttime) << "\n";
            //std::cout << "bit rate:" << (tr.sizeOfFrames) / ((tr.endtime - tr.starttime) / 1000000.0) / 1024 << " KB/s \n";
            float fps= (tr.numberOfFrames * 1000000.0) / (tr.endtime - tr.starttime);
            float bitrate=(tr.sizeOfFrames) / ((tr.endtime - tr.starttime) / 1000000.0) / 1024;
            snprintf(statics,legnthofStatics,"FPS:%.3f        bit rate:%.3f KB/s",fps,bitrate);
            ui->statusbar->showMessage(statics);
            tr.starttime =tr.endtime;
            tr.numberOfFrames=1;
            tr.sizeOfFrames=frameSize;
        }
    }
    else
    {
        /*std::cout << " codec:" << codecName << " P-frame "
                  << " size:" << frameSize << " bytes "
                  << " presentation time:" << (int)presentationTime.tv_sec << "." << uSecsStr << "\n";*/
        addItemToTable(codecName,"P",videoSize,uSecsStr, privateData);
        if (tr.starttime != 0)
        {
            tr.numberOfFrames++;
            tr.sizeOfFrames += frameSize;
        }

    }
    free(frameData);
    av_packet_unref(&packet);
}


void onConnectionSetup(char *codecName, void *privateData)
{
    AVCodec *codec;
    if (strcmp(codecName, "H264") == 0)
        codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    else if (strcmp(codecName, "H265") == 0)
        codec = avcodec_find_decoder(AV_CODEC_ID_H265);
    else
        return;

    if (!codec)
    {
        std::cout << "Failed to find the suitable decoder"
                  << "\n";
        return;
    }
    pCodecCtx = avcodec_alloc_context3(codec);
    if (!pCodecCtx)
    {
        std::cout << "Failed to find the suitable context"
                  << "\n";
        return;
    }
    // Open the codec
    if (avcodec_open2(pCodecCtx, codec, NULL) < 0)
    {
        std::cout << "Failed to open the codec"
                  << "\n";
        avcodec_free_context(&pCodecCtx);
        return;
    }
    tr.starttime = 0;
    tr.endtime = 0;
    tr.numberOfFrames = 0;
    tr.sizeOfFrames = 0;
}

void MainWindow::on_startButton_clicked()
{

    bool overTCP=false;
    std::string url=ui->urlText->toPlainText().toStdString();
    std::string username=ui->usernameText->toPlainText().toStdString();
    std::string password=ui->passwordText->toPlainText().toStdString();
    player = new rtspPlayer((void *)ui);
    player->onFrameData = onFrameArrival;
    player->onConnectionSetup = onConnectionSetup;
    if (ui->overTCPCheck->isChecked()) overTCP=true;
    if (player->startRTSP(url.c_str(), overTCP, username.c_str(), password.c_str()) != OK)
    {
        delete player;
        player=NULL;
        ui->statusbar->showMessage("RTSP connection failed");
    }
    else ui->startButton->setEnabled(false);
}


void MainWindow::on_actionSave_triggered()
{
    QString filename=QFileDialog::getSaveFileName(NULL, "Save file", "", "*.config");
    if (filename.contains(".config") == false)
        filename+=".config";
    QSettings settings(filename, QSettings::IniFormat);
    settings.setValue("rtsp_url",ui->urlText->toPlainText());
    settings.setValue("username",ui->usernameText->toPlainText());
    settings.setValue("password",ui->passwordText->toPlainText());
    QSettings defsettings("rtsp_client_def.ini", QSettings::IniFormat);
    defsettings.setValue("current_config",filename);
}


void MainWindow::on_actionOpen_triggered()
{
    QString filename=QFileDialog::getOpenFileName(NULL, "Open file", "", "*.config");
    QSettings rtspSetting(filename,QSettings::IniFormat);
    QString rtspUrl=rtspSetting.value("rtsp_url","").toString();
    QString username=rtspSetting.value("username","").toString();
    QString password=rtspSetting.value("password","").toString();
    ui->urlText->setText(rtspUrl);
    ui->usernameText->setText(username);
    ui->passwordText->setText(password);
}


void MainWindow::on_stopButton_clicked()
{
    player->stopRTSP();
    delete player;
    ui->startButton->setEnabled(true);
}

