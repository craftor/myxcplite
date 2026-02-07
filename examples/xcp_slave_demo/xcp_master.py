import socket
import struct
import time

HOST = '127.0.0.1'
PORT = 5555

def main():
    print(f"Connecting to XCP Slave at {HOST}:{PORT}")
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
    except ConnectionRefusedError:
        print("Connection failed. Is the slave running?")
        return

    print("Connected.")
    ctr = 0

    # 1. Send CONNECT (PID=0xFF, Mode=0x00)
    cmd = b'\xFF\x00'
    header = struct.pack('<HH', len(cmd), ctr)
    s.sendall(header + cmd)
    ctr += 1

    resp_header = s.recv(4)
    resp_len, resp_ctr = struct.unpack('<HH', resp_header)
    resp_data = s.recv(resp_len)
    if resp_data[0] != 0xFF:
        print(f"Connect failed: {resp_data.hex()}")
        return
    print("Connect OK")

    # 2. Loop sending SHORT_DOWNLOAD to modify counter_max
    # counter_max address is 0x80010000 (from previous logs)
    # SHORT_DOWNLOAD: PID(0xF4) | N(1) | Res(1) | AddrExt(1) | Addr(4) | Data...
    
    ADDRESS = 0x80010000
    LOOP_COUNT = 10

    for i in range(LOOP_COUNT):
        val = 1024 + i
        print(f"[{i+1}/{LOOP_COUNT}] Setting counter_max to {val}")
        
        # PID=0xF4 (SHORT_DOWNLOAD), N=2 bytes, Res=0, AddrExt=0, Addr=ADDRESS
        # Data = val (uint16 little endian)
        # Struct format: B B B B I H
        cmd = struct.pack('<BBBBIH', 0xF4, 2, 0, 0, ADDRESS, val)
        
        header = struct.pack('<HH', len(cmd), ctr)
        s.sendall(header + cmd)
        ctr += 1
        
        # Receive response
        resp_header = s.recv(4)
        if not resp_header: break
        resp_len, resp_ctr = struct.unpack('<HH', resp_header)
        resp_data = s.recv(resp_len)
        
        if resp_data[0] != 0xFF:
            print(f"Error response: {resp_data.hex()}")
        
        time.sleep(0.5)

    # 3. Send DISCONNECT (PID=0xFE)
    cmd = b'\xFE'
    header = struct.pack('<HH', len(cmd), ctr)
    s.sendall(header + cmd)
    ctr += 1

    resp_header = s.recv(4)
    resp_len, resp_ctr = struct.unpack('<HH', resp_header)
    resp_data = s.recv(resp_len)
    
    s.close()
    print("Disconnected.")

if __name__ == "__main__":
    main()
