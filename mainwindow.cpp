#include <QScrollBar>
#include "mainwindow.h"
#include "./ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    QStringList headers;
    headers << "Name" << "Date" << "Undso";
    ui->setupUi(this);
    //ui->statusbar->showMessage("fuck");

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    QTextCursor cursor=ui->textBrowser->textCursor();
    QScrollBar *scrollbar=ui->textBrowser->verticalScrollBar();
    cursor.clearSelection();
    cursor.movePosition(QTextCursor::End);
    ui->textBrowser->setTextCursor(cursor);
    ui->textBrowser->insertPlainText("H.264         I         1234566       1.16\n");
    scrollbar->setValue(scrollbar->maximum());

}



