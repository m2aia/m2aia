/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Lorenz Schwab
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#ifndef CHART_H
#define CHART_H

#include <QTimer>
#include <QWidget>
/*#include <QtCharts/QAreaSeries>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
*/
QT_BEGIN_NAMESPACE
namespace Ui { class Chart; }
QT_END_NAMESPACE

class Chart : public QWidget
{
    Q_OBJECT

public:
    Chart(QWidget *parent = nullptr);
    ~Chart();
//    void show();
//    void Load_File();

private:
    //void update();

    Ui::Chart *ui;
    //QtCharts::QLineSeries* bottom_line, *graph;
    //QtCharts::QAreaSeries* plot;
    //QtCharts::QChart* chart;
    //QtCharts::QChartView* chart_view;
    //QPen* pen;
    //QLinearGradient* gradient;
    //QTimer* timer;
};
#endif // CHART_H
