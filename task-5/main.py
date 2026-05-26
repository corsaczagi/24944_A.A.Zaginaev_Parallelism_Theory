import cv2
import numpy as np
from ultralytics import YOLO
import time
from queue import Queue, Empty
from threading import Thread, Lock
import os

model_path = 'yolov8s-pose.pt'
if not os.path.exists(model_path):
    print("скачивание модели...")
    model = YOLO(model_path)
    del model
    print("модель загружена")

TARGET_SIZE = (640, 480)
MAX_FRAMES = 200
FRAME_STEP = 1
TOTAL_TO_PROCESS = 200

def resize_frame(frame, target_size=TARGET_SIZE):
    h, w = frame.shape[:2]
    if w != target_size[0] or h != target_size[1]:
        frame = cv2.resize(frame, target_size, interpolation=cv2.INTER_LINEAR)
    return frame

class WorkerThread:
    def __init__(self, worker_id):
        self.worker_id = worker_id
        self.model = YOLO('yolov8s-pose.pt')
        self.model.to('cpu')
        self.queue = Queue()
        self.results = {}
        self.lock = Lock()
        self.running = True
        self.task_count = 0
    
    def run(self):
        while self.running:
            try:
                idx, frame = self.queue.get(timeout=1.0)
                if idx is None:
                    break
                
                small_frame = resize_frame(frame)
                results = self.model(small_frame, verbose=False)
                if len(results) > 0:
                    small_frame = results[0].plot()
                frame = resize_frame(small_frame, (frame.shape[1], frame.shape[0]))
                
                with self.lock:
                    self.results[idx] = frame
                
                self.queue.task_done()
                self.task_count += 1
                
            except Empty:
                continue
            except Exception:
                self.queue.task_done()
    
    def stop(self):
        self.running = False
        self.queue.put((None, None))

def process_video(video_path, output_path, num_workers):
    cap = cv2.VideoCapture(video_path)
    
    fps = int(cap.get(cv2.CAP_PROP_FPS))
    orig_width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    orig_height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    
    fourcc = cv2.VideoWriter_fourcc(*'mp4v')
    out = cv2.VideoWriter(output_path, fourcc, fps, (orig_width, orig_height))
    
    workers = []
    for i in range(num_workers):
        worker = WorkerThread(i)
        t = Thread(target=worker.run)
        worker.thread = t
        workers.append(worker)
        t.start()
    
    start = time.time()
    frame_count = 0
    sent_count = 0
    
    while True:
        ret, frame = cap.read()
        if not ret:
            break
        
        if frame_count % FRAME_STEP == 0 and sent_count < TOTAL_TO_PROCESS:
            workers[sent_count % num_workers].queue.put((sent_count, frame.copy()))
            sent_count += 1
        
        frame_count += 1
        if sent_count >= TOTAL_TO_PROCESS:
            break
    
    for w in workers:
        w.queue.join()
    
    for w in workers:
        w.stop()
    
    for w in workers:
        w.thread.join()
    
    all_results = {}
    for w in workers:
        with w.lock:
            all_results.update(w.results)
    
    for i in range(sent_count):
        if i in all_results:
            out.write(all_results[i])
    
    elapsed = time.time() - start
    cap.release()
    out.release()
    
    return sent_count, elapsed


def main():
    print("В процессе работы")

    video_path = "test-video.mp4"
    
    if not os.path.exists(video_path):
        print(f"ошибка: видео {video_path} не найдено")
        return
    
    base_frames, base_time = process_video(video_path, "output_base.mp4", 1)
    
    results = [(1, base_time, base_frames, 1.0)]
    
    for workers in [2, 4, 8, 16]:
        frames, elapsed = process_video(video_path, f"output_{workers}.mp4", workers)
        speedup = base_time / elapsed
        results.append((workers, elapsed, frames, speedup))
    
    best = max(results[1:], key=lambda x: x[3])
    
    with open("results.txt", "w") as f:
        f.write("лабораторная работа: ускорение инференса yolov8-pose\n\n")
        f.write(f"обработано кадров: {base_frames}\n")
        f.write(f"размер обработки: {TARGET_SIZE[0]}x{TARGET_SIZE[1]}\n\n")
        f.write(f"{'потоков':>8} | {'время':>10} | {'ускорение':>10}\n")
        for workers, elapsed, frames, speedup in results:
            f.write(f"{workers:8} | {elapsed:10.2f} | {speedup:10.2f}x\n")
        f.write(f"\nоптимальное число потоков: {best[0]}\n")

    print("\nрезультаты сохранены в results.txt")

if __name__ == '__main__':
    main()