import socket

# 创建套接字
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# 绑定端口
server_socket.bind(('', 56789))
# 监听
server_socket.listen()
# 等待连接
print('等待连接')
client_socket, client_info = server_socket.accept()
print('已连接！', client_socket, client_info)
# 接收的消息
recv_data = client_socket.recv(1024 * 1024).decode('UTF-8', 'replace')  # 接收 1MB 的信息
print('→', recv_data)
client_socket.close()
server_socket.close()
