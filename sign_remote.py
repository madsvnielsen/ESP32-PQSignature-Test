# Used for measuring signing over WiFi
import socket
import time
import argparse

def send_and_measure(ip, port, message):
    while True:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5) 
            sock.connect((ip, port))
            start_time = time.time()
            sock.sendall(message.encode('utf-8'))
            response = sock.recv(1024)
            end_time = time.time()
            print((end_time - start_time) * 1000)
        except Exception as e:
            print("Error:", e)
        finally:
            sock.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Send a message and measure response time.")
    parser.add_argument("ip", type=str, help="ESP32 IP address")
    parser.add_argument("port", type=int, help="ESP32 port")
    parser.add_argument("message", type=str, help="Message to send")
    args = parser.parse_args()
    send_and_measure(args.ip, args.port, args.message)
