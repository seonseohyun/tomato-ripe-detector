#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "login.h"
#include "server_connector.h"
#include "tomato_history.h"
#include "chart.h"

#include <QDate>
#include <QPalette>
#include <QTcpSocket>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include <QDebug>
#include <QMessageBox>
#include <QJsonArray>
#include <QApplication>
#include <QtCharts>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connector = new server_connector(this);

    this->loginWidget = new login(connector, this);
    this->historyWidget = new tomato_history(this);
    connect(connector, &server_connector::server_connected, this, &MainWindow::update_status);  // 상태창 업데이트 연결

    ui->gridLayout_login->addWidget(loginWidget, 0, 0);
    ui->gridLayout_for_analysis->addWidget(historyWidget, 0, 0);
    this->chartWidget = new chart(this);
    ui->gridLayout_for_graph->addWidget(chartWidget, 0, 0);

    //==[페이지 전환]==
    connect(loginWidget, &login::loginSuccess, this, &MainWindow::onLoginSuccess);
    connect(historyWidget, &tomato_history::goChart, this, &MainWindow::go_chart);

    connect(chartWidget, &chart::gohome, this, &MainWindow::go_home);

}

void MainWindow::update_status(bool connected)
{
    qDebug() << "업데이트 시도됨, connected = " << connected;

    //날짜 써주기
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    ui->label_date0_2->setText(today);

    //서버 커넥트 여부 써주기
    if (connected) {
        ui->label_status0_2->setText("서버 연결중");
        QPalette pal = ui->label_status0_2->palette();
        pal.setColor(QPalette::WindowText, QColor(Qt::green));
        ui->label_status0_2->setPalette(pal);
    } else {
        ui->label_status0_2->setText("서버 연결 실패");
        QPalette pal = ui->label_status0_2->palette();
        pal.setColor(QPalette::WindowText, QColor(Qt::red));
        ui->label_status0_2->setPalette(pal);
    }
}



void MainWindow::onLoginSuccess(QJsonObject res)
{
    qDebug() << "[로그인 성공] 응답 JSON:" << res;

    lst_farminfo.clear();

    QJsonArray farms = res["farms"].toArray();
    for (const QJsonValue &farmVal : farms) {
        QJsonObject farmObj = farmVal.toObject();

        std_farminfo f;
        f.num = farmObj["farm_num"].toString().toInt();
        f.name = farmObj["farm_name"].toString();
        f.address = farmObj["farm_address"].toString();
        f.area_cnt = farmObj["farm_area"].toString().toInt();
        lst_farminfo.append(f);
    }
    historyWidget->setFarmList(lst_farminfo, this->connector);
    go_home();
}

void MainWindow::go_chart(QList<std_tomato> data) {
    static int cnt = 0;
    ui->stackedWidget->setCurrentIndex(CHART_PAGE);
    // 제이슨 알려주기~
    this->chartWidget->setting(data);
    qDebug() << "실행횟수 : " << ++cnt;

}

void MainWindow::go_home() {
    ui->stackedWidget->setCurrentIndex(IMG_PAGE);
    // this->chartWidget->deleteLater();
    // this->chartWidget = new chart(this);
    // ui->gridLayout_for_graph->addWidget(this->chartWidget, 0, 0);
    // connect(chartWidget, &chart::gohome, this, &MainWindow::go_home);
}



MainWindow::~MainWindow()
{
    delete ui;
}
