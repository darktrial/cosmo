#include <QScrollBar>
#include <QSettings>
#include <QFileDialog>
#include <QFile>
#include "mainwindow.h"
#include "./ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    QStringList headers;
    headers << "Codec" << "Frame type" << "Frame size"<<"Time";
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


/*void MainWindow::on_pushButton_clicked()
{


}*/




void MainWindow::on_startButton_clicked()
{
    /*QTextCursor cursor=ui->textBrowser->textCursor();
    QScrollBar *scrollbar=ui->textBrowser->verticalScrollBar();

    QTextBlockFormat format;
    format.setBackground(Qt::red);
    cursor.clearSelection();
    cursor.movePosition(QTextCursor::End);
    ui->textBrowser->setTextCursor(cursor);
    QString str("H264");
    str+=QString(60,' ');
    str+=QString("I");
    str+=QString(60,' ');
    str+=QString("1234566");
    str+=QString(60,' ');
    str+=QString("1.16");
    ui->textBrowser->insertPlainText(str);
    scrollbar->setValue(scrollbar->maximum());*/
    QScrollBar *scrollbar=ui->tableWidget->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
    ui->tableWidget->insertRow(ui->tableWidget->rowCount());
    QTableWidgetItem *item=new QTableWidgetItem("12345");
    item->setTextAlignment(Qt::AlignCenter);
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,0,item);//new QTableWidgetItem("12345",Qt::AlignCenter));
    //QString colcount=QString("%1").arg(ui->tableWidget->rowCount());
    //ui->statusbar->showMessage(colcount);
    if (ui->tableWidget->rowCount()>10)
        ui->tableWidget->removeRow(1);
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

