#!/usr/bin/env python3
"""
Скрипт для заполнения слотов на серверах xash3d
Отправляет запросы на подключение к серверу для заполнения слотов "воздухом"
"""

import socket
import struct
import time
import random
import hashlib
import threading
import sys
from typing import Optional, Tuple

# Константы протокола
PROTOCOL_VERSION = 49
PROTOCOL_GOLDSRC_VERSION = 48
OOB_HEADER = b"\xff\xff\xff\xff"
C2S_GETCHALLENGE = b"getchallenge steam\n"
C2S_CONNECT = b"connect"
S2C_CHALLENGE = b"challenge"
S2C_CONNECTION = b"client_connect"

# Настройки по умолчанию
DEFAULT_NICKNAME_PREFIX = "eBash3D better Nash3D kekw"
DEFAULT_MAX_BOTS = 32
DEFAULT_DELAY = 0.1  # Задержка между подключениями в секундах


class Xash3DClient:
    """Класс для подключения к серверу xash3d или GoldSource"""
    
    def __init__(self, server_ip: str, server_port: int, nickname: str = None, qport: int = None, password: str = None, debug: bool = False, goldsource: bool = False):
        self.server_ip = server_ip
        self.server_port = server_port
        self.nickname = nickname or f"{DEFAULT_NICKNAME_PREFIX}{random.randint(1000, 9999)}"
        self.qport = qport or random.randint(1, 65535)
        self.password = password
        self.sock = None
        self.connected = False
        self.challenge = None
        self.debug = debug
        self.goldsource = goldsource
        self.auto_reconnect = False
        self.running = True
        
    def _create_socket(self) -> socket.socket:
        """Создает UDP сокет"""
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(5.0)
        return sock
    
    def _generate_uuid(self) -> str:
        """Генерирует UUID (MD5 хеш)"""
        random_data = f"{random.randint(0, 2**32)}{time.time()}{self.qport}"
        return hashlib.md5(random_data.encode()).hexdigest()
    
    def _create_protinfo(self) -> str:
        """Создает protinfo строку"""
        if self.goldsource:
            # Для GoldSource используем формат с CD Key
            # protinfo содержит: prot=3 (steam auth), unique=0xffffffff, raw=steam, cdkey=<md5>
            # CD Key - это MD5 хеш случайного числа (как в CL_GetCDKey)
            import random as rnd
            random_key = rnd.randint(0, 0x7ffffffe)
            key_str = str(random_key)
            cdkey_hash = hashlib.md5(key_str.encode()).hexdigest().lower()
            return f"\\prot\\3\\unique\\4294967295\\raw\\steam\\cdkey\\{cdkey_hash}"
        else:
            # Для Xash3D
            uuid = self._generate_uuid()
            ext = 1  # NET_EXT_SPLITSIZE
            # Убираем завершающий обратный слеш - он может вызывать проблемы при парсинге
            return f"\\uuid\\{uuid}\\qport\\{self.qport}\\ext\\{ext}"
    
    def _create_userinfo(self) -> str:
        """Создает userinfo строку"""
        userinfo = f"\\name\\{self.nickname}"
        if self.password:
            userinfo += f"\\password\\{self.password}"
        return userinfo
    
    def _send_oob_packet(self, sock: socket.socket, data: bytes):
        """Отправляет out-of-band пакет"""
        packet = OOB_HEADER + data
        if self.debug:
            print(f"    [DEBUG] Отправка пакета ({len(packet)} байт): {packet[:100]}")
        sock.sendto(packet, (self.server_ip, self.server_port))
    
    def _recv_oob_packet(self, sock: socket.socket, timeout: float = 2.0) -> Optional[bytes]:
        """Принимает out-of-band пакет"""
        try:
            sock.settimeout(timeout)
            data, addr = sock.recvfrom(2048)
            if self.debug:
                print(f"    [DEBUG] Получен пакет от {addr[0]}:{addr[1]} ({len(data)} байт)")
                print(f"    [DEBUG] Данные (hex): {data.hex()[:200]}")
                print(f"    [DEBUG] Данные (ascii): {data[:100]}")
            
            if addr[0] == self.server_ip and addr[1] == self.server_port:
                if data.startswith(OOB_HEADER):
                    payload = data[len(OOB_HEADER):]
                    if self.debug:
                        print(f"    [DEBUG] Payload: {payload[:100]}")
                    return payload
                else:
                    if self.debug:
                        print(f"    [DEBUG] Пакет не начинается с OOB_HEADER")
            else:
                if self.debug:
                    print(f"    [DEBUG] Пакет от неожиданного адреса: {addr[0]}:{addr[1]} (ожидался {self.server_ip}:{self.server_port})")
        except socket.timeout:
            if self.debug:
                print(f"    [DEBUG] Таймаут при получении пакета")
        except Exception as e:
            print(f"  Ошибка получения пакета: {e}")
            if self.debug:
                import traceback
                traceback.print_exc()
        return None
    
    def get_challenge(self) -> Optional[int]:
        """Получает challenge от сервера"""
        if self.debug:
            print(f"    [DEBUG] Получение challenge...")
        sock = self._create_socket()
        try:
            # Отправляем запрос challenge
            if self.debug:
                print(f"    [DEBUG] Отправка getchallenge")
            self._send_oob_packet(sock, C2S_GETCHALLENGE)
            
            # Ждем ответ (для GoldSource увеличиваем таймаут)
            timeout = 5.0 if self.goldsource else 2.0
            response = self._recv_oob_packet(sock, timeout=timeout)
            if response:
                if self.debug:
                    print(f"    [DEBUG] Получен ответ на getchallenge: {response}")
                # Парсим ответ: "challenge <value>\n" для Xash3D или "A00000000 <value> ..." для GoldSource
                try:
                    # Удаляем все нулевые байты и пробелы
                    response_str = response.decode('utf-8', errors='ignore').replace('\x00', '').strip()
                    if self.debug:
                        print(f"    [DEBUG] Ответ (decoded): '{response_str}'")
                    parts = [p for p in response_str.split() if p]  # Убираем пустые элементы
                    if self.debug:
                        print(f"    [DEBUG] Части ответа: {parts}")
                        print(f"    [DEBUG] Первая часть: '{parts[0] if len(parts) > 0 else 'N/A'}'")
                    
                    # Проверяем формат Xash3D: "challenge <value>"
                    if len(parts) >= 2 and parts[0] == S2C_CHALLENGE.decode('utf-8'):
                        try:
                            challenge = int(parts[1])
                            self.challenge = challenge
                            if self.debug:
                                print(f"    [DEBUG] Challenge получен (Xash3D): {challenge}")
                            return challenge
                        except ValueError as e:
                            if self.debug:
                                print(f"    [DEBUG] Ошибка парсинга challenge: {e}")
                    # Проверяем формат GoldSource: "A00000000 <challenge> ..."
                    elif len(parts) >= 2 and parts[0].startswith("A00000000"):
                        try:
                            challenge = int(parts[1])
                            self.challenge = challenge
                            if self.debug:
                                print(f"    [DEBUG] Challenge получен (GoldSource): {challenge}")
                            return challenge
                        except ValueError as e:
                            if self.debug:
                                print(f"    [DEBUG] Ошибка парсинга challenge (GoldSource): {e}")
                    else:
                        if self.debug:
                            print(f"    [DEBUG] Неожиданный формат ответа. Ожидался 'challenge <value>' или 'A00000000 <value>'")
                            print(f"    [DEBUG] Первая часть: '{parts[0] if len(parts) > 0 else 'N/A'}'")
                            print(f"    [DEBUG] Сравнение с 'A00000000': {parts[0] == 'A00000000' if len(parts) > 0 else False}")
                except Exception as e:
                    if self.debug:
                        print(f"    [DEBUG] Ошибка декодирования ответа: {e}")
            else:
                if self.debug:
                    print(f"    [DEBUG] Не получен ответ на getchallenge")
        except Exception as e:
            print(f"  Ошибка получения challenge: {e}")
            if self.debug:
                import traceback
                traceback.print_exc()
        finally:
            sock.close()
        return None
    
    def connect(self) -> bool:
        """Подключается к серверу"""
        if self.connected:
            return True
        
        if self.debug:
            print(f"    [DEBUG] Начало подключения...")
        
        # Получаем challenge
        challenge = self.get_challenge()
        if challenge is None:
            if self.debug:
                print(f"    [DEBUG] Не удалось получить challenge")
            return False
        
        if self.debug:
            print(f"    [DEBUG] Challenge получен: {challenge}")
        
        sock = self._create_socket()
        try:
            # Создаем protinfo и userinfo
            protinfo = self._create_protinfo()
            userinfo = self._create_userinfo()
            
            if self.debug:
                print(f"    [DEBUG] Protinfo: {protinfo}")
                print(f"    [DEBUG] Userinfo: {userinfo}")
            
            # Формируем пакет подключения
            # Формат: connect <version> <challenge> "<protinfo>" "<userinfo>\n"
            # В реальном клиенте используется Netchan_OutOfBandPrint с %s, 
            # что означает что строки передаются как есть, без дополнительного экранирования
            # Но внутри кавычек кавычки должны быть экранированы как \"
            # Обратные слеши передаются как есть - парсер их не экранирует специально
            protinfo_escaped = protinfo.replace('"', '\\"')
            userinfo_escaped = userinfo.replace('"', '\\"')
            
            # Выбираем версию протокола в зависимости от режима
            protocol_version = PROTOCOL_GOLDSRC_VERSION if self.goldsource else PROTOCOL_VERSION
            connect_data = f"{C2S_CONNECT.decode('utf-8')} {protocol_version} {challenge} \"{protinfo_escaped}\" \"{userinfo_escaped}\"\n"
            connect_packet = connect_data.encode('utf-8')
            
            # Проверяем, что пакет правильно сформирован
            if self.debug:
                # Проверяем наличие завершающего \n
                if not connect_packet.endswith(b'\n'):
                    print(f"    [DEBUG] ВНИМАНИЕ: Пакет не заканчивается на \\n!")
                    # Добавляем \n если его нет
                    connect_packet += b'\n'
                # Проверяем, что в пакете есть закрывающие кавычки
                if b'"' not in connect_packet[-20:]:
                    print(f"    [DEBUG] ВНИМАНИЕ: В конце пакета нет закрывающих кавычек!")
                    print(f"    [DEBUG] Последние 30 байт пакета: {connect_packet[-30:]}")
                    print(f"    [DEBUG] Последние 30 байт (ascii): {connect_packet[-30:]}")
            
            if self.debug:
                print(f"    [DEBUG] Protinfo (original): {protinfo}")
                print(f"    [DEBUG] Protinfo (escaped): {protinfo_escaped}")
                print(f"    [DEBUG] Userinfo (original): {userinfo}")
                print(f"    [DEBUG] Userinfo (escaped): {userinfo_escaped}")
                print(f"    [DEBUG] Connect data (full): {connect_data}")
                print(f"    [DEBUG] Connect packet (bytes length): {len(connect_packet)}")
                print(f"    [DEBUG] Connect packet (last 50 bytes): {connect_packet[-50:]}")
                newline_check = connect_packet.endswith(b'\n')
                print(f"    [DEBUG] Connect packet ends with \\n: {newline_check}")
            
            # Отправляем запрос на подключение
            if self.goldsource:
                # Для GoldSource нужно добавить Steam ticket после connect строки
                # В реальном клиенте: 
                # 1. MSG_WriteStringf записывает строку с \n и добавляет null terminator (\x00)
                # 2. MSG_SeekToBit откатывается на 8 бит (1 байт) назад, перезаписывая null terminator
                # 3. CL_WriteSteamTicket добавляет 512 байт нулей
                # Так что формат: connect строка С \n, БЕЗ null terminator + 512 байт нулей
                connect_packet_clean = connect_packet.rstrip(b'\x00')  # Убираем null terminator если есть
                # Формируем полный пакет: OOB_HEADER + connect строка (с \n, без \x00) + 512 байт нулей
                full_packet = OOB_HEADER + connect_packet_clean + b'\x00' * 512
                if self.debug:
                    print(f"    [DEBUG] Отправка connect пакета с Steam ticket ({len(full_packet)} байт)")
                    print(f"    [DEBUG] Connect строка (первые 100 байт): {connect_packet_clean[:100]}")
                    print(f"    [DEBUG] Connect строка заканчивается на \\n: {connect_packet_clean.endswith(b'\n')}")
                    print(f"    [DEBUG] Первые 20 байт полного пакета (hex): {full_packet[:20].hex()}")
                    print(f"    [DEBUG] Последние 20 байт полного пакета (hex): {full_packet[-20:].hex()}")
                sock.sendto(full_packet, (self.server_ip, self.server_port))
            else:
                self._send_oob_packet(sock, connect_packet)
            
            # Ждем ответ (для GoldSource увеличиваем таймаут)
            timeout = 5.0 if self.goldsource else 2.0
            response = self._recv_oob_packet(sock, timeout=timeout)
            if response:
                response_str = response.decode('utf-8', errors='ignore').strip()
                if self.debug:
                    print(f"    [DEBUG] Получен ответ на connect: '{response_str}'")
                
                # Проверяем успешное подключение
                if response_str.startswith(S2C_CONNECTION.decode('utf-8')):
                    if self.debug:
                        print(f"    [DEBUG] Подключение успешно (client_connect)")
                    self.connected = True
                    return True
                elif "disconnect" in response_str.lower() or "reject" in response_str.lower():
                    # Сервер отклонил подключение
                    if self.debug:
                        print(f"    [DEBUG] Сервер отклонил подключение")
                    return False
                else:
                    # Если сервер отправил что-то другое (не ошибку), возможно подключение принято
                    # но сервер не отправил client_connect сразу
                    if self.debug:
                        print(f"    [DEBUG] Неожиданный ответ, но считаем подключенным")
                    self.connected = True
                    return True
            else:
                if self.debug:
                    print(f"    [DEBUG] Не получен ответ на connect")
                
        except Exception as e:
            print(f"  Ошибка подключения: {e}")
            if self.debug:
                import traceback
                traceback.print_exc()
        finally:
            sock.close()
        
        return False
    
    def send_new_command(self) -> bool:
        """Отправляет команду 'new' после подключения (для полной инициализации)"""
        if not self.connected:
            return False
        
        sock = self._create_socket()
        try:
            # После client_connect нужно отправить команду "new" через netchan
            # Но для простоты просто переподключаемся периодически
            # или отправляем ping для поддержания соединения
            ping_data = b"ping\n"
            self._send_oob_packet(sock, ping_data)
            return True
        except Exception:
            return False
        finally:
            sock.close()
    
    def check_connection(self) -> bool:
        """Проверяет, активно ли подключение, отправляя ping"""
        if not self.connected:
            return False
        
        sock = self._create_socket()
        try:
            ping_data = b"ping\n"
            self._send_oob_packet(sock, ping_data)
            
            # Ждем ответ (или таймаут)
            sock.settimeout(2.0)
            try:
                response = self._recv_oob_packet(sock)
                # Если получили ответ, подключение активно
                return True
            except socket.timeout:
                # Таймаут - возможно подключение разорвано, но это не всегда означает отключение
                # OOB ping может не получать ответ, но подключение все еще активно
                # Поэтому считаем подключение активным, если не получили явного disconnect
                return True
        except Exception:
            # При ошибке считаем что подключение потеряно
            return False
        finally:
            sock.close()
    
    def send_keepalive_packet(self):
        """Отправляет пакет для поддержания подключения"""
        if not self.connected:
            return False
        
        sock = self._create_socket()
        try:
            # Отправляем ping для поддержания подключения
            ping_data = b"ping\n"
            self._send_oob_packet(sock, ping_data)
            
            # Пытаемся получить ответ, чтобы проверить, не отключился ли сервер
            sock.settimeout(1.0)
            try:
                response = self._recv_oob_packet(sock)
                if response:
                    response_str = response.decode('utf-8', errors='ignore').strip().lower()
                    # Если получили disconnect, помечаем как отключенного
                    if "disconnect" in response_str or "reject" in response_str:
                        if self.debug:
                            print(f"    [{self.nickname}] Получен disconnect от сервера")
                        self.connected = False
                        return False
            except socket.timeout:
                # Таймаут - это нормально, сервер может не отвечать на ping
                pass
            except Exception:
                pass
            
            return True
        except Exception:
            return False
        finally:
            sock.close()
    
    def auto_reconnect_loop(self):
        """Автоматически переподключается при отключении и периодически переподключается"""
        reconnect_delay = 0.5  # Задержка перед переподключением
        reconnect_interval = 50.0  # Переподключение каждые 50 секунд для поддержания слота
        last_reconnect = 0
        connection_start_time = 0
        last_ack_time = 0
        
        while self.running:
            current_time = time.time()
            
            if not self.connected:
                # Пытаемся переподключиться
                if not self.debug:
                    print(f"[{self.nickname}] Переподключение...", end=" ", flush=True)
                else:
                    print(f"    [{self.nickname}] Попытка переподключения...")
                
                if self.connect():
                    if not self.debug:
                        print("✓")
                    else:
                        print(f"    [{self.nickname}] Переподключен успешно")
                    connection_start_time = current_time
                    last_reconnect = current_time
                    last_ack_time = current_time
                else:
                    if not self.debug:
                        print("✗")
                    else:
                        print(f"    [{self.nickname}] Не удалось переподключиться, повтор через {reconnect_delay} сек")
                    time.sleep(reconnect_delay)
            else:
                # Периодически переподключаемся, чтобы поддерживать слот заполненным
                # OOB ping не поддерживает подключение через netchan, поэтому нужно переподключаться
                time_since_connect = current_time - connection_start_time
                time_since_reconnect = current_time - last_reconnect
                time_since_ack = current_time - last_ack_time
                
                # Переподключаемся каждые 50 секунд или если не получали ack больше 30 секунд
                if time_since_reconnect >= reconnect_interval or time_since_ack >= 30.0:
                    if self.debug:
                        print(f"    [{self.nickname}] Периодическое переподключение (прошло {time_since_connect:.1f} сек, ack: {time_since_ack:.1f} сек)")
                    self.connected = False
                    continue
                
                # Отправляем keepalive пакеты и проверяем ответ
                if int(current_time) % 3 == 0:  # Каждые 3 секунды
                    sock = self._create_socket()
                    try:
                        ping_data = b"ping\n"
                        self._send_oob_packet(sock, ping_data)
                        
                        # Пытаемся получить ack
                        sock.settimeout(2.0)
                        try:
                            response = self._recv_oob_packet(sock)
                            if response and b"ack" in response:
                                last_ack_time = current_time
                        except socket.timeout:
                            pass
                    except Exception:
                        pass
                    finally:
                        sock.close()
                
                # Небольшая задержка перед следующей итерацией
                time.sleep(1.0)
    
    def keep_alive(self, duration: float = 3600.0):
        """Поддерживает подключение активным"""
        if not self.connected:
            return
        
        sock = self._create_socket()
        start_time = time.time()
        
        try:
            while time.time() - start_time < duration and self.connected:
                # Отправляем ping для поддержания подключения
                # Сервер может отключить клиента по таймауту, если не получает пакеты
                ping_data = b"ping\n"
                self._send_oob_packet(sock, ping_data)
                
                # Проверяем подключение
                time.sleep(10.0)  # Ping каждые 10 секунд
                
                # Если включено авто-переподключение, проверяем состояние
                if self.auto_reconnect:
                    if not self.check_connection():
                        if self.debug:
                            print(f"    [{self.nickname}] Подключение потеряно в keep_alive")
                        self.connected = False
                        break
        except Exception:
            pass
        finally:
            sock.close()
    
    def stop(self):
        """Останавливает клиента"""
        self.running = False
        self.connected = False


def fill_server_slots(server_ip: str, server_port: int, num_bots: int = DEFAULT_MAX_BOTS, 
                     delay: float = DEFAULT_DELAY, keep_alive: bool = True, password: str = None, 
                     debug: bool = False, auto_reconnect: bool = True, goldsource: bool = False):
    """
    Заполняет слоты на сервере
    
    Args:
        server_ip: IP адрес сервера
        server_port: Порт сервера
        num_bots: Количество ботов для подключения
        delay: Задержка между подключениями в секундах
        keep_alive: Поддерживать ли подключения активными
    """
    protocol_name = "GoldSource" if goldsource else "Xash3D"
    print(f"=" * 60)
    print(f"Заполнение слотов на сервере {server_ip}:{server_port} ({protocol_name})")
    print(f"=" * 60)
    print(f"Количество ботов: {num_bots}")
    print(f"Задержка между подключениями: {delay} сек")
    print(f"Поддержание подключений: {'Да' if keep_alive else 'Нет'}")
    print(f"Автоматическое переподключение: {'Да' if auto_reconnect else 'Нет'}")
    if password:
        print(f"Пароль сервера: {'*' * len(password)}")
    print()
    
    clients = []
    connected_count = 0
    
    for i in range(num_bots):
        nickname = f"{DEFAULT_NICKNAME_PREFIX}{i+1:03d}"
        client = Xash3DClient(server_ip, server_port, nickname, password=password, debug=debug, goldsource=goldsource)
        
        print(f"[{i+1}/{num_bots}] Подключение {nickname}...", end=" ", flush=True)
        if debug:
            print()  # Новая строка для отладки
        
        if client.connect():
            print("✓ Подключен")
            connected_count += 1
            clients.append(client)
            
            # Настраиваем авто-переподключение
            if auto_reconnect:
                client.auto_reconnect = True
                # Запускаем поток для автоматического переподключения
                reconnect_thread = threading.Thread(target=client.auto_reconnect_loop, daemon=True)
                reconnect_thread.start()
            elif keep_alive:
                # Запускаем поток для поддержания подключения
                thread = threading.Thread(target=client.keep_alive, args=(3600.0,), daemon=True)
                thread.start()
        else:
            print("✗ Не удалось подключиться")
        
        # Задержка между подключениями
        if i < num_bots - 1:
            time.sleep(delay)
    
    print()
    print(f"=" * 60)
    print(f"Подключено: {connected_count}/{num_bots}")
    print(f"=" * 60)
    
    if (keep_alive or auto_reconnect) and connected_count > 0:
        mode = "автоматически переподключаются" if auto_reconnect else "поддерживаются активными"
        print(f"\nПодключения {mode}. Нажмите Ctrl+C для остановки.")
        try:
            while True:
                time.sleep(1)
                # Периодически показываем статистику
                if auto_reconnect:
                    active_count = sum(1 for c in clients if c.connected)
                    if active_count != connected_count:
                        print(f"[Статистика] Активных подключений: {active_count}/{num_bots}")
                        connected_count = active_count
        except KeyboardInterrupt:
            print("\n\nОстановка...")
            # Останавливаем всех клиентов
            for client in clients:
                client.stop()
            print(f"Отключено {len(clients)} клиентов")


def main():
    """Главная функция"""
    if len(sys.argv) < 3:
        print("Использование: python fill_server_slots.py <server_ip> <server_port> [num_bots] [delay]")
        print()
        print("Параметры:")
        print("  server_ip   - IP адрес сервера")
        print("  server_port - Порт сервера (обычно 27015)")
        print("  num_bots    - Количество ботов (по умолчанию 32)")
        print("  delay       - Задержка между подключениями в секундах (по умолчанию 0.1)")
        print("  password    - Пароль сервера (если требуется)")
        print("  --debug     - Включить отладочный вывод")
        print("  --no-reconnect - Отключить автоматическое переподключение")
        print("  --gs         - Подключение к GoldSource серверу (протокол 48)")
        print()
        print("Примеры:")
        print("  python fill_server_slots.py 192.168.1.100 27015")
        print("  python fill_server_slots.py 192.168.1.100 27015 16 0.2")
        print("  python fill_server_slots.py 192.168.1.100 27015 32 0.1 mypassword")
        print("  python fill_server_slots.py 192.168.1.100 27015 5 0.5 --debug")
        print("  python fill_server_slots.py 192.168.1.100 27015 32 0.1 --gs")
        sys.exit(1)
    
    server_ip = sys.argv[1]
    try:
        server_port = int(sys.argv[2])
    except ValueError:
        print(f"Ошибка: неверный порт '{sys.argv[2]}'")
        sys.exit(1)
    
    num_bots = DEFAULT_MAX_BOTS
    if len(sys.argv) >= 4:
        try:
            num_bots = int(sys.argv[3])
            if num_bots < 1 or num_bots > 256:
                print("Предупреждение: количество ботов должно быть от 1 до 256")
                num_bots = max(1, min(256, num_bots))
        except ValueError:
            print(f"Ошибка: неверное количество ботов '{sys.argv[3]}'")
            sys.exit(1)
    
    delay = DEFAULT_DELAY
    if len(sys.argv) >= 5:
        try:
            delay = float(sys.argv[4])
            if delay < 0:
                delay = 0
        except ValueError:
            print(f"Ошибка: неверная задержка '{sys.argv[4]}'")
            sys.exit(1)
    
    password = None
    debug = False
    auto_reconnect = True
    goldsource = False
    
    # Обрабатываем аргументы
    for i in range(5, len(sys.argv)):
        arg = sys.argv[i]
        if arg == "--debug" or arg == "-d":
            debug = True
        elif arg == "--no-reconnect":
            auto_reconnect = False
        elif arg == "--gs" or arg == "-gs":
            goldsource = True
        elif password is None:
            password = arg
    
    try:
        fill_server_slots(server_ip, server_port, num_bots, delay, password=password, 
                         debug=debug, auto_reconnect=auto_reconnect, goldsource=goldsource)
    except KeyboardInterrupt:
        print("\n\nПрервано пользователем")
        sys.exit(0)
    except Exception as e:
        print(f"\nОшибка: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()

