#ifndef CHART_H
#define CHART_H

#include <QWidget>
#include <QApplication>
#include <QtCharts>
#include "common_struct.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class chart;
}
QT_END_NAMESPACE

class chart : public QWidget
{
    Q_OBJECT

public:
    chart(QWidget *parent = nullptr);
    void setting(QList<std_tomato> data);
    ~chart();

signals:
    void gohome();

private:
    Ui::chart *ui;
    QChartView *chartView;
};
#endif // CHART_H
