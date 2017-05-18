import socket
import sys

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Connect the socket to the port where the server is listening
server_address = ('192.168.4.1', 333)
print >>sys.stderr, 'connecting to %s port %s' % server_address
sock.connect(server_address)

while(True):
    data = input("write a message: \n")
    sock.send(data)
