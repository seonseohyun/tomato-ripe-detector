import socket
import json
import numpy as np
import cv2
from PIL import Image
import base64
from detectron2.config import get_cfg
from detectron2 import model_zoo
from detectron2.engine import DefaultPredictor
from detectron2.data import MetadataCatalog
from tensorflow.keras.models import load_model
from tensorflow.keras.preprocessing.image import img_to_array
import tensorflow as tf
import torch
import os

from detectron2.utils.visualizer import Visualizer, ColorMode

# ----------------- 설정 -----------------
os.environ["TF_FORCE_GPU_ALLOW_GROWTH"] = "true"
tf.config.set_visible_devices([], 'GPU')
# ----------------- Keras 분류기 -----------------
class TomatoRipenessClassifier:
    def __init__(self, model_path):
        self.class_names = ["yellow", "red", "green"]
        self.model = load_model(model_path)

    def preprocess(self, image: Image.Image):
        image = image.resize((224, 224))
        array = img_to_array(image) / 255.0
        return np.expand_dims(array, axis=0)

    def predict(self, image: Image.Image):
        input_array = self.preprocess(image)
        predictions = self.model.predict(input_array, verbose=0)
        return self.class_names[np.argmax(predictions)]
# ----------------- Detectron2 -----------------
def setup_cfg(model_path):
    cfg = get_cfg()
    cfg.merge_from_file(model_zoo.get_config_file(
        "COCO-InstanceSegmentation/mask_rcnn_R_50_FPN_3x.yaml"))
    cfg.MODEL.ROI_HEADS.NUM_CLASSES = 1
    cfg.MODEL.ROI_HEADS.SCORE_THRESH_TEST = 0.75
    cfg.MODEL.WEIGHTS = model_path
    cfg.MODEL.DEVICE = "cuda" if torch.cuda.is_available() else "cpu"
    MetadataCatalog.get("tomato_train").set(thing_classes=["tomato"])
    return cfg
# ----------------- 이미지 수신 -----------------
def recv_image_exact(sock, size):
    data = b''
    while len(data) < size:
        packet = sock.recv(size - len(data))
        if not packet:
            raise ConnectionError("이미지 수신 중 연결 끊김")
        data += packet
    return data
# ----------------- 감지 + 분류 -----------------
def process_image(image_bytes, predictor, classifier, farm_num, img_num):
    nparr = np.frombuffer(image_bytes, np.uint8)
    image = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

    outputs = predictor(image)
    instances = outputs["instances"].to("cpu")
    boxes = instances.pred_boxes.tensor.numpy()

    labels = []
    count = {"green": 0, "yellow": 0, "red": 0}

    for box in boxes:
        x1, y1, x2, y2 = map(int, box)
        crop = image[y1:y2, x1:x2]
        if crop.size == 0:
            labels.append("unknown")
            continue
        crop_pil = Image.fromarray(cv2.cvtColor(crop, cv2.COLOR_BGR2RGB))
        label = classifier.predict(crop_pil)
        labels.append(label)
        if label in count:
            count[label] += 1

    # 라벨 시각화
    v = Visualizer(image[:, :, ::-1], MetadataCatalog.get("tomato_train"),
                   scale=1.0, instance_mode=ColorMode.IMAGE)
    vis_output = v.draw_instance_predictions(instances)
    result_image = np.ascontiguousarray(vis_output.get_image()[:, :, ::-1])

    # 텍스트 라벨 추가
    for i, box in enumerate(boxes):
        x1, y1, _, _ = map(int, box)
        cv2.putText(result_image, labels[i], (x1, y1 - 10),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)

    # JPEG 인코딩 + base64
    _, buffer = cv2.imencode(".jpg", result_image)
    b64_encoded = base64.b64encode(buffer).decode('utf-8')

    return {
        "protocol" : "img_py",
        "farm_num" : farm_num,
        "img_num" : img_num,
        "count": count,
        "result_image_base64": b64_encoded
    }
# ----------------- JSON 응답 전송 -----------------
def send_json(sock, data: dict):
    encoded = json.dumps(data).encode('utf-8')
    sock.sendall(len(encoded).to_bytes(4, 'big'))
    sock.sendall(encoded)

def recv_json(sock):
    # 1. 4바이트 길이 수신
    length_bytes = sock.recv(4)
    if len(length_bytes) < 4:
        raise ConnectionError("길이 정보 수신 실패")

    json_length = int.from_bytes(length_bytes, byteorder='big')

    # 2. 본문 수신 (길이만큼)
    data = b''
    while len(data) < json_length:
        chunk = sock.recv(json_length - len(data))
        if not chunk:
            raise ConnectionError("JSON 본문 수신 중 연결 끊김")
        data += chunk

    # 3. 디코딩 및 JSON 파싱
    json_str = data.decode('utf-8')
    return json.loads(json_str)

HOST = '10.10.20.123'  # 서버 IP (로컬 테스트 시 localhost)
PORT = 12345        # 서버 포트 (서버와 동일해야 함)

client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

client_socket.connect((HOST, PORT))
print("[INFO] 서버에 연결됨")

cfg = setup_cfg("./output/model_final.pth")
predictor = DefaultPredictor(cfg)
classifier = TomatoRipenessClassifier("./넷째.keras")

send_json(client_socket, {"protocol": "LOGIN_PY"})

while True:
    header = recv_json(client_socket)
    print(header)
    protocol = header.get("protocol")
    file_size = header.get("file_size", 0)

    if protocol == "img_py":
        print(f"[INFO] 이미지 수신 요청 ({file_size} 바이트)")
        image_bytes = recv_image_exact(client_socket, file_size)
        farm_num = header.get("farm_num")
        result = process_image(image_bytes, predictor, classifier, header.get("farm_num"), header.get("img_num"))
        send_json(client_socket, result)
        print("[INFO] 결과 전송 완료")