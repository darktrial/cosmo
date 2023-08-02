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
    QHeaderView *heading=ui->tableWidget->horizontalHeader();
    heading->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->setColumnCount(4);
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    //ui->statusbar->showMessage("fuck");*/

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
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

}



