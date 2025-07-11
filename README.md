🍅 Tomato Smartfarm System 🍅

**팀원:** 김대업, 오장관, 박은비, 선서현  
**프로젝트:** 토마토 스마트팜 농장주들을 위한 성숙도 예측 프로그램
**진행 기간:** 2024년 6월 24일 ~ 7월 7일

---

## 📌 목표
- 딥러닝 CNN 인공지능 모델을 프로그램화하여 농장 현장에서 실시간으로 토마토 성숙도를 분석하고 결과를 제공.

---

## 역할 분담
- **박은비:** 서버 및 DB 구축
- **선서현:** 클라이언트 관리자, 카메라 클라이언트 UI 생성, 통신
- **오장관:** AI 모델 구현
- **김대업:** 팀장, 업무 분배, 모델 학습

---

## 개발 환경
- OS: Linux
- 언어: C++, Python
- GUI 프레임워크: Qt
- DB: MySQL (MariaDB)
- 통신: TCP/IP 소켓

---

## 주요 기능
- 농장의 카메라가 하루에 한번 사진을 서버에 전송 (가정)
- Mask R-CNN, ResNet 모델을 이용한 이미지 탐지 및 성숙도 예측
- 예측 결과 DB 저장 및 관리자 툴에 시각화 제공
- 농장 단위 데이터 통계 제공 (성숙률, 예상수확일 등)

---

## 구현 흐름 & 통신 구조
- **프로토콜:** JSON 기반, 클라이언트(Qt, Python) ↔ C++ 서버 간 메시지 및 이미지 주고받기
- **핵심 프로토콜:**  
  - `login_qt` : 로그인 및 농장 정보 조회
  - `image_qt` : 이미지 전송, Mask R-CNN 결과 수신
  - `img_py` : Python 서버에서 분석 → 결과 송신
  - `select_result` : 이미지 분석 결과, 분석된 이미지 DB 저장 및 클라이언트 통계 제공

---

## DB ERD
- `FARM_INFO` : 농장 기본 정보 (농장uid, 주소, 농장이름, 소유주, 농장 카메라 수)
- `FARM_IMG` : 촬영 이미지 정보 (이미지uid, 농장번호, 이미지경로, 이미지 송신 시간, 이미지 내 토마토 총 개수, red 개수, yellow 개수, green 개수)
- `FARM_SUMMARY` : 통계 요약 (uid, 농장uid, 이미지경로, 이미지 내 토마토 총 개수, red 개수 합, yellow 개수 합, green 개수 합, 예상 수확일, 성장률)
- `OWNER` : 농장 주 기본 정보 (농장주 uid, 주소, 비밀번호, 사장님 이름)
---

## 와이어프레임
- 카메라 클라이언트: 실시간 토마토 나무 이미지 출력
- 관리자 클라이언트: 성숙도 분석 결과, 농장별 데이터 히스토리 및 통계 차트 제공

---

## 예상 문제 및 해결 방향
- 데이터 수집량 부족 → 클래스 통합, 증강 데이터로 보완
- 서버-클라이언트 데이터 송수신 안정성 확보
- Mask R-CNN & ResNet 혼용 테스트 → 최적화 진행, Fine-tuning

---

## 📂 프로젝트 폴더 구조

tomato-ripe-detector/
├─ Cam/ 카메라 클라이언트 (Qt)
├─ client_tomato/ 관리자 클라이언트 (Qt)
├─ client_python.py AI 분석 클라이언트 (Python)
├─ server_main.cpp TCP 서버 (C++)
├─ output/ Mask R-CNN 모델 파일 폴더
├─ model_train/ CNN 학습 스크립트
├─ README.md

- **Cam/**: 현장 카메라 프로그램, Qt로 제작
- **client_tomato/**: 관리자용 클라이언트, Qt로 제작
- **client_python.py**: Detectron2 + Keras AI 분석기
- **server_main.cpp**: TCP 서버 코드

## 📂 model_train/
- `cnn_model_save_keras.py`: Keras CNN 성숙도 분류 모델 학습 스크립트
- `cnn_model_save_torch.py`: PyTorch 버전 CNN 성숙도 분류 모델 학습 스크립트
- 학습 데이터는 `./tomato_data` 폴더 구조를 사용 (train/val)
- 학습 후 `넷째.keras` 또는 `tomato_resnet01.pth` 저장됨
