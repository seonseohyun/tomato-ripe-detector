import os
import tensorflow as tf
from tensorflow.keras.callbacks import EarlyStopping, ReduceLROnPlateau
from tensorflow.keras import Model, Input
import matplotlib.pyplot as plt

os.environ["TF_FORCE_GPU_ALLOW_GROWTH"] = "true"            # 점진적 할당
os.environ["TF_XLA_FLAGS"] = "--tf_xla_enable_xla_devices=0"  # XLA 끄기

# import matplotlib.pyplot as plt

from tensorflow.keras import layers, models
from tensorflow.keras.preprocessing.image import ImageDataGenerator
# 2) fp16 켜기 (반드시 이 순서)
from tensorflow.keras import mixed_precision
mixed_precision.set_global_policy("mixed_float16")

#
# gpus = tf.config.list_physical_devices('GPU')
# if gpus:
#     print("✅ GPU 사용 가능:", gpus)
#     try:
#         # GPU 메모리 자동 할당 설정
#         for gpu in gpus:
#             tf.config.experimental.set_memory_growth(gpu, True)
#     except RuntimeError as e:
#         print(e)
# else:
#     print("❌ GPU를 사용할 수 없습니다.")
os.environ["CUDA_VISIBLE_DEVICES"] = "-1"


# ✅ 데이터셋 경로 (train/val 폴더구조 필요)
data_dir = './tomato_data'

# ✅ 하이퍼파라미터
img_size = (244, 244)
batch_size = 16
num_classes = 3
epochs = 100

# ✅ 데이터 전처리 및 증강

# ✅ train_datagen 정의
# train_datagen = ImageDataGenerator(
#     rescale=1./255,
#     rotation_range=20,
#     width_shift_range=0.1,
#     height_shift_range=0.1,
#     shear_range=0.1,
#     zoom_range=0.1,
#     horizontal_flip=True,
#     brightness_range=[0.8, 1.2],
#     fill_mode='nearest'
# )

train_datagen = ImageDataGenerator(
    rescale=1./255,
    rotation_range=10,          # 너무 많이 돌리면 도마도 형태 깨짐
    width_shift_range=0.05,     # 너무 많이 이동하면 정보 손실
    height_shift_range=0.05,
    shear_range=0.05,
    zoom_range=0.1,
    horizontal_flip=True,       # 좌우 대칭 OK
    brightness_range=[0.9, 1.1],
    fill_mode='nearest'
)

val_datagen = ImageDataGenerator(
    rescale=1./255
)

# ✅ train_generator 정의
train_generator = train_datagen.flow_from_directory(
    os.path.join(data_dir, 'train'),
    target_size=img_size,
    batch_size=batch_size,
    class_mode='sparse'
)

val_generator = val_datagen.flow_from_directory(
    os.path.join(data_dir, 'val'),
    target_size=img_size,
    batch_size=batch_size,
    class_mode='sparse'
)

# ✅ 모델: ResNet50 기반
base_model = tf.keras.applications.ResNet50(
    weights='imagenet',
    include_top=False,
    input_shape=img_size + (3,)
    # input_shape=(96, 96, 3)
)

base_model.trainable = True  # 처음엔 학습하지 않게 (원한다면 True로 바꿔도 됨)



model = models.Sequential([
    base_model,
    layers.GlobalAveragePooling2D(),
    # layers.MaxPool2D(2),
    # layers.Conv2D(32, kernel_size=3 , strides = 3, activation = 'relu'),
    # layers.MaxPool2D(2),
    # layers.Conv2D(64, kernel_size=2, activation='relu'),
    # layers.BatchNormalization(),
    # layers.Flatten(),
    layers.Dense(1024, activation='relu'),
    layers.Dropout(0.5),  # Dropout 더 강하게
    layers.Dense(num_classes, activation='softmax', dtype='float32')
])

input_tensor = Input(shape=(244, 244, 3))
output_tensor = model(input_tensor)  # model은 기존 Sequential 모델
model = Model(inputs=input_tensor, outputs=output_tensor)  # 새 Model 덮어쓰기

# ✅ 컴파일
model.compile(
    optimizer=tf.keras.optimizers.Adam(learning_rate=0.001),
    loss='sparse_categorical_crossentropy',
    metrics=['accuracy']
)

# ✅ 학습
# model.fit(
#     train_generator,
#     validation_data=val_generator,
#     epochs=epochs
# )
model.compile(optimizer="adam",
              loss="sparse_categorical_crossentropy",
              metrics=["accuracy"])
callbacks = [
    EarlyStopping(patience=5, restore_best_weights=True),
    ReduceLROnPlateau(patience=3, factor=0.5)
]
# ✅ 학습

history = model.fit(
    train_generator,
    validation_data=val_generator,
    epochs=epochs,
    callbacks=callbacks
)
# history = model.fit(
#     train_generator,
#     validation_data=val_generator,
#     epochs=epochs
# )

# ✅ 모델 저장

model.save('tomato_resnet01.h5')
print("모델 저장 완료: tomato_resnet01.h5")