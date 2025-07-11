#ifndef TOMATO_HISTORY_H
#define TOMATO_HISTORY_H

#include <QWidget>
#include <QJsonArray>
#include "common_struct.h"
#include "server_connector.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class tomato_history;
}
QT_END_NAMESPACE

class tomato_history : public QWidget
{
    Q_OBJECT

public:
    tomato_history(QWidget *parent = nullptr);
    ~tomato_history();
    void setFarmList(const QList<std_farminfo> &farms, server_connector *connector);    //농장정보 콤보박스 셋팅
    void updateAreaCombo(int area_cnt);     //콤보박스 갱신
    void send_img_request();       //이미지 요청
    void set_widget();             //받은 정보 셋팅
    void clear_widget();           //위젯 초기화
    void set_total_widget();
    server_connector *m_connector;
    QList<std_image> lst_img;       //사진 담는 공간~
    QList<std_tomato> lst_tomato;   //농장 토마토 총 개수 담는 공간~


signals:
    void goChart(QList<std_tomato> data);

private:
    Ui::tomato_history *ui;

private slots:
    void recv_img(QJsonObject res);
};
#endif // TOMATO_HISTORY_H
