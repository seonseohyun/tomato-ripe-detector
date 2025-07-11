#include "server_connector.h"
#include <QMessageBox>
#include <QDebug>
#include <arpa/inet.h>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>

server_connector::server_connector(QObject *parent)
    : QObject(parent)
{
    //프로그램 실행 즉시 서버 연결되도록 생성자에서 연결하기
    socket_ = new QTcpSocket(this);
    reconnect_timer = new QTimer(this);

    connect(socket_, &QTcpSocket::connected, this, [=]() {
        qDebug() << "서버 연결됨!";
        emit server_connected(true);   //성공 여부 전달해주세요
        reconnect_timer->stop();  //성공하면 재시도를 중단하세용~
    });

    connect(socket_, &QTcpSocket::disconnected, this, [=]() {
        qDebug() << "서버 연결 끊김!";
        emit server_connected(false);
        reconnect_timer->start(3000); //3초마다 재연결 시도할게요
    });

    connect(socket_, &QTcpSocket::errorOccurred, this, [=](QAbstractSocket::SocketError socketError) {
        qDebug() << "소켓 에러:" << socketError << socket_->errorString();
        emit server_connected(false);
    });


    connect(reconnect_timer, &QTimer::timeout, this, &server_connector::connect_to_server); //타임아웃 될 때마다 커넥트를 실행하쇼
    reconnect_timer->start(0); //처음 실행하자마자 커넥트 시도하삼

    connect(this->socket_, &QTcpSocket::readyRead, this, &server_connector::onReadyRead);  //////*****요녀석

}

//=================서버 연결 ===============
void server_connector::connect_to_server()
{
    socket_->abort();
    qDebug() << "connect_to_server: 연결 시도!";
    socket_->connectToHost("10.10.20.123", 12345);
}

QTcpSocket* server_connector::getSocket() const {
    return socket_;
}

//==============서버 통신 =================

void server_connector::sendJson(const QJsonObject &json, std::function<void(QJsonObject)> handler)
{
    responseHandler_ = handler;

    QJsonDocument doc(json);
    QByteArray body = doc.toJson(QJsonDocument::Compact);
    qDebug() << "보낸 값: " << doc;

    // 길이 헤더: 4바이트
    quint32 len = htonl(body.size());


    // 길이 + 본문 붙이기
    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&len), sizeof(len));
    packet.append(body);

    // 전송
    socket_->write(packet);
    socket_->flush();
}


void server_connector::onReadyRead()
{

    qDebug() << "리드 하나요?" ;
    recv_buffer_ += socket_->readAll();

    while (true) {
        // 1) 헤더(4바이트) 받을 만큼 쌓였는지 확인
        if (recv_buffer_.size() < 4) {
            return;  // 아직 부족
        }

        // 2) 길이 파싱 (BigEndian)
        QDataStream stream(recv_buffer_);
        stream.setByteOrder(QDataStream::BigEndian);
        quint32 bodySize = 0;
        stream >> bodySize;

        // 3) 남은 데이터가 충분한지 확인
        if (recv_buffer_.size() - 4 < bodySize) {
            return;  // 본문이 아직 덜 옴
        }

        // 4) 본문 추출
        QByteArray body = recv_buffer_.mid(4, bodySize);

        // 5) 버퍼에서 헤더+본문 제거
        recv_buffer_ = recv_buffer_.mid(4 + bodySize);

        // 6) JSON 파싱
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(body, &err);
        if (err.error != QJsonParseError::NoError) {
            qDebug() << "[JSON 파싱 실패]" << err.errorString();
            continue;  // 실패해도 다음 메시지 가능성 대비
        }

        QJsonObject json = doc.object();
        // qDebug() << "[파싱된 JSON]" << json;
        responseHandler_(json);
    }
}

void server_connector::sendImage(QString imagePath, int farm_num)
{
    QFile file(imagePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "파일 열기 실패!";
        return;
    }

    QByteArray imageData = file.readAll();
    file.close();

    quint64 fileSize = static_cast<quint64>(imageData.size());

    // === JSON 헤더 ===
    QJsonObject json;
    json["protocol"] = "image";
    json["farm_num"] = farm_num;  //여기서 걸림
    json["file_name"] = QFileInfo(imagePath).fileName();
    json["file_size"] = static_cast<qint64>(fileSize);

    QJsonDocument doc(json);
    QByteArray jsonBody = doc.toJson(QJsonDocument::Compact);
    quint32 jsonLen = htonl(jsonBody.size());

    QByteArray jsonPacket;
    jsonPacket.append(reinterpret_cast<const char*>(&jsonLen), sizeof(jsonLen));
    jsonPacket.append(jsonBody);

    socket_->write(jsonPacket);
    socket_->flush();


    // === 파일 본문 ===
    QByteArray filePacket;

    socket_->write(imageData);
    socket_->flush();

    qDebug() << "[이미지 전송] JSON:" << jsonBody;
    qDebug() << "[이미지 전송] 파일 크기:" << fileSize;
}

server_connector::~server_connector()
{
}
