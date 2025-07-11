#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "server_connector.h"
#include "login.h"
#include "tomato_history.h"
#include "common_struct.h"
#include "chart.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    //================================
    QList<std_farminfo> lst_farminfo;

    void farm_select();
    void go_chart(QList<std_tomato> data);
    void go_home();
    void update_status(bool connected);
    int selected_farm_num = 0;        //선택한 농장 번호
    int farm_cnt;                     //농장 개수
    ~MainWindow();

private slots:
    void onLoginSuccess(QJsonObject res);

private:
    Ui::MainWindow *ui;
    server_connector *connector;
    login *loginWidget;             //로그인 위젯 포인터
    tomato_history *historyWidget;  //분석창 위젯 포인터
    chart *chartWidget;             //차트 위젯 포인터
};
#endif // MAINWINDOW_H
