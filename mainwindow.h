#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QtNetwork>

#include <QChartView>
#include <QLabel>

using namespace QtCharts;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private slots:
     void on_pushButton_clicked();
     void on_request_complete(QNetworkReply* reply);

     void wheelEvent(QWheelEvent *event);
     void zoomX(qreal factor, qreal xcenter);
     void mousePressEvent(QMouseEvent *event);

     void on_comboBox_currentIndexChanged(int index);

     void downloadComplete(QNetworkReply* reply);
     void downloadCoinList();

     void on_comboBox_2_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *manager;

    QChartView *chartView;
    QChart *chart;

    QChartView *chartView_indicators;
    QChart *chart_indicators;

    QString histo_time = "d";
    int combo2_index = 0;
    QLabel *label;
};

#endif // MAINWINDOW_H
