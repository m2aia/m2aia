#include "chart.h"
#include "ui_chart.h"
#include <QFile>
#include <QFileDialog>
//#include <QtCharts/QLineSeries>
#include <QPoint>
#include <QTextStream>

#include <iostream>

Chart::Chart(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Chart)
{
    ui->setupUi(this);
    //this->timer = new QTimer(this);
    //connect(this->timer, &QTimer::timeout, this, &Chart::update);
}

Chart::~Chart()
{
    /*
    delete timer;
    delete pen;
    delete gradient;
    delete chart_view;
    delete chart;
    delete bottom_line;
    delete graph;
    delete plot;*/
    delete ui;
}

/*void Chart::Load_File()
{
    QString qs = QFileDialog::getOpenFileName(nullptr, "Open file", nullptr, "*.roc (*.roc)");
    QFile qfile(qs);
    bool ok = qfile.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream file(&qfile);
    if (!ok)
    {
        std::cerr << "the file " << qs.toStdString() << " could not be opened!" << std::endl;
        return;
    }
    bottom_line = new QtCharts::QLineSeries();
    bottom_line->append(0, 0);
    bottom_line->append(1, 0);
    bottom_line->append(1, 1);
    graph = new QtCharts::QLineSeries();
    while (!file.atEnd())
    {
        float x, y;
        file >> x >> y;
        if (file.atEnd()) break;
        graph->append(x, y);
    }
    plot = new QtCharts::QAreaSeries(graph, bottom_line);
}

void Chart::show()
{
    plot->setName("ROC Graph");
    pen = new QPen(0x059605);
    pen->setWidth(0.3);
    plot->setPen(*pen);
    gradient = new QLinearGradient(0, 0, 0, 1);
    gradient->setColorAt(0.0, 0x3cc63c);
    gradient->setColorAt(1.0, 0x26f626);
    gradient->setCoordinateMode(QGradient::ObjectBoundingMode);
    plot->setBrush(*gradient);
    chart = new QtCharts::QChart();
    chart->addSeries(plot);
    chart->setTitle("ROC example");
    chart->createDefaultAxes();
    chart->axes(Qt::Horizontal).first()->setRange(0, 1);
    chart->axes(Qt::Vertical).first()->setRange(0, 1);
    chart_view = new QtCharts::QChartView(chart, this);
    chart->setMinimumWidth(250);
    chart->setMinimumHeight(250);
    chart->setPreferredWidth(this->width());
    chart->setPreferredHeight(this->height());
    chart_view->setRenderHint(QPainter::Antialiasing);
    chart_view->show();
    this->setVisible(true);
    //update the chart every 1 second
    this->timer->start(1000);
}

void Chart::update()
{
    chart->setMaximumWidth(this->width() + 5);
    chart->setMaximumHeight(this->height() + 5);
    chart->setMinimumSize(this->width(), this->height());
    chart_view->setMinimumSize(this->width(), this->height());
}

*/