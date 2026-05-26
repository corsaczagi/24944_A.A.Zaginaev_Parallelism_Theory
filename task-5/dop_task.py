import os

# FIX для Linux / Wayland
os.environ["QT_QPA_PLATFORM"] = "xcb"

import cv2
import time
import torch
from threading import Thread, Lock
from ultralytics import YOLO


# Оптимизация CPU
torch.set_num_threads(4)


class RealtimePoseDetector:

    def __init__(self):

        print("Загрузка модели...")

        # Более быстрая модель для realtime
        self.model = YOLO("yolov8n-pose.pt")

        self.model.to("cpu")

        print("Модель загружена\n")

        self.running = True

        # Последний кадр с камеры
        self.latest_frame = None

        # Последний обработанный кадр
        self.processed_frame = None

        self.frame_lock = Lock()

        # FPS
        self.fps = 0
        self.frame_counter = 0
        self.last_time = time.time()

        # Один worker thread
        self.worker_thread = Thread(target=self.worker)

        self.worker_thread.daemon = True

        self.worker_thread.start()

    def worker(self):

        while self.running:

            frame = None

            # Берем только свежий кадр
            with self.frame_lock:

                if self.latest_frame is not None:

                    frame = self.latest_frame.copy()

                    # удаляем старый кадр
                    self.latest_frame = None

            if frame is None:

                time.sleep(0.001)

                continue

            # Уменьшаем размер для ускорения
            small = cv2.resize(frame, (416, 320))

            # Инференс
            results = self.model(
                small,
                verbose=False
            )

            # Стандартная YOLO отрисовка
            output = results[0].plot()

            # Fullscreen upscale
            output = cv2.resize(output, (1280, 720))

            with self.frame_lock:

                self.processed_frame = output

    def run(self):

        cap = cv2.VideoCapture(0)

        if not cap.isOpened():

            print("Ошибка открытия камеры")

            return

        # Минимальный буфер
        cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)

        cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)

        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

        print("Q или ESC для выхода\n")

        window_name = "Realtime YOLOv8 Pose"

        # Окно можно растягивать
        cv2.namedWindow(window_name, cv2.WINDOW_NORMAL)

        # Полноэкранный режим
        cv2.setWindowProperty(
            window_name,
            cv2.WND_PROP_FULLSCREEN,
            cv2.WINDOW_FULLSCREEN
        )

        while self.running:

            ret, frame = cap.read()

            if not ret:
                break

            # Сохраняем только новый кадр
            with self.frame_lock:

                self.latest_frame = frame.copy()

            # Показываем обработанный кадр
            with self.frame_lock:

                if self.processed_frame is not None:

                    display = self.processed_frame.copy()

                else:

                    display = cv2.resize(frame, (1280, 720))

            # FPS
            self.frame_counter += 1

            current_time = time.time()

            if current_time - self.last_time >= 1.0:

                self.fps = self.frame_counter / (
                    current_time - self.last_time
                )

                self.frame_counter = 0

                self.last_time = current_time

            # Текст FPS
            cv2.putText(
                display,
                f"FPS: {self.fps:.1f}",
                (30, 50),
                cv2.FONT_HERSHEY_SIMPLEX,
                1.2,
                (0, 255, 0),
                3
            )

            cv2.putText(
                display,
                "Q / ESC - EXIT",
                (30, 100),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.8,
                (255, 255, 255),
                2
            )

            cv2.imshow(window_name, display)

            key = cv2.waitKey(1)

            # Выход
            if key == ord('q') or key == 27:

                print("\nВыход...")

                self.running = False

                break

        cap.release()

        cv2.destroyAllWindows()
        self.worker_thread.join()

        print(f"\nИтоговый FPS: {self.fps:.1f}")


def main():

    print("=" * 50)

    print("REALTIME YOLOv8 POSE")

    print("=" * 50)

    detector = RealtimePoseDetector()

    detector.run()


if __name__ == "__main__":

    main()