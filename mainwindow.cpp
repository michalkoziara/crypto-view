#include "mainwindow.h"
#include "ui_mainwindow.h"

// biblioteki obslugi Json
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>

// biblioteki obslugi okna wykresu
#include <QLogValueAxis>
#include <QValueAxis>
#include <QChart>

// biblioteki obslugi wykresow
#include <QtCharts/QCandlestickSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QCandlestickSeries>
#include <QtCharts/QChartView>

#include <QDateTimeAxis>

// rysowanie
#include <QVector>
#include <QPainter>
#include <QSplineSeries>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    qDebug()<<"Wersja SSL przewidywana z QT: "<<QSslSocket::sslLibraryBuildVersionString();
    qDebug()<<"Wersja SSL uzywana podczas uruchomienia: "<<QSslSocket::sslLibraryVersionNumber();
    qDebug()<<"Sciezki bibliotek: "<<QCoreApplication::libraryPaths();

    manager = new QNetworkAccessManager();
    downloadCoinList();

    chartView = new QChartView(this);    
    chartView->setRenderHint(QPainter::Antialiasing);

    chartView_indicators = new QChartView(this);
    chartView_indicators->setRenderHint(QPainter::Antialiasing);
    // dodaje wykres do layoutu
    chart = new QChart();
    chart_indicators = new QChart();

    // wyglad tla wykresu
    QLinearGradient plotAreaGradient;
    plotAreaGradient.setStart(QPointF(0, 0));
    plotAreaGradient.setFinalStop(QPointF(0, 1));
    plotAreaGradient.setColorAt(0.0, QRgb(0xbdd3ff));
    plotAreaGradient.setColorAt(1.0, QRgb(0xf2f7ff));
    plotAreaGradient.setCoordinateMode(QGradient::ObjectBoundingMode);

    chart->setPlotAreaBackgroundBrush(plotAreaGradient);
    chart->setPlotAreaBackgroundVisible(true);

    chart_indicators->setPlotAreaBackgroundBrush(plotAreaGradient);
    chart_indicators->setPlotAreaBackgroundVisible(true);

    ui->verticalLayout_3->addWidget(chartView);
    ui->verticalLayout_3->addWidget(chartView_indicators);

    label = new QLabel(this);
    ui->verticalLayout_3->addWidget(label);
}


MainWindow::~MainWindow()
{
    delete chartView;
    delete manager;
    delete ui;
}

void MainWindow::downloadCoinList()
{
    QNetworkRequest request;
    request.setUrl(QUrl("https://api.binance.com/api/v1/ticker/allPrices"));

    QObject::connect(manager, SIGNAL(finished(QNetworkReply*)), this,  SLOT(downloadComplete(QNetworkReply*)),Qt::UniqueConnection);
    manager->get(request);
}


void MainWindow::downloadComplete(QNetworkReply* reply)
{
    if (reply->error()) {
        qDebug() << reply->errorString();
        return;
    }

    //przeksztalcam w json

    QJsonParseError err;
    QJsonDocument jsonResponse = QJsonDocument::fromJson(reply->readAll(), &err);
    reply->deleteLater();

    qDebug() << jsonResponse.isArray();
    qDebug() << jsonResponse.isObject();
    qDebug() << jsonResponse.isNull();

    QJsonArray array = jsonResponse.array();
    qDebug() << array;

    foreach (const QJsonValue & v, array)
        ui->comboBox_2->addItem(v.toObject().value("symbol").toString());

    disconnect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadComplete(QNetworkReply*)));
}


void MainWindow::on_pushButton_clicked()
{
    QNetworkRequest request;
    request.setUrl(QUrl("https://api.binance.com/api/v1/klines?symbol="+ui->comboBox_2->itemText(combo2_index)+"&interval=1"+histo_time+"&limit="+QString::number(ui->horizontalSlider->value())));

    QObject::connect(manager, SIGNAL(finished(QNetworkReply*)), this,  SLOT(on_request_complete(QNetworkReply*)),Qt::UniqueConnection);
    manager->get(request);
}

void MainWindow::on_request_complete(QNetworkReply* reply)
{
    if (reply->error()) {
        qDebug() << reply->errorString();
        return;
    }
    chart->removeAllSeries();

    QString strReply = (QString)reply->readAll();
    reply->deleteLater();

    //przeksztalcam w json
    QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toLatin1());

    qDebug() << jsonResponse.isArray();
    qDebug() << jsonResponse.isObject();

    QJsonArray out_array = jsonResponse.array();

    // ustawiam serie swiec japonskich
    QtCharts::QCandlestickSeries *series =  new QCandlestickSeries();
    series->setIncreasingColor(QColor(Qt::green));
    series->setDecreasingColor(QColor(Qt::red));
    QStringList categories;

   // QVector<QPointF> closeVector;
   QLineSeries *series_line = new QLineSeries;

    foreach (const QJsonValue & v, out_array)
    {
        qDebug() << v;
        // ustawiam parametry swiecy
        const qreal timestamp = v.toArray().at(0).toDouble();
        double open = QString(v.toArray().at(1).toString()).toDouble();
        double high = QString(v.toArray().at(2).toString()).toDouble();
        double low = QString(v.toArray().at(3).toString()).toDouble();
        double close = QString(v.toArray().at(4).toString()).toDouble();

        qDebug() << "open" << open;

        QPointF p(timestamp,high);
        qDebug() << p;
       // closeVector.push_back(p);
        *series_line << p;

        QtCharts::QCandlestickSet *candlestickSet =
                new QtCharts::QCandlestickSet(open,high,low,close,timestamp);

        series->append(candlestickSet);

        // dodaje pojedyncza swiece do serii wykresu

        // podpisy
        if(histo_time=="d")
            categories << QDateTime::fromMSecsSinceEpoch(candlestickSet->timestamp()).toString("dd MMM\n");
        else if(histo_time == "h")
            categories << QDateTime::fromMSecsSinceEpoch(candlestickSet->timestamp()).toString("hh ddd\n");
        else
            categories << QDateTime::fromMSecsSinceEpoch(candlestickSet->timestamp()).toString("hh:mm\n");

        qDebug() << QDateTime::fromMSecsSinceEpoch(candlestickSet->timestamp()).toString();
   // categories << QString::number(candlestickSet->timestamp());
    }

    // Tytul okna
    QFont font;
    font.setPixelSize(18);
    chart->setTitleFont(font);
    chart->setTitleBrush(QBrush(Qt::black));
    chart->setTitle(ui->comboBox_2->itemText(combo2_index));

    // tworze wykres w oknie wykresu
    chart->addSeries(series);
    chart->legend()->hide();

    chart->setAnimationOptions(QChart::SeriesAnimations);

    chart->createDefaultAxes();
    QBarCategoryAxis *axisX = qobject_cast<QBarCategoryAxis *>(chart->axes(Qt::Horizontal).at(0));
    QValueAxis *axisY = qobject_cast<QValueAxis *>(chart->axes(Qt::Vertical).at(0));

    axisX->setCategories(categories);
    axisX->setGridLineColor(QRgb(0xffffff));
    axisY->setGridLineColor(QRgb(0xffffff));

    axisY->setMax(axisY->max() * 1.01);
    axisY->setMin(axisY->min() * 0.99);

    axisY->setLabelFormat("%.10f");

    if(ui->horizontalSlider->value()>50)
    {
        QFont labelsFont;
        labelsFont.setPixelSize(6);
        axisX->setLabelsFont(labelsFont);
    }

    chartView->setChart(chart);


    // indicator charts

    chart_indicators->addSeries(series_line);
    chart_indicators->createDefaultAxes();

    QValueAxis *axisX_indicator = qobject_cast<QValueAxis *>(chart_indicators->axes(Qt::Horizontal).at(0));
    QValueAxis *axisY_indicator = qobject_cast<QValueAxis *>(chart_indicators->axes(Qt::Vertical).at(0));

    axisX_indicator->setGridLineColor(QRgb(0xffffff));
    axisY_indicator->setGridLineColor(QRgb(0xffffff));

    axisY_indicator->setMax(axisY->max() * 1.01);
    axisY_indicator->setMin(axisY->min() * 0.99);

    axisY_indicator->setLabelFormat("%.10f");

    chartView_indicators->setChart(chart_indicators);
    chart->legend()->hide();

    disconnect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(on_request_complete(QNetworkReply*)));
    QObject::disconnect(manager);
}


void MainWindow::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0) {
        zoomX(2, event->pos().x() - chart->plotArea().x());
        qDebug() << chart->plotArea();
    } else if (event->angleDelta().y() < 0) {
        zoomX(0.9, event->pos().x() - chart->plotArea().x());
    }
}

void MainWindow::zoomX(qreal factor, qreal xcenter)
{
    QRectF rect = chart->plotArea();
    qreal widthOriginal = rect.width();
    rect.setWidth(widthOriginal / factor);
    qreal centerScale = (xcenter / widthOriginal);

    qreal leftOffset = (xcenter - (rect.width() * centerScale) );

    rect.moveLeft(rect.x() + leftOffset);
    chart->zoomIn(rect);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if(event->button()==Qt::MiddleButton && chartView->underMouse())
        chart->zoomReset();
}

void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    switch(index)
    {
    case 0:
    {
        histo_time = "d";
        break;
    }
    case 1:
    {
        histo_time = "h";
        break;
    }
    case 2:
    {
        histo_time = "m";
        break;
    }
    }
}

void MainWindow::on_comboBox_2_currentIndexChanged(int index)
{
    combo2_index=index;
}
