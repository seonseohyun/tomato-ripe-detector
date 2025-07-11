#ifndef SERVER_CONNECTOR_H
#define SERVER_CONNECTOR_H

#include <QWidget>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <functional>

namespace Ui {
class server_connector;
}

class server_connector : public QObject
{
    Q_OBJECT

public:
    explicit server_connector(QObject *parent = nullptr);
    void connect_to_server();   // 서버 연결 시도
    QTcpSocket* getSocket() const;  //소켓 리턴
    QTimer* reconnect_timer = nullptr;  //재연결 시도하는 타이머
    void onReadyRead();
    QTcpSocket *socket_;

    //==통신==
    void sendJson(const QJsonObject &json,std::function<void(QJsonObject)> callback = nullptr);
    void sendImage(QString imagePath, int farm_num);
    ~server_connector();

signals:
    void server_connected(bool success);  //true면 연결 false면 연결실패 -> 메인윈도우에 알려주기

private slots:

private:
    Ui::server_connector *ui;
    QByteArray recv_buffer_;
    std::function<void(QJsonObject)> responseHandler_;
};

#endif // SERVER_CONNECTOR_H
