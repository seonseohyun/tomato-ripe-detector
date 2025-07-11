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
    m_connector->sendJson(loginJson, [this](QJsonObject res) {
        QString proto = res["protocol"].toString();
        qDebug() << "[Login 콜백 JSON]" << res;
        qDebug() << "[Login 콜백 proto]" << proto;

        bool success = (proto == "login_0");  //이게 틀렸나?

        // 3) 메인쓰레드에서 UI 처리하도록 invokeMethod로 실행
            QMetaObject::invokeMethod(this, [this, success, res]() { //성공여부랑 응답 제이슨을 통째로 씀
            qDebug() << "[Login 콜백] : " << success;

            if (success) {
                emit loginSuccess(res);
                QMessageBox::information(nullptr, "로그인 성공", "로그인 성공!");
            } else {
                QMessageBox::warning(nullptr, "로그인 실패", "ID/PW 확인!");
            }
        });
    });
}



login::~login()
{
    delete ui;
}
