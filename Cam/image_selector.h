#ifndef IMAGE_SELECTOR_H
#define IMAGE_SELECTOR_H

#include <QWidget>
#include <QTcpSocket>

namespace Ui {
class image_selector;
}

class image_selector : public QWidget
{
    Q_OBJECT

public:
    explicit image_selector(QWidget *parent = nullptr);
    ~image_selector();
    QString get_image_path() const;
    void setSocket(QTcpSocket* s) { socket = s; }
    void reset();    //이미지 선택 및 전송 후 초기화

signals:
    void image_selected(const QString &path);
    void image_ready(QString imagePath);

private slots:
    void onSelectImage();  // 버튼 연결될 슬롯
    void onUploadImage();  // 전송용 슬롯


private:
    Ui::image_selector *ui;
    QString currentImagePath;  // 이미지 경로 저장용
    QTcpSocket* socket = nullptr;  // 소켓 저장용

};

#endif // IMAGE_SELECTOR_H
