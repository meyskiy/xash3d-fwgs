#!/usr/bin/env python3
"""
Скрипт для регистрации фейковых серверов в мастер-сервере Xash3D FWGS

Протокол регистрации:
1. Сервер отправляет S2M_HEARTBEAT с challenge на мастер
2. Мастер отвечает M2S_CHALLENGE с master_challenge и server_challenge
3. Сервер отправляет S2M_INFO с информацией о сервере
"""

import socket
import struct
import random
import time
import argparse
from typing import Optional, Tuple
import sys


class FakeServerRegistrar:
    """Класс для регистрации фейковых серверов в мастер-сервере"""
    
    # Константы протокола
    CONNECTIONLESS_HEADER = b'\xff\xff\xff\xff'
    S2M_HEARTBEAT = b'q\xff'
    S2M_INFO = b'0\n'
    M2S_CHALLENGE = b's'
    
    def __init__(self, master_host: str, master_port: int = 27010, debug: bool = True):
        """
        Инициализация регистратора
        
        Args:
            master_host: Адрес мастер-сервера
            master_port: Порт мастер-сервера (по умолчанию 27010)
            debug: Включить отладочный вывод
        """
        self.debug = debug
        
        # Резолвим адрес мастера
        if self.debug:
            print(f"[DEBUG] Резолвинг {master_host}...")
        try:
            resolved_ip = socket.gethostbyname(master_host)
            if self.debug:
                print(f"[DEBUG] {master_host} -> {resolved_ip}")
            self.master_addr = (resolved_ip, master_port)
        except socket.gaierror as e:
            print(f"[✗] Ошибка резолвинга {master_host}: {e}")
            sys.exit(1)
            
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(5.0)
        
        if self.debug:
            print(f"[DEBUG] Создан UDP сокет")
        
    def _send_packet(self, data: bytes, addr: Optional[Tuple[str, int]] = None):
        """Отправка пакета"""
        if addr is None:
            addr = self.master_addr
        self.sock.sendto(data, addr)
        print(f"[→] Отправлено {len(data)} байт на {addr[0]}:{addr[1]}")
        print(f"    Данные: {data[:80]}")
        
    def _recv_packet(self, timeout: float = 5.0) -> Tuple[bytes, Tuple[str, int]]:
        """Получение пакета"""
        self.sock.settimeout(timeout)
        if self.debug:
            print(f"[DEBUG] Ожидание пакета (timeout={timeout}s)...")
        try:
            data, addr = self.sock.recvfrom(2048)
            print(f"[←] Получено {len(data)} байт от {addr[0]}:{addr[1]}")
            print(f"    Данные: {data[:80]}")
            if self.debug:
                print(f"[DEBUG] Hex: {data[:40].hex()}")
            return data, addr
        except socket.timeout:
            if self.debug:
                print(f"[DEBUG] Таймаут истек, пакетов не получено")
            raise
        
    def _build_info_string(self, **kwargs) -> bytes:
        """
        Построение Quake info string
        
        Формат: \\key1\\value1\\key2\\value2...
        """
        parts = []
        for key, value in kwargs.items():
            parts.append(f"\\{key}\\{value}")
        return ''.join(parts).encode('latin-1')
        
    def send_heartbeat(self, server_port: int, bind_ip: str = '0.0.0.0') -> int:
        """
        Отправка heartbeat на мастер-сервер
        
        Args:
            server_port: Порт фейкового сервера
            
        Returns:
            server_challenge - случайное число для идентификации
        """
        # Привязываем сокет к порту сервера
        try:
            bind_addr = (bind_ip, server_port)
            self.sock.bind(bind_addr)
            actual_addr = self.sock.getsockname()
            print(f"[*] Сокет привязан к {actual_addr[0]}:{actual_addr[1]}")
            if self.debug:
                print(f"[DEBUG] Bind address: {bind_addr}")
        except OSError as e:
            print(f"[!] Ошибка привязки к {bind_ip}:{server_port}: {e}")
            print(f"[*] Используем случайный порт")
            try:
                self.sock.bind((bind_ip, 0))
                actual_addr = self.sock.getsockname()
                print(f"[*] Привязан к {actual_addr[0]}:{actual_addr[1]}")
            except OSError as e2:
                print(f"[✗] Критическая ошибка привязки: {e2}")
                raise
        
        # Генерируем случайный challenge
        server_challenge = random.randint(0, 0x7FFFFFFF)
        
        # Формируем пакет: \xff\xff\xff\xff + "q\xff" + challenge (4 байта LE)
        packet = self.CONNECTIONLESS_HEADER + self.S2M_HEARTBEAT
        packet += struct.pack('<I', server_challenge)
        
        print(f"\n[*] Отправка HEARTBEAT (challenge: 0x{server_challenge:08x})")
        self._send_packet(packet)
        
        return server_challenge
        
    def receive_challenge(self) -> Tuple[int, int]:
        """
        Получение challenge от мастер-сервера
        
        Returns:
            (master_challenge, server_challenge)
        """
        print(f"\n[*] Ожидание M2S_CHALLENGE от мастера...")
        data, addr = self._recv_packet()
        
        # Проверяем заголовок
        if not data.startswith(self.CONNECTIONLESS_HEADER):
            raise ValueError("Неверный заголовок пакета")
            
        # Проверяем тип пакета
        if not data[4:5] == self.M2S_CHALLENGE:
            raise ValueError(f"Неожиданный тип пакета: {data[4:10]}")
            
        # Парсим challenge'ы
        # Формат: \xff\xff\xff\xff + "s" + master_challenge (4 байта) + server_challenge (4 байта)
        if len(data) >= 13:
            master_challenge = struct.unpack('<I', data[5:9])[0]
            server_challenge = struct.unpack('<I', data[9:13])[0]
        else:
            # Старый формат без server_challenge
            master_challenge = struct.unpack('<I', data[5:9])[0]
            server_challenge = 0
            
        print(f"[✓] Получен M2S_CHALLENGE:")
        print(f"    master_challenge: 0x{master_challenge:08x}")
        print(f"    server_challenge: 0x{server_challenge:08x}")
        
        return master_challenge, server_challenge
        
    def send_server_info(self, master_challenge: int, server_info: dict):
        """
        Отправка информации о сервере
        
        Args:
            master_challenge: Challenge полученный от мастера
            server_info: Словарь с информацией о сервере
        """
        # Устанавливаем обязательные поля
        info = {
            'protocol': '49',
            'challenge': str(master_challenge),
            'players': server_info.get('players', '0'),
            'max': server_info.get('max', '32'),
            'bots': server_info.get('bots', '0'),
            'gamedir': server_info.get('gamedir', 'valve'),
            'map': server_info.get('map', 'crossfire'),
            'type': server_info.get('type', 'd'),  # d=dedicated, l=listen
            'password': server_info.get('password', '0'),
            'os': 'w',  # windows
            'secure': '0',
            'lan': '0',
            'version': server_info.get('version', '0.20'),
            'region': '255',
            'product': server_info.get('gamedir', 'valve'),
            'nat': server_info.get('nat', '0'),
        }
        
        # Формируем info string
        info_string = self._build_info_string(**info)
        
        # Формируем пакет: \xff\xff\xff\xff + "0\n" + info_string
        packet = self.CONNECTIONLESS_HEADER + self.S2M_INFO + info_string
        
        print(f"\n[*] Отправка S2M_INFO:")
        print(f"    Информация о сервере: {info}")
        self._send_packet(packet)
        
    def register_server(self, server_port: int, server_info: dict, bind_ip: str = '0.0.0.0') -> bool:
        """
        Полная регистрация сервера на мастере
        
        Args:
            server_port: Порт фейкового сервера
            server_info: Информация о сервере
            
        Returns:
            True если регистрация успешна
        """
        try:
            # Шаг 1: Отправляем heartbeat
            server_challenge = self.send_heartbeat(server_port, bind_ip)
            
            # Шаг 2: Получаем challenge от мастера
            master_challenge, returned_challenge = self.receive_challenge()
            
            # Проверяем, что мастер вернул наш challenge
            if returned_challenge != 0 and returned_challenge != server_challenge:
                print(f"[!] Предупреждение: мастер вернул другой challenge")
                print(f"    Ожидали: 0x{server_challenge:08x}, получили: 0x{returned_challenge:08x}")
            
            # Шаг 3: Отправляем информацию о сервере
            self.send_server_info(master_challenge, server_info)
            
            print(f"\n[✓] Сервер успешно зарегистрирован на мастере!")
            return True
            
        except socket.timeout:
            print(f"\n[✗] Таймаут ожидания ответа от мастера")
            if self.debug:
                print(f"[DEBUG] Мастер не ответил в течение {self.sock.gettimeout()}s")
                print(f"[DEBUG] Проверьте:")
                print(f"[DEBUG]   - Доступность мастера: {self.master_addr}")
                print(f"[DEBUG]   - Firewall/NAT настройки")
                print(f"[DEBUG]   - Правильность порта")
            return False
        except Exception as e:
            print(f"\n[✗] Ошибка регистрации: {e}")
            import traceback
            traceback.print_exc()
            return False
            
    def close(self):
        """Закрытие сокета"""
        self.sock.close()


def main():
    parser = argparse.ArgumentParser(
        description='Регистрация фейковых серверов в мастер-сервере Xash3D FWGS'
    )
    parser.add_argument(
        '--master',
        default='mentality.rip',
        help='Адрес мастер-сервера (по умолчанию: mentality.rip)'
    )
    parser.add_argument(
        '--port',
        type=int,
        default=27010,
        help='Порт мастер-сервера (по умолчанию: 27010)'
    )
    parser.add_argument(
        '--server-port',
        type=int,
        default=27015,
        help='Порт фейкового сервера (по умолчанию: 27015)'
    )
    parser.add_argument(
        '--count',
        type=int,
        default=1,
        help='Количество серверов для регистрации (по умолчанию: 1)'
    )
    parser.add_argument(
        '--gamedir',
        default='valve',
        help='Игровая директория (по умолчанию: valve)'
    )
    parser.add_argument(
        '--map',
        default='crossfire',
        help='Название карты (по умолчанию: crossfire)'
    )
    parser.add_argument(
        '--hostname',
        default='Fake Server',
        help='Имя сервера (по умолчанию: Fake Server)'
    )
    parser.add_argument(
        '--max-players',
        type=int,
        default=32,
        help='Максимум игроков (по умолчанию: 32)'
    )
    parser.add_argument(
        '--interval',
        type=float,
        default=1.0,
        help='Интервал между регистрациями в секундах (по умолчанию: 1.0)'
    )
    parser.add_argument(
        '--bind-ip',
        default='0.0.0.0',
        help='IP адрес для привязки (по умолчанию: 0.0.0.0)'
    )
    parser.add_argument(
        '--spoof-ip',
        help='IP адрес для подделки (требует raw sockets, только Linux)'
    )
    parser.add_argument(
        '--timeout',
        type=float,
        default=5.0,
        help='Таймаут ожидания ответа в секундах (по умолчанию: 5.0)'
    )
    parser.add_argument(
        '--no-debug',
        action='store_true',
        help='Отключить отладочный вывод'
    )
    
    args = parser.parse_args()
    
    if args.spoof_ip:
        print("[!] ВНИМАНИЕ: IP spoofing требует raw sockets и root привилегий")
        print("[!] Эта функция работает только на Linux")
        print("[!] Используйте на свой риск!")
        print()
    
    print("=" * 70)
    print("  Регистратор фейковых серверов Xash3D FWGS")
    print("=" * 70)
    print(f"Мастер-сервер: {args.master}:{args.port}")
    print(f"Количество серверов: {args.count}")
    print(f"Начальный порт: {args.server_port}")
    print(f"Bind IP: {args.bind_ip}")
    if args.spoof_ip:
        print(f"Spoof IP: {args.spoof_ip}")
    print(f"Таймаут: {args.timeout}s")
    print(f"Отладка: {'Выкл' if args.no_debug else 'Вкл'}")
    print("=" * 70)
    
    success_count = 0
    
    for i in range(args.count):
        server_port = args.server_port + i
        
        print(f"\n{'=' * 70}")
        print(f"  Регистрация сервера #{i+1}/{args.count} на порту {server_port}")
        print(f"{'=' * 70}")
        
        server_info = {
            'gamedir': args.gamedir,
            'map': args.map,
            'max': str(args.max_players),
            'players': str(random.randint(0, args.max_players)),
            'bots': '0',
            'type': 'd',
            'password': '0',
            'version': '0.20',
            'nat': '0',
        }
        
        registrar = FakeServerRegistrar(args.master, args.port, debug=not args.no_debug)
        registrar.sock.settimeout(args.timeout)
        
        try:
            if registrar.register_server(server_port, server_info, args.bind_ip):
                success_count += 1
        finally:
            registrar.close()
            
        # Пауза между регистрациями
        if i < args.count - 1:
            time.sleep(args.interval)
    
    print(f"\n{'=' * 70}")
    print(f"  Завершено: {success_count}/{args.count} серверов зарегистрировано")
    print(f"{'=' * 70}")


if __name__ == '__main__':
    main()
