#include "chart.h"
#include "ui_chart.h"
#include <QScatterSeries>


chart::chart(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::chart)
{
    ui->setupUi(this);

    connect(ui->Btn_gohome, &QPushButton::clicked, this, [=](){
        emit gohome();
    });
    this->chartView = new QChartView(this);
    chartView->setRenderHint(QPainter::Antialiasing);
    this->ui->gridLayout_chart->addWidget(chartView);

    // 싸이즈 조절~
    resize(800, 300);
}

void chart::setting(QList<std_tomato> data)
{
    // 날짜 및 데이터
    for(int i=0; i<data.size(); i++) {
        qDebug()<< data[i].farm_total_cnt << "total count";
    }
    QStringList dates = {};
    QBarSet *red = new QBarSet("빨강도마도 개수");
    QBarSet *yellow = new QBarSet("노랑도마도 개수");
    QBarSet *green = new QBarSet("초록도마도 개수");
    // 막대그래프 데이터 (숙성도)
    QList<qreal> growthRate = {};
    growthRate.clear();
    for(int i = 0; i < data.size(); i++)
    {
        qDebug() << "초록백분율 : " << (data[i].total_green_cnt/data[i].farm_total_cnt) * 100;

        qDebug() << "노랑백분율 : " << (data[i].total_yellow_cnt/data[i].farm_total_cnt) * 100;

        qDebug() << "토탈 : " << data[i].farm_total_cnt;

        qDebug() << "빨강 : " << data[i].total_red_cnt;
        qDebug() << "초록 : " << data[i].total_green_cnt;;
        qDebug() << "노랑 : " << data[i].total_yellow_cnt;
        qDebug() << "성장률 : " << data[i].growthrate;



        qDebug() << "세팅함수의 첫포문 들어왔니?" << i << "번째 도는중 " ;
        *red << (data[i].total_red_cnt * 100)/data[i].farm_total_cnt;
        *green << (data[i].total_green_cnt * 100)/data[i].farm_total_cnt;
        *yellow << (data[i].total_yellow_cnt * 100)/data[i].farm_total_cnt;
        dates.append(data[i].date);
        qDebug() << "숙성도 어팬드 하기전" ;
        growthRate.append(data[i].growthrate);
        qDebug() << "숙성도 어팬드 한다음" ;
    }

    red->setColor(Qt::red);
    yellow->setColor(QColor(0xFFD700));  // 노란색 (gold)
    green->setColor(Qt::green);

    QBarSeries *barSeries = new QBarSeries();
    barSeries->append(red);
    barSeries->append(yellow);
    barSeries->append(green);

    // 선그래프 데이터 (성장률)
    QLineSeries *lineSeries = new QLineSeries();
    lineSeries->setName("성장률 (%)");
    for (int i = 0; i < dates.size(); ++i) {
        qDebug() << i << " 반복문 들어옴?!";
        lineSeries->append(i - barSeries->barWidth()/3, (data[i].total_red_cnt * 100)/data[i].farm_total_cnt);
    }
    // 차트 구성
    QChart *chart = new QChart();
    chart->addSeries(barSeries);
    chart->addSeries(lineSeries);
    chart->setTitle("도마도 숙성도 및 성장률");

    // X축 (날짜 문자열로)
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(dates);
    chart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);
    lineSeries->attachAxis(axisX);

    // Y축
    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, 100);
    axisY->setTitleText("성장률 (%)");
    chart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);
    lineSeries->attachAxis(axisY);

    auto *scatter = new QScatterSeries();
    scatter->setName("성장률 포인트");
    scatter->setMarkerSize(7);                               // 점 크기
    scatter->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    scatter->setColor(Qt::black);                             // 점 색상

    // 2) 데이터 채우기
    for (int i = 0; i < growthRate.size(); ++i)
        scatter->append(i- barSeries->barWidth()/3,(data[i].total_red_cnt * 100)/data[i].farm_total_cnt);

    // 3) 차트에 추가 (가장 마지막에!)
    chart->addSeries(scatter);
    scatter->attachAxis(axisX);
    scatter->attachAxis(axisY);

    // 뷰
    this->chartView->setChart(chart);
}


chart::~chart()
{
    delete ui;
}
