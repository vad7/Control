import requests
import time
import statistics
import argparse
from collections import deque
from urllib.parse import urljoin

class DownloadMonitor:
    def __init__(self, base_url, filename, interval=5, history_size=10):
        self.base_url = base_url.rstrip('/') + '/'
        self.filename = filename
        self.full_url = urljoin(self.base_url, filename)
        self.interval = interval
        self.speeds = deque(maxlen=history_size)
        self.failures = 0
        self.successes = 0
    
    def format_speed(self, speed):
        units = ["B/s", "KB/s", "MB/s", "GB/s"]
        unit_index = 0
        while speed >= 1024 and unit_index < len(units)-1:
            speed /= 1024
            unit_index += 1
        return f"{speed:.2f} {units[unit_index]}"
    
    def run(self):
        print(f"Начинаем мониторинг файла: {self.filename}")
        print(f"Полный URL: {self.full_url}")
        print(f"Интервал проверки: {self.interval} секунд")
        print("-" * 50)
        
        while True:
            try:
                start_time = time.perf_counter()
                response = requests.get(self.full_url, timeout=10)
                end_time = time.perf_counter()
                
                download_time = end_time - start_time
                
                if response.status_code == 200:
                    file_size = len(response.content)
                    speed_bytes_sec = file_size / download_time
                    self.speeds.append(speed_bytes_sec)
                    self.successes += 1
                    
                    # Статистика
                    avg_speed = statistics.mean(self.speeds) if self.speeds else 0
                    max_speed = max(self.speeds) if self.speeds else 0
                    
                    print(f"Успешно: {file_size} байт за {download_time:.3f} сек")
                    print(f"Текущая скорость: {self.format_speed(speed_bytes_sec)}")
                    print(f"Средняя скорость: {self.format_speed(avg_speed)}")
                    print(f"Максимальная скорость: {self.format_speed(max_speed)}")
                    print(f"Успешных запросов: {self.successes}, Неудач: {self.failures}")
                    
                else:
                    self.failures += 1
                    print(f"Ошибка HTTP: {response.status_code}")
                    
            except requests.exceptions.RequestException as e:
                self.failures += 1
                print(f"Сетевая ошибка: {e}")
            
            print("-" * 50)
            time.sleep(self.interval)

def main():
    parser = argparse.ArgumentParser(description='Мониторинг скорости загрузки файла с веб-сервера')
    parser.add_argument('filename', help='Имя файла для мониторинга (например: testfile.dat)')
    parser.add_argument('--base-url', default='http://192.168.0.7', 
                       help='Базовый URL сервера (по умолчанию: http://192.168.0.7)')
    parser.add_argument('--interval', type=int, default=0, 
                       help='Интервал между запросами в секундах (по умолчанию: 0)')
    parser.add_argument('--history', type=int, default=10, 
                       help='Размер истории для статистики (по умолчанию: 10)')
    
    args = parser.parse_args()
    
    monitor = DownloadMonitor(
        base_url=args.base_url,
        filename=args.filename,
        interval=args.interval,
        history_size=args.history
    )
    
    try:
        monitor.run()
    except KeyboardInterrupt:
        print("\nМониторинг остановлен пользователем")

if __name__ == "__main__":
    main()
