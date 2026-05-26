#!/usr/bin/env python3
import argparse
import cv2
import numpy as np
import logging
import time
import sys
import os
from queue import Queue, Empty
from threading import Thread, Event
from abc import ABC, abstractmethod

os.environ['QT_QPA_PLATFORM'] = 'xcb'
os.makedirs('log', exist_ok=True)

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[logging.FileHandler('log/sensor.log'), logging.StreamHandler()]
)

class Sensor(ABC):
    @abstractmethod
    def get(self):
        pass


class SensorCam(Sensor):
    def __init__(self, device_name, resolution):
        self.logger = logging.getLogger('SensorCam')
        w, h = map(int, resolution.split('x'))
        
        try:
            if device_name.isdigit():
                self.cap = cv2.VideoCapture(int(device_name))
            else:
                self.cap = cv2.VideoCapture(device_name)
        except Exception as e:
            self.logger.error(f"Не удалось создать объект камеры: {e}")
            raise
        
        if not self.cap.isOpened():
            raise IOError(f"Камера {device_name} не открылась")
        
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, w)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, h)
        
        actual_w = int(self.cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        actual_h = int(self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        
        if actual_w != w or actual_h != h:
            self.logger.warning(f"Запрошено {w}x{h}, получено {actual_w}x{actual_h}")
        
        self.logger.info(f"Камера инициализирована: {actual_w}x{actual_h}")

    def get(self):
        ret, frame = self.cap.read()
        if not ret:
            raise IOError("Ошибка чтения кадра")
        return frame

    def __del__(self):
        if hasattr(self, 'cap'):
            self.cap.release()
            logging.getLogger('SensorCam').info("Камера освобождена")


class SensorX(Sensor):
    def __init__(self, delay: float):
        self._delay = delay
        self._data = 0

    def get(self) -> int:
        time.sleep(self._delay)
        self._data += 1
        return self._data


class SensorThread(Thread):
    def __init__(self, sensor, queue, name):
        super().__init__(name=name, daemon=True)
        self.sensor = sensor
        self.queue = queue
        self.stop_event = Event()

    def run(self):
        logger = logging.getLogger(f'Thread_{self.name}')
        logger.info("Запущен")
        
        while not self.stop_event.is_set():
            try:
                data = self.sensor.get()
                while not self.queue.empty():
                    try:
                        self.queue.get_nowait()
                    except Empty:
                        break
                self.queue.put(data)
            except Exception as e:
                logger.error(f"Ошибка: {e}")
                time.sleep(0.1)
        
        logger.info("Остановлен")

    def stop(self):
        self.stop_event.set()


class WindowImage:
    def __init__(self, fps):
        self.fps = fps
        self.window = 'Sensor Display'
        cv2.namedWindow(self.window, cv2.WINDOW_NORMAL)
        logging.getLogger('WindowImage').info(f"Окно создано, FPS: {fps}")

    def show(self, img):
        cv2.imshow(self.window, img)
        return cv2.waitKey(int(1000 / self.fps)) & 0xFF

    def __del__(self):
        try:
            cv2.destroyWindow(self.window)
        except:
            pass


class SensorDisplay:
    def __init__(self, camera_name, resolution, fps):
        self.logger = logging.getLogger('SensorDisplay')
        self.fps = fps
        self.frame_count = 0
        self.last_frame = None
        self.last_values = [0, 0, 0]
        self.running = True

        self.camera_q = Queue(maxsize=2)
        self.sensor_qs = [Queue(maxsize=2) for _ in range(3)]

        self.logger.info("Инициализация датчиков...")
        self.camera = SensorCam(camera_name, resolution)
        self.sensors = [SensorX(0.01), SensorX(0.1), SensorX(1.0)]

        self.threads = [SensorThread(self.camera, self.camera_q, "Camera")]
        for i, s in enumerate(self.sensors):
            self.threads.append(SensorThread(s, self.sensor_qs[i], f"Sensor{i}"))

        self.window = WindowImage(fps)
        self.logger.info("Все датчики инициализированы")

    def start(self):
        for t in self.threads:
            t.start()
        self.logger.info("Все потоки запущены")

    def stop(self):
        self.logger.info("Остановка...")
        self.running = False
        for t in self.threads:
            t.stop()
        for t in self.threads:
            t.join(timeout=1)
        self.logger.info("Программа остановлена")

    def run(self):
        last_display = time.time()
        while self.running:
            try:
                self.last_frame = self.camera_q.get_nowait()
                self.frame_count += 1
            except Empty:
                pass
            
            for i in range(3):
                try:
                    self.last_values[i] = self.sensor_qs[i].get_nowait()
                except Empty:
                    pass

            if time.time() - last_display >= 1.0 / self.fps:
                frame = self.last_frame if self.last_frame is not None else np.zeros((480, 640, 3), dtype=np.uint8)
                frame = cv2.flip(frame, 1)
                
                overlay = frame.copy()
                cv2.rectangle(overlay, (10, 10), (350, 140), (0, 0, 0), -1)
                cv2.addWeighted(overlay, 0.3, frame, 0.7, 0, frame)
                
                for i, (label, val) in enumerate(zip(["100Hz:", "10Hz:", "1Hz:"], self.last_values)):
                    cv2.putText(frame, f"{label} {val}", (20, 40 + i*30), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0,255,0), 2)
                
                cv2.putText(frame, f"FPS: {self.fps}", (frame.shape[1]-100, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255,255,255), 1)
                cv2.putText(frame, "Press 'q'", (frame.shape[1]-100, frame.shape[0]-20), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0,0,255), 1)
                
                if self.window.show(frame) == ord('q'):
                    break
                last_display = time.time()
            
            time.sleep(0.001)
        
        self.stop()


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--camera', default='0')
    p.add_argument('--resolution', default='640x480')
    p.add_argument('--fps', type=float, default=30.0)
    args = p.parse_args()

    print(f"Камера: {args.camera}")
    print(f"Разрешение: {args.resolution}")
    print(f"FPS: {args.fps}")

    try:
        app = SensorDisplay(args.camera, args.resolution, args.fps)
    except Exception as e:
        logging.error(f"Ошибка инициализации: {e}")
        print(f"\n Ошибка: {e}")
        print(" Лог сохранен в log/sensor.log")
        sys.exit(1)

    app.start()
    app.run()
    
    print("\n Программа завершена")


if __name__ == "__main__":
    main()