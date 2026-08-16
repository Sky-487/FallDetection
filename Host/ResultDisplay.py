# from asyncio.windows_events import NULL
import socket
import sys

def udp_server(host='0.0.0.0', port=8888):
    """
    UDP服务器，用于接收Arduino发送的SerialBuffer数据
    :param host: 监听的主机地址，默认0.0.0.0表示监听所有网络接口
    :param port: 监听的端口号
    """
    # 创建UDP套接字
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    try:
        # 绑定地址和端口
        sock.bind((host, port))
        print(f"UDP服务器启动，监听 {host}:{port}")
        
        while True:
            # 接收数据，缓冲区大小为1024字节
            data, addr = sock.recvfrom(1024)
            
            # 解码数据并打印
            received_data = data.decode('utf-8').strip()
            print(f"来自 {addr} 的数据: {received_data}")
            print("\n") 

    # 下面的代码不起作用，可直接通过终端窗口中的终止终端按钮（删除桶）退出！
    # except KeyboardInterrupt:
    #     print("\n服务器正在关闭...")
    #     sys.exit(0)  # 使用 sys.exit 可以更明确地退出程序

    finally:
        sock.close()

if __name__ == '__main__':
    # 默认监听端口8888，您可以根据需要修改(注：IP只能是运行此代码的计算机地址或是默认地址)
    # udp_server(host='192.168.224.203', port=8888)
    udp_server(port=8888)