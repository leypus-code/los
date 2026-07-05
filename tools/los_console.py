#!/usr/bin/env python3
import socket
import sys
import time
import select
import termios
import tty

HOST = "127.0.0.1"
PORT = 7777

def connect():
    while True:
        try:
            s = socket.create_connection((HOST, PORT), timeout=1)
            s.setblocking(False)
            print(f"[los-console] connected to {HOST}:{PORT}")
            print("[los-console] type here. Enter sends newline. Ctrl-C exits.")
            return s
        except OSError:
            print("[los-console] waiting for LOS serial port...")
            time.sleep(0.5)

def main():
    sock = connect()

    old = termios.tcgetattr(sys.stdin)
    try:
        tty.setcbreak(sys.stdin.fileno())

        while True:
            r, _, _ = select.select([sys.stdin, sock], [], [], 0.05)

            for item in r:
                if item is sys.stdin:
                    ch = sys.stdin.read(1)

                    if ch == "\x03":
                        print("\n[los-console] exit")
                        return

                    if ch == "\r":
                        ch = "\n"

                    sock.sendall(ch.encode("utf-8", errors="ignore"))

                    # local echo so we see what we type
                    if ch == "\n":
                        sys.stdout.write("\n")
                    else:
                        sys.stdout.write(ch)
                    sys.stdout.flush()

                elif item is sock:
                    try:
                        data = sock.recv(4096)
                    except BlockingIOError:
                        data = b""

                    if not data:
                        print("\n[los-console] disconnected")
                        return

                    text = data.decode("utf-8", errors="replace")
                    sys.stdout.write(text)
                    sys.stdout.flush()

    finally:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old)
        sock.close()

if __name__ == "__main__":
    main()
