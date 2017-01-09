#include "statwindow.h"
#include <QVector>
#include <cmath>
#include "civil.h"
#include "ui_statwindow.h"

StatWindow::StatWindow(QWidget *parent)
    : QWidget(parent), ui(new Ui::statWindow)
{
    ui->setupUi(this);
}

StatWindow::~StatWindow()
{
    delete ui;
}

void StatWindow::paintStat()
{
    int maxSample = 1000;
    double step;
    if (size_t(maxSample) > civils.size())
    {
        maxSample = civils.size();
        step = 1.0;
    }
    else
    {
        step = double(civils.size()) / double(maxSample);
    }

    QVector<double> x(maxSample), y(maxSample);
    double maxX = 0.0, maxY = 0.0;
    for (int i = 0; i < maxSample; ++i)
    {
        x[i] = civils[floor(i * step)].birthTime;
        y[i] = civils[floor(i * step)].tech;
        maxX = max(maxX, x[i]);
        maxY = max(maxY, y[i]);
    }

    ui->qCustomPlot->addGraph();
    ui->qCustomPlot->graph(0)->setPen(QPen(Qt::red));
    ui->qCustomPlot->graph(0)->setLineStyle(QCPGraph::lsNone);
    ui->qCustomPlot->graph(0)->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ssCircle, 4));

    ui->qCustomPlot->graph(0)->setData(x, y);
    ui->qCustomPlot->xAxis->setLabel("birthTime");
    ui->qCustomPlot->yAxis->setLabel("tech");
    ui->qCustomPlot->xAxis->setRange(0, maxX);
    ui->qCustomPlot->yAxis->setRange(0, maxY);
    ui->qCustomPlot->replot();
}
