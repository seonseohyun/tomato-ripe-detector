#include "image_selector.h"
#include "ui_image_selector.h"

#include <QFileDialog>
#include <QPixmap>
#include <QIcon>
#include <QMessageBox>
#include <QDataStream>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QDebug>
#include <arpa/inet.h>



image_selector::image_selector(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::image_selector)
{
    ui->setupUi(this);
    //버튼 시그널 연결함 ~ 이미지 선택이랑~
    connect(ui->Btn_upload, &QPushButton::clicked, this, &image_selector::onSelectImage);  //이미지 선택 버튼
    connect(ui->Btn_server, &QPushButton::clicked, this, &image_selector::onUploadImage); //서버 전송 버튼

}
void image_selector::onSelectImage()
{
    QString filePath = QFileDialog::getOpenFileName(this, "이미지 선택", "/home/up/Desktop/도마도/아카이브/097.지능형 스마트팜 통합 데이터(토마토)_val_img/01.데이터/2.Validation/원천데이터/e.열매", "Images (*.png *.jpg *.jpeg)");
    if (!filePath.isEmpty()) {
        QPixmap pix(filePath);
        ui->img->setIcon(QIcon(pix));
        ui->img->setIconSize(ui->img->size());

        currentImagePath = filePath;
        QMessageBox::information(this, "", "이미지가 선택되었습니다.");
        emit image_selected(filePath); //경로 전달해주고
    } else {
        QMessageBox::information(this, "", "이미지 선택을 취소하셨습니다.");
    }
}

void image_selector::onUploadImage()
{
    if (currentImagePath.isEmpty()) {
        QMessageBox::warning(this, "경고", "먼저 이미지를 선택해주세요.");
        return;
    }

    QFile file(currentImagePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "에러", "이미지 파일을 열 수 없습니다.");
        return;
    }
    file.close();

    emit image_ready(currentImagePath);

    qDebug() << "[image_selector] 업로드 요청: " << currentImagePath;

    reset();
}

void image_selector::reset(){
    currentImagePath.clear();
    ui->img->setIcon(QIcon());
    qDebug() << "[image_selector] 선택 상태 초기화 완료!";

}

image_selector::~image_selector()
{
    delete ui;
}
