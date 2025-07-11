#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include "image_selector.h"
#include "server_connector.h"
#include "login.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct std_farminfo {
    int num;
    QString name;
    QString address;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    void on_btn_browse_clicked();
    void update_status(bool connected);


    QList<std_farminfo> lst_farminfo;

    void farm_select();
    int selected_farm_num = 0;        //선택한 농장 번호
    ~MainWindow();

private slots:
    void onLoginSuccess(QJsonObject res); //로그인 성공시그널 받기
    void upload_image_sig(QString imagePath);

private:
    Ui::MainWindow *ui;
    server_connector* connector;
    image_selector *imgWidget;  // 멤버로 경로 보관해서 쓸게요~
    login *login_widget;
};
#endif // MAINWINDOW_H
