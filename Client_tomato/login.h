#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include <QMessageBox>
#include "server_connector.h"

namespace Ui {
class login;
}

class login : public QWidget
{
    Q_OBJECT

public:
    explicit login(server_connector *connector, QWidget *parent = nullptr);
    void login_sig(QJsonObject res);

    ~login();
    server_connector *m_connector;  //멤버변수로 가질거야 -> 이녀석을 통해서만 서버 요청할것

signals:
    void loginSuccess(QJsonObject res);  //로그인 성공시 시그널 보냄, 제이슨파일 통째로 알려줌 -> 메인에서 창 넘기기 할게용~

private slots:
    void on_Btn_login_clicked();  //로그인 버튼 클릭

private:
    Ui::login *ui;
};

#endif // LOGIN_H
