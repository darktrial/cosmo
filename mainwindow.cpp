#include <QScrollBar>
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

