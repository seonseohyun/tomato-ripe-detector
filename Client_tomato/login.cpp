#include "login.h"
#include "ui_login.h"

login::login(server_connector * connector,QWidget *parent)
    : QWidget(parent)
    ,m_connector(connector)
    , ui(new Ui::login)
{
    ui->setupUi(this);
    connect(ui->Btn_login, &QPushButton::clicked, this, &login::on_Btn_login_clicked);  //로그인 버튼 클릭 연결
    ui->lineEdit_pw->setEchoMode(QLineEdit::Password);
}

void login::on_Btn_login_clicked()
{
    QJsonObject loginJson;
    loginJson["protocol"] = "login"; // 요청합니다. 프로토콜 -> login
    loginJson["id"] = ui->lineEdit_id->text();
    loginJson["password"] = ui->lineEdit_pw->text();

    // 서버에 로그인 JSON 전송 -> 응답 오면 실행될 콜백 람다
    // m_connector->sendJson(loginJson, this->login_sig);

    m_connector->sendJson(loginJson, [this](QJsonObject res){this->login_sig (res);});
}

void login::login_sig(QJsonObject res)
{
    emit loginSuccess(res);
}

login::~login()
{
    delete ui;
}
