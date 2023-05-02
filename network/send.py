import socket

# 创建套接字
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# 连接
client_socket.connect(('127.0.0.1', 54123))
print('已连接！')
# 发送消息
client_socket.send(input('>>> ').encode('UTF-8'))
client_socket.close()
