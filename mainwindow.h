#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_startButton_clicked();

    void on_actionSave_triggered();

    void on_actionOpen_triggered();

    void on_stopButton_clicked();

    void on_actionsave_statics_triggered();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
