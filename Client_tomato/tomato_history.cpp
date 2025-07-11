#include "tomato_history.h"
#include "ui_tomato_history.h"
#include "common_struct.h"

tomato_history::tomato_history(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::tomato_history)
{
    ui->setupUi(this);
    ui->dateEdit->setDate(QDate::currentDate());  //기본 값 오늘 날짜로 지정ㅎ

    connect(ui->Btn_go_chart, &QPushButton::clicked, this, [=](){
        emit goChart(lst_tomato);
    });
    //currentIndexChanged는 두 버전이 있음 인덱스 넘겨주기랑 텍스트 넘겨주기. 그래서 어느 시그널에 연결할지 명시 안하믄 c++ 컴파일 오류난다.
}

void tomato_history::setFarmList(const QList<std_farminfo> &farms, server_connector *connector){
    ui->comboBox_farm->clear();
    for (const std_farminfo &f : farms) {
        ui->comboBox_farm->addItem(f.name, f.num);
    }

    if (!farms.isEmpty()) {
        updateAreaCombo(farms.first().area_cnt);
        ui->comboBox_farm->setCurrentIndex(0);  // 첫번째 농장 선택! 인덱스 0으로 임의로 바꿔줌 요기도
    }

    // farm 콤보박스 선택 바뀌면 area 콤보 갱신 ,dateEdit은 따로 연결해야되는구나.
    connect(ui->comboBox_farm, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [=](int index) {
                if (index >= 0 && index < farms.size()) {
                    updateAreaCombo(farms[index].area_cnt);
                }
            });
    this->m_connector = connector;
    this->send_img_request();
    this->set_widget();
    connect(this->ui->comboBox_farm, &QComboBox::currentIndexChanged,this,&tomato_history::send_img_request);
    connect(this->ui->dateEdit, &QDateEdit::dateChanged, this, &tomato_history::send_img_request);
    connect(ui->comboBox_area, QOverload<int>::of(&QComboBox::currentIndexChanged),this, &tomato_history::set_widget);
}

void tomato_history::send_img_request(){
    QJsonObject ReqImgJson;
    int idx = ui->comboBox_farm->currentIndex();

    ReqImgJson["protocol"] = "select_result"; // 요청합니다. 프로토콜 -> select_result
    ReqImgJson["farm_num"] = this->ui->comboBox_farm->itemData(idx).toString();
    QString dateString = ui->dateEdit->date().toString("yyyy-MM-dd");
    ReqImgJson["date"] = dateString;
    qDebug() << "[선택한 날짜]" << dateString;
    this->m_connector->sendJson(ReqImgJson, [this](QJsonObject res){this-> recv_img(res);});
    this->clear_widget();

}

void tomato_history::updateAreaCombo(int area_cnt)
{
    ui->comboBox_area->clear();
    for (int i = 1; i <= area_cnt; ++i) {
        ui->comboBox_area->addItem(QString("AREA %1").arg(i));
    }
    //큐티에서는 선택안한 콤보박스 인덱스를 -1로 처리한대요~ 그래서 0을 줘버림
    if (area_cnt > 0) {
        ui->comboBox_area->setCurrentIndex(0);
    }
}

void tomato_history::recv_img(QJsonObject res){
    lst_img.clear();
    lst_tomato.clear();

    QJsonArray results = res["results"].toArray();
    if (results.isEmpty()) {
        qDebug() << "[에러] 서버에서 결과 없음";
        lst_img.clear();
        return;  // 더 진행 안함
    }
    for (const QJsonValue &ResultVal : results) {
        QJsonObject ResultObj = ResultVal.toObject();
        std_image img;
        img.total_cnt = ResultObj["total"].toInt();
        img.red_cnt = ResultObj["red"].toInt();
        img.yellow_cnt = ResultObj["yellow"].toInt();
        img.green_cnt = ResultObj["green"].toInt();

        QByteArray b64 = ResultObj["before"].toString().toUtf8();
        QByteArray raw = QByteArray::fromBase64(b64);
        img.before = raw;

        qDebug() << "[디코딩된 이미지 크기 before]" << img.before.size();

        QByteArray b64_ = ResultObj["after"].toString().toUtf8();
        QByteArray raw_ = QByteArray::fromBase64(b64_);
        img.after = raw_;

        qDebug() << "[디코딩된 이미지 크기 after]" << img.after.size();
        lst_img.append(img);
    }

    QJsonArray resultsTotal = res["results_total"].toArray();
    if (results.isEmpty()) {
        qDebug() << "[에러] 서버에서 결과 없음";
        lst_img.clear();
        return;  // 더 진행 안함
    }

    for (const QJsonValue &Result_all : resultsTotal) {
        QJsonObject ResultToTalObj = Result_all.toObject();
        std_tomato tomato;

        tomato.farm_total_cnt = ResultToTalObj["TOTAL_COUNT"].toInt();
        tomato.total_red_cnt = ResultToTalObj["RED_TOMATO_COUNT"].toInt();
        tomato.total_yellow_cnt = ResultToTalObj["YELLOW_TOMATO_COUNT"].toInt();
        tomato.total_green_cnt = ResultToTalObj["GREEN_TOMATO_COUNT"].toInt();
        tomato.harv_date = ResultToTalObj["HARVEST_DATE"].toString();
        tomato.date = ResultToTalObj["DATE"].toString();
        tomato.growthrate = ResultToTalObj["GROWTHRATE"].toInt();

        lst_tomato.append(tomato);
    }
    this->set_widget();
}

void tomato_history::set_widget(){
    qDebug() << "[위젯 세팅]";

    int area_index = ui->comboBox_area->currentIndex();
    if (area_index < 0 || area_index >= lst_img.size()) {
        qDebug() << "[에러] 인덱스 범위 초과:" << area_index;
        return;
    }

    QPixmap pixmap_before;
    QPixmap pixmap_after;
    qDebug() << "[인덱스]"<< area_index;

    //before 셋팅
    if (pixmap_before.loadFromData(lst_img[area_index].before)) {
        ui->label_img_before->setPixmap(pixmap_before.scaled(this->ui->label_img_before->size()));

        qDebug() << "[성공] after 이미지 로드 성공";
    }
    else{
        qDebug() << "[에러] before 이미지 로드 실패";
    }

    //after 셋팅
    if (pixmap_after.loadFromData(lst_img[area_index].after)) {
        ui->label_img_after->setPixmap(pixmap_after.scaled(this->ui->label_img_after->size()));
        qDebug() << "[성공] after 이미지 로드 성공";
    }
    else{
        qDebug() << "[에러] after 이미지 로드 실패";
    }

    ui->label_total_cnt->setText(QString::number(lst_img[area_index].total_cnt));
    ui->label_red_cnt->setText(QString::number(lst_img[area_index].red_cnt));
    ui->label_yellow_cnt->setText(QString::number(lst_img[area_index].yellow_cnt));
    ui->label_green_cnt->setText(QString::number(lst_img[area_index].green_cnt));
    this->set_total_widget();

}

void tomato_history::set_total_widget(){
    QString date = this->ui->dateEdit->date().toString("yyyy-MM-dd");
    int index;
    qDebug() << lst_tomato.size();

    for(int i = 0; i< lst_tomato.size() ; i++){
        if(lst_tomato[i].date == date){
            index = i;
        }
    }
    this->ui->label_farm_total_cnt->setText(QString::number(lst_tomato[index].farm_total_cnt));
    qDebug() << lst_tomato[index].farm_total_cnt;
    this->ui->label_total_red_cnt->setText(QString::number(lst_tomato[index].total_red_cnt));
    this->ui->label_total_yellow_cnt->setText(QString::number(lst_tomato[index].total_yellow_cnt));
    this->ui->label_total_green_cnt->setText(QString::number(lst_tomato[index].total_green_cnt));
    this->ui->label_harv_date->setText(lst_tomato[index].harv_date);
}

void tomato_history::clear_widget(){
    qDebug() << "[위젯 클리어]";

    this->ui->label_img_before->clear();
    this->ui->label_img_after->clear();

    this->ui->label_total_cnt->clear();
    this->ui->label_red_cnt->clear();
    this->ui->label_yellow_cnt->clear();
    this->ui->label_green_cnt->clear();

    this->ui->label_farm_total_cnt->clear();
    this->ui->label_total_red_cnt->clear();
    this->ui->label_total_yellow_cnt->clear();
    this->ui->label_total_green_cnt->clear();
}

tomato_history::~tomato_history()
{
    delete ui;
}
