#include "mainwindow.h"
#include "ui_mainwindow.h"
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connector = new server_connector(this);  // 서버 연결 위젯 먼저 생성

    login_widget = new login(connector, this);   //로그인 위젯 먼저 생성
    ui->gridLayout_login->addWidget(login_widget);   // UI에 추가

    connect(connector, &server_connector::server_connected, this, &MainWindow::update_status);  // 상태창 업데이트 연결
    connect(login_widget, &login::loginSuccess, this, &MainWindow::onLoginSuccess);   // 로그인 성공 시 창 넘기기 연결

    image_selector* imgWidget = new image_selector(this);  // 이미지 전송 위젯
    imgWidget->setSocket(connector->getSocket());          // 소켓 넘겨주기
    ui->gridLayout_forwidget->addWidget(imgWidget);         // UI에 추가

    connect(imgWidget, &image_selector::image_ready, this, &MainWindow::upload_image_sig);
    connect(this->ui->comboBox, &QComboBox::currentIndexChanged, this, &MainWindow::farm_select);
    connector->connect_to_server();  // 프로그램 시작 즉시 서버 연결
}



void MainWindow::update_status(bool connected)

{
    qDebug() << "업데이트 시도됨, connected = " << connected;

    //날짜 써주기
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    ui->label_date->setText(today);

    //서버 커넥트 여부 써주기
    if (connected) {
        ui->label_status->setText("서버 연결중");
        QPalette pal = ui->label_status->palette();
        pal.setColor(QPalette::WindowText, QColor(Qt::green));
        ui->label_status->setPalette(pal);
    } else {
        ui->label_status->setText("서버 연결 실패");
        QPalette pal = ui->label_status->palette();
        pal.setColor(QPalette::WindowText, QColor(Qt::red));
        ui->label_status->setPalette(pal);
    }
}

void MainWindow::onLoginSuccess(QJsonObject res){
    qDebug() << "[로그인 성공] 응답 JSON:" << res;
    lst_farminfo.clear();  // 이전 농장 정보는 초기화

    QJsonArray farms = res["farms"].toArray();
    for (const QJsonValue &farmVal : farms) {
        QJsonObject farmObj = farmVal.toObject();

        std_farminfo f;
        f.num = farmObj["farm_num"].toString().toInt();
        f.name = farmObj["farm_name"].toString();
        f.address = farmObj["farm_address"].toString();
        lst_farminfo.append(f);

        ui->comboBox->addItem(f.name, f.num);  // 표시 텍스트 = 이름, UserData = 번호
        // qDebug() << "이름: " << f.name << "번호: " << f.num;
        qDebug() << farmObj["farm_num"].toString().toInt();

    }

    qDebug() << "[저장 완료] 농장 수:" << lst_farminfo.size();

    // ui->comboBox->clear();
    // for (const std_farminfo &f : lst_farminfo) {
    //     ui->comboBox->addItem(f.name, f.num);  // 표시 텍스트 = 이름, UserData = 번호
    //     qDebug() << "이름: " << f.name << "번호: " << f.num
    // }
    qDebug() << "콤보박스에 농장 개수 =" << ui->comboBox->count();
    ui->stackedWidget->setCurrentIndex(0);

}

void MainWindow::farm_select() {
    int idx = ui->comboBox->currentIndex();
    selected_farm_num = ui->comboBox->itemData(idx).toInt();
    qDebug() << "선택된 농장 번호:" << selected_farm_num;
    qDebug() << ui->comboBox->itemData(idx).toInt();
}

void MainWindow::upload_image_sig(QString imagePath)
{
    farm_select();
    if (selected_farm_num == 0) {
        QMessageBox::warning(this, "경고", "농장을 선택해주세요!");
        return;
    }

    connector->sendImage(imagePath, selected_farm_num);
    qDebug() << "이미지 전송 요청. 농장 번호:" << selected_farm_num << ", 경로:" << imagePath;
}

MainWindow::~MainWindow()
{
    disconnect(connector, nullptr, this, nullptr);
    delete ui;
}
