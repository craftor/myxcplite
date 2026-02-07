import socket
import struct
import time
import random

HOST = '127.0.0.1'
PORT = 5555

def connect(s):
    print("Connecting...")
    # CONNECT (PID=0xFF, Mode=0x00)
    cmd = b'\xFF\x00'
    send_cmd(s, cmd)
    resp = recv_resp(s)
    if resp[0] != 0xFF:
        print(f"Connect failed: {resp.hex()}")
        return False
    print("Connect OK")
    return True

def disconnect(s):
    print("Disconnecting...")
    # DISCONNECT (PID=0xFE)
    cmd = b'\xFE'
    send_cmd(s, cmd)
    recv_resp(s)
    print("Disconnected.")

def get_status(s):
    # GET_STATUS (PID=0xFD)
    cmd = b'\xFD'
    send_cmd(s, cmd)
    resp = recv_resp(s)
    if resp[0] == 0xFF:
        print(f"Status: {resp.hex()}")
    else:
        print(f"Get Status failed: {resp.hex()}")

def get_id(s):
    # GET_ID (PID=0xFA, Type=0)
    cmd = b'\xFA\x00'
    send_cmd(s, cmd)
    resp = recv_resp(s)
    if resp[0] == 0xFF:
        # Mode (1) | Len (4) | Data...
        length = struct.unpack('<I', resp[4:8])[0]
        # Data starts at index 8? No, let's check standard.
        # GET_ID Response: PID(1) | Mode(1) | Res(2) | Len(4) | Data...
        # Wait, standard says: PID(1) | Mode(1) | Res(2) | Length(4) | ...
        # My struct unpack might be off if I don't handle headers.
        # Let's just print hex.
        print(f"ID Info: {resp.hex()}")
    else:
        print(f"Get ID failed: {resp.hex()}")

def set_mta(s, addr, addr_ext=0):
    # SET_MTA (PID=0xF6, Res(2), AddrExt(1), Addr(4))
    cmd = struct.pack('<BxxBI', 0xF6, addr_ext, addr)
    send_cmd(s, cmd)
    resp = recv_resp(s)
    if resp[0] != 0xFF:
        print(f"Set MTA failed: {resp.hex()}")

def short_download(s, addr, val):
    # SHORT_DOWNLOAD (PID=0xF4, N(1), Res(1), AddrExt(1), Addr(4), Data(2))
    cmd = struct.pack('<BBBBIH', 0xF4, 2, 0, 0, addr, val)
    send_cmd(s, cmd)
    resp = recv_resp(s)
    if resp[0] != 0xFF:
        print(f"Short Download failed: {resp.hex()}")

def download(s, data):
    # DOWNLOAD (PID=0xF0, N(1), Data...)
    # Max N depends on CTO. Assuming we can send chunks.
    # Let's send in chunks of 32 bytes to be safe.
    chunk_size = 32
    for i in range(0, len(data), chunk_size):
        chunk = data[i:i+chunk_size]
        n = len(chunk)
        cmd = struct.pack('<BB', 0xF0, n) + chunk
        send_cmd(s, cmd)
        resp = recv_resp(s)
        if resp[0] != 0xFF:
            print(f"Download failed at offset {i}: {resp.hex()}")
            return

def send_cmd(s, cmd):
    global ctr
    header = struct.pack('<HH', len(cmd), ctr)
    s.sendall(header + cmd)
    ctr += 1

def recv_resp(s):
    resp_header = s.recv(4)
    if not resp_header: return b''
    resp_len, resp_ctr = struct.unpack('<HH', resp_header)
    resp_data = s.recv(resp_len)
    return resp_data

ctr = 0

def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((HOST, PORT))
    except ConnectionRefusedError:
        print("Connection failed.")
        return

    if not connect(s):
        return

    get_status(s)
    get_id(s)

    # Addresses
    ADDR_COUNTER_MAX = 0x80010000
    ADDR_MAP = 0x8001000C

    # Loop
    # LOOP_COUNT = 5
    # for i in range(LOOP_COUNT):
    #     print(f"\n--- Loop {i+1} ---")
        
    #     # 1. Short Download to counter_max
    #     val = 1000 + i * 10
    #     print(f"Setting counter_max to {val}")
    #     short_download(s, ADDR_COUNTER_MAX, val)

    #     # 2. Block Download to map
    #     # Create a pattern
    #     map_data = bytes([(x + i) % 256 for x in range(64)])
    #     print(f"Downloading 64 bytes to map (Pattern start {i})")
    #     set_mta(s, ADDR_MAP)
    #     download(s, map_data)

    #     time.sleep(1)

    # Latency Test
    print("\n--- Starting Latency Test ---")
    LATENCY_LOOPS = 1000
    DELAY_BETWEEN_PACKETS = 0.001 # 100us
    
    total_latency = 0.0
    min_latency = float('inf')
    max_latency = 0.0

    # We will use SHORT_DOWNLOAD to update counter_max as the test packet
    # It is a small packet (8 bytes payload + header)
    
    for i in range(LATENCY_LOOPS):
        start_time = time.perf_counter()
        
        # Send packet
        # short_download(s, ADDR_COUNTER_MAX, 1000 + (i % 100))
        # Inline short_download to avoid print overhead and measure strictly request-response
        cmd = struct.pack('<BBBBIH', 0xF4, 2, 0, 0, ADDR_COUNTER_MAX, 1000 + (i % 100))
        send_cmd(s, cmd)
        resp = recv_resp(s)
        
        end_time = time.perf_counter()
        
        if resp[0] != 0xFF:
            print(f"Error in loop {i}: {resp.hex()}")
            break
            
        latency = (end_time - start_time) * 1000000 # us
        total_latency += latency
        if latency < min_latency: min_latency = latency
        if latency > max_latency: max_latency = latency
        
        time.sleep(DELAY_BETWEEN_PACKETS)

    avg_latency = total_latency / LATENCY_LOOPS
    print(f"Latency Test Completed ({LATENCY_LOOPS} iterations)")
    print(f"Average Latency: {avg_latency:.2f} us")
    print(f"Min Latency: {min_latency:.2f} us")
    print(f"Max Latency: {max_latency:.2f} us")

    disconnect(s)
    s.close()

if __name__ == "__main__":
    main()
