#include <string.h>

#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include "netdb.h"

#include "FreeRTOS.h"
#include "mprintf.h"
#define LOG_I mprintf

#define BUF_SIZE       (1024)

static int started = 0;
static int is_running = 0;
static int port = 5000;
static const char send_data[] = "This is TCP Server from RT-Thread.\r\n"; /* 发送用到的数据 */

void tcpserv(void *arg) {
	int ret;
	char *recv_data; /* 用于接收的指针，后面会做一次动态分配以请求可用内存 */
	int sock, connected, bytes_received;
	struct sockaddr_in server_addr, client_addr;

	struct timeval timeout;
	fd_set readset, readset_c;
	socklen_t sin_size = sizeof(struct sockaddr_in);
	if (recv_data) {
			vPortFree(recv_data);
			recv_data = NULL;
		}
		if (connected >= 0) {
			//closesocket(connected);
			connected = -1;
		}
		if (sock >= 0) {
			//closesocket(sock);
			sock = -1;
		}
		started = 0;
		is_running = 0;
	recv_data = pvPortMalloc(BUF_SIZE + 1); /* 分配接收用的数据缓冲 */
	if (recv_data == NULL) {
		mprintf("No memory\rn");
		return;
	}

	/* 一个socket在使用前，需要预先创建出来，指定SOCK_STREAM为TCP的socket */
	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		LOG_I("Create socket error\rn");
		goto __exit;
	}

	/* 初始化服务端地址 */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port); /* 服务端工作的端口 */
	server_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(server_addr.sin_zero), 0x0, sizeof(server_addr.sin_zero));

	/* 绑定socket到服务端地址 */
	if (bind(sock, (struct sockaddr * )&server_addr, sizeof(struct sockaddr))
			== -1) {
		LOG_I("Unable to bind\r\n");
		goto __exit;
	}

	/* 在socket上进行监听 */
	if (listen(sock, 10) == -1) {
		LOG_I("Listen error\r\n");
		goto __exit;
	}

	LOG_I("\nTCPServer Waiting for client on port %d...\r\n", port);

	started = 1;
	is_running = 1;

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	while (is_running) {
		connected=-1;
		FD_ZERO(&readset);
		FD_SET(sock, &readset);

		//LOG_I("Waiting for a new connection...");

		/* Wait for read or write */
		if (select(sock + 1, &readset, NULL, NULL, &timeout) == 0)
			continue;

		/* 接受一个客户端连接socket的请求，这个函数调用是阻塞式的 */
		connected = accept(sock, (struct sockaddr * )&client_addr, &sin_size);
		/* 返回的是连接成功的socket */
		if (connected >= 0) {
			LOG_I("I got a connection from (%s , %d)\r\n",
							inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		}else {
			continue;
		}

		/* 接受返回的client_addr指向了客户端的地址信息 */

		/* 客户端连接的处理 */
		while (is_running) {
			FD_ZERO(&readset_c);
			FD_SET(connected, &readset_c);

			/* Wait for read or write */
			if (select(connected + 1, &readset_c, NULL, NULL, &timeout) == 0) {
				LOG_I("Received connect id is %d\r\n",connected);
				continue;
			}

			/* 从connected socket中接收数据，接收buffer是1024大小，但并不一定能够收到1024大小的数据 */
			bytes_received = recv(connected, recv_data, BUF_SIZE, 0);
			if (bytes_received < 0) {
				LOG_I("Received error, close the connect.\r\n");
				closesocket(connected);
				connected = -1;
				break;
			} else if (bytes_received == 0) {
				/* 打印recv函数返回值为0的警告信息 */
				LOG_I("Received warning, recv function return 0.\r\n");
				closesocket(connected);
				connected = -1;
				break;
			} else {
				/* 有接收到数据，把末端清零 */
				recv_data[bytes_received] = '\0';
				if (strcmp(recv_data, "q") == 0
						|| strcmp(recv_data, "Q") == 0) {
					/* 如果是首字母是q或Q，关闭这个连接 */
					LOG_I("Got a 'q' or 'Q', close the connect.\r\n");
					closesocket(connected);
					connected = -1;
					break;
				} else if (strcmp(recv_data, "exit") == 0) {
					/* 如果接收的是exit，则关闭整个服务端 */
					closesocket(connected);
					connected = -1;
					goto __exit;
				} else {
					/* 在控制终端显示收到的数据 */
					LOG_I("Received data = %s\r\n", recv_data);
				}
			}

			/* 发送数据到connected socket */
			ret = send(connected, send_data, strlen(send_data), 0);
			if (ret < 0) {
				LOG_I("send error, close the connect.\r\n");
				closesocket(connected);
				connected = -1;
				break;
			} else if (ret == 0) {
				/* 打印send函数返回值为0的警告信息 */
				LOG_I("Send warning, send function return 0.\r\n");
			}
		}
	}

	__exit: if (recv_data) {
		vPortFree(recv_data);
		recv_data = NULL;
	}
	if (connected >= 0) {
		closesocket(connected);
		connected = -1;
	}
	if (sock >= 0) {
		closesocket(sock);
		sock = -1;
	}
	started = 0;
	is_running = 0;
	vTaskDelete(NULL);
}

