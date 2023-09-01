#include <QScrollBar>
#include <QSettings>
#include <QFileDialog>
#include <QFile>
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "rtspClientHelper.hh"
#include "xlsxdocument.h"
#include "xlsxchartsheet.h"
#include "xlsxcellrange.h"
#include "xlsxchart.h"
#include "xlsxrichstring.h"
#include "xlsxworkbook.h"
extern "C"
{
#include "ffmpeg/include/libavformat/avformat.h"
#include "ffmpeg/include/libavcodec/avcodec.h"
extern int ffpg_get_minqp();
extern int ffpg_get_maxqp();
extern double ffpg_get_avgqp();
}
#define COSMOVERSION "1.1.2"
#define COLUMN_COUNT 6
#define lengthOfTime    32
#define lengthOfSize    32
#define legnthofStatics 64
#define MAX_ROWS 2048
AVCodecContext *pCodecCtx = NULL;
rtspPlayer *player=NULL;
bool hasIframe = false;
typedef struct _timeRecords
{
    long long starttime;
    long long endtime;
    int numberOfFrames;
    int sizeOfFrames;
} timeRecords;
timeRecords tr;

void getFrameStatics(const char *codecName, AVPacket *packet, bool &isIframe, int &min_qp, int &max_qp, double &avg_qp)
{
    int got_frame=0;
    int ret = 0;
    int qp_sum = 0;
    AVBufferRef *qp_table_buf = NULL;
    unsigned char *qscale_table = NULL;

    if (!pCodecCtx)
        return;

    AVFrame *frame = av_frame_alloc();
    int x, y;
    avcodec_send_packet(pCodecCtx, packet);
    got_frame = avcodec_receive_frame(pCodecCtx, frame);

    if (got_frame == 0)
    {
        isIframe = frame->pict_type == AV_PICTURE_TYPE_I;
        if (strcmp(codecName,"H264") == 0)
        {          
            int mb_width = (pCodecCtx->width + 15) / 16;
            int mb_height = (pCodecCtx->height + 15) / 16;
            int mb_stride = mb_width + 1;          
            if (mb_width != 0)
            {
                qp_table_buf = av_buffer_ref(frame->qp_table_buf);
                qscale_table = qp_table_buf->data;              
                if (qscale_table!= NULL)
                {
                    for (y = 0; y < mb_height; y++)
                    {
                        for (x = 0; x < mb_width; x++)
                        {
                            qp_sum += qscale_table[x + y * mb_stride];
                            if (qscale_table[x + y * mb_stride] > max_qp)
                            {
                                max_qp = qscale_table[x + y * mb_stride];
                            }
                            if (qscale_table[x + y * mb_stride] < min_qp || min_qp == 0)
                            {
                                min_qp = qscale_table[x + y * mb_stride];
                            }
                        }
                    }
                    avg_qp = (double)qp_sum / (mb_height * mb_width);               
                }
                av_buffer_unref(&qp_table_buf);
            }
        }
        else
        {
            min_qp=ffpg_get_minqp();
            max_qp=ffpg_get_maxqp();
            avg_qp=ffpg_get_avgqp();
        }
    }
    av_frame_free(&frame);
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
    headers << "Codec" << "Frame type" << "Avg QP" << "Frame size"<<"Arrival Time"<<"Presentation Time";
    ui->setupUi(this);
    QHeaderView *hheading=ui->tableWidget->horizontalHeader();
    QHeaderView *vheading=ui->tableWidget->verticalHeader();
    hheading->setSectionResizeMode(QHeaderView::Stretch);
    vheading->setVisible(false);
    ui->tableWidget->setColumnCount(COLUMN_COUNT);
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableWidget->setFocusPolicy(Qt::NoFocus);

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
    setWindowIcon(QIcon(":/icons/cosmo.ico"));
    QCoreApplication::setApplicationVersion(COSMOVERSION);
    QString appversion="COSMO Version "+QString(COSMOVERSION);
    setWindowTitle(appversion);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void addItemToTable(const char *codecName, const char *frameType, const char *avgqp, const char *frameSize, const char* arrivalTime, const char *presetationTime, void *privateData)
{


    QColor green(Qt::green);
    Ui::MainWindow *ui=(Ui::MainWindow *)privateData;
    QScrollBar *scrollbar=ui->tableWidget->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
    ui->tableWidget->insertRow(ui->tableWidget->rowCount());
    QTableWidgetItem *codec=new QTableWidgetItem(codecName);
    codec->setTextAlignment(Qt::AlignCenter);
    QTableWidgetItem *frame_type=new QTableWidgetItem(frameType);
    frame_type->setTextAlignment(Qt::AlignCenter);
    QTableWidgetItem *frame_qp=new QTableWidgetItem(avgqp);
    frame_qp->setTextAlignment(Qt::AlignCenter);
    QTableWidgetItem *frame_size=new QTableWidgetItem(frameSize);
    //frame_size->setTextAlignment(Qt::AlignCenter);
    QTableWidgetItem *arrival_time=new QTableWidgetItem(arrivalTime);
    //arrival_time->setTextAlignment(Qt::AlignCenter);
    QTableWidgetItem *presentation_time=new QTableWidgetItem(presetationTime);
    //presentation_time->setTextAlignment(Qt::AlignCenter);

    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,0,codec);
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,1,frame_type);
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,2,frame_qp);
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,3,frame_size);
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,4,arrival_time);
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,5,presentation_time);

    if (strcmp("I",frameType)==0)
    {
        for (int j=0;j<COLUMN_COUNT;j++)
        {
            if (strcmp(codecName,"JPEG") != 0)
                ui->tableWidget->item(ui->tableWidget->rowCount()-1,j)->setBackground(green);
        }
    }

    if (ui->tableWidget->rowCount()>MAX_ROWS)
    {
        ui->tableWidget->removeRow(0);
    }
}

bool checkspsOrpps(const char *codecName, unsigned char *videoData)
{
    int nal_unit_type;
    if (strcmp(codecName,"H264")==0)
    {
        nal_unit_type = videoData[0] & 0x1f;
        if (nal_unit_type==7 || nal_unit_type ==8) return true;
    }
    if (strcmp(codecName,"H265")==0)
    {
        nal_unit_type=(videoData[0]& 0x7E)>>1;
        if (nal_unit_type==32 || nal_unit_type ==33 || nal_unit_type ==34)
            return true;
    }
    return false;
}

void onFrameArrival(unsigned char *videoData, const char *codecName, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, void *privateData)
{
    char uSecsStr[lengthOfTime];
    char videoSize[lengthOfSize];
    char avgqp[lengthOfSize];
    char statics[legnthofStatics];
    char arrivalTime[lengthOfTime];
    u_int8_t start_code[4] = {0x00, 0x00, 0x00, 0x01};
    u_int8_t *frameData;
    AVPacket packet;
    int min_qp = 0;
    int max_qp = 0;
    int qp_sum = 0;
    double avg_qp = 0;
    bool isIframe = false;
    Ui::MainWindow *ui=(Ui::MainWindow *)privateData;
    double frameArrivalTime = getCurrentTimeMicroseconds()/1000000.0;
    snprintf(arrivalTime,lengthOfTime,"%.3f",frameArrivalTime);
    //qDebug("frame arrival time %.3f",frameArrivalTime);
    if (strcmp(codecName, "JPEG") == 0 || strcmp(codecName, "H264") == 0 || strcmp(codecName, "H265") == 0)
    {
        snprintf(uSecsStr, lengthOfTime, "%u.%03u",  (int)presentationTime.tv_sec, (unsigned)presentationTime.tv_usec/1000);
        snprintf(videoSize,lengthOfSize,"%u", frameSize);
    }
    else return;

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
        addItemToTable("JPEG","I","NA",videoSize,arrivalTime, uSecsStr, privateData);
        return;
    }
    frameData = (u_int8_t *)malloc(frameSize + 4);
    memcpy(frameData, start_code, 4);
    memcpy(frameData + 4, videoData, frameSize);
    av_new_packet(&packet, frameSize + 4);
    packet.data = frameData;
    packet.size = frameSize + 4;
    getFrameStatics(codecName,&packet, isIframe, min_qp, max_qp, avg_qp);
    snprintf(avgqp,lengthOfSize,"%.2f",avg_qp);
    if (isIframe)
    {
        /*std::cout << " codec:" << codecName << " I-Frame "
                  << " size:" << frameSize << " bytes "
                  << "presentation time:" << (int)presentationTime.tv_sec << "." << uSecsStr << "\n";*/
        hasIframe=true;
        addItemToTable(codecName,"I", avgqp,videoSize,arrivalTime, uSecsStr, privateData);
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
        if (checkspsOrpps(codecName, videoData) == false)
        {
            if (hasIframe)
            {

                    addItemToTable(codecName,"P",avgqp,videoSize,arrivalTime, uSecsStr, privateData);
            }
            if (tr.starttime != 0)
            {
                tr.numberOfFrames++;
                tr.sizeOfFrames += frameSize;
            }
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
    ui->tableWidget->setRowCount(0);
    std::string url=ui->urlText->toPlainText().toStdString();
    std::string username=ui->usernameText->toPlainText().toStdString();
    std::string password=ui->passwordText->toPlainText().toStdString();
    hasIframe=false;
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
    else
    {
        ui->startButton->setEnabled(false);
        ui->overTCPCheck->setEnabled(false);
        ui->urlText->setEnabled(false);
        ui->usernameText->setEnabled(false);
        ui->passwordText->setEnabled(false);
    }
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
    if (player!=NULL)
    {
        player->stopRTSP();
        delete player;
    }
    player=NULL;
    ui->startButton->setEnabled(true);
    ui->overTCPCheck->setEnabled(true);
    ui->urlText->setEnabled(true);
    ui->usernameText->setEnabled(true);
    ui->passwordText->setEnabled(true);
    if (pCodecCtx != NULL)
        avcodec_free_context(&pCodecCtx);
}


void MainWindow::on_actionsave_statics_triggered()
{
    int i,j,x=2,y=1;
    QXlsx::Document xlsxW;
    QString filename=QFileDialog::getSaveFileName(NULL, "Export statics", "", "*.xlsx");

    xlsxW.write(1, 1, QString("Codec"));
    xlsxW.write(1, 2, QString("Frame type"));
    xlsxW.write(1, 3, QString("Avg QP"));
    xlsxW.write(1, 4, QString("Frame size"));
    xlsxW.write(1, 5, QString("Arrival Time"));
    xlsxW.write(1, 6, QString("Presentation Time"));

    for (i=0;i<ui->tableWidget->rowCount();i++)
    {
        for (j=0;j<COLUMN_COUNT;j++)
        {
            xlsxW.write(x, y, ui->tableWidget->item(i,j)->text());
            y++;
        }
        x++;
        y=1;
    }
    xlsxW.saveAs(filename);
}

