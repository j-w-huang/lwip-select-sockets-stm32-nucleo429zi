/*
 * thread_server.c
 *
 *  Created on: Sep 14, 2019
 *      Author: yuer
 *      记得设置->
 *              MEMP_NUM_SYS_TIMEOUT + n
 *              LWIP_SOCKET_SELECT 1
 */
#include <string.h>

#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>

#include "netdb.h"

#include "FreeRTOS.h"
#include "task.h"
#include "mprintf.h"

const int PORT = 2019;
const int BACKLOG = MEMP_NUM_TCP_PCB;

#define BUF_SIZE  (128)

char tmpBuf[128];

void threadSvr(void * arg) {

	struct sockaddr_in server_addr, client_addr;
	socklen_t sin_size;

	int sock_fd, new_fd;
	int err;

	char * buf = NULL;

	fd_set fdsr;
	int maxsock;
	int i;
	struct timeval tv;
	int fd_arr[BACKLOG];
	int ret;
	int con_amount = 0;

	buf = pvPortMalloc(BUF_SIZE + 1); /* 分配接收用的数据缓冲 */
	if (buf == NULL) {
		mprintf("no enough memory give to socket.\r\n");
		return;
	}

	sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sock_fd == -1) {
		mprintf("failed to create socket server.\r\n");
		goto __exit;
	}

	int yes = 1;
	//设置地址复用
	err = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	if (err == -1) {
		mprintf("failed to setsockopt.\r\n");
		goto __exit;
	}

	int net_tv = 2000;
	//设置发送时限
	err = setsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, (char * )&net_tv,
			sizeof(int));
	if (err == -1) {
		mprintf("failed to setsockopt.\r\n");
		goto __exit;
	}
	//设置接收时限
	err = setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (char * )&net_tv,
			sizeof(int));
	if (err == -1) {
		mprintf("failed to setsockopt.\r\n");
		goto __exit;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(&(server_addr.sin_zero), 0x00, sizeof(server_addr.sin_zero));

	err = bind(sock_fd, (struct sockaddr * )(&server_addr),
			sizeof(struct sockaddr));
	if (err == -1) {
		mprintf("failed to bind port %d.\r\n", PORT);
		goto __exit;
	}

	err = listen(sock_fd, BACKLOG);
	if (err == -1) {
		mprintf("failed to listen.\r\n");
		goto __exit;
	}

	sin_size = sizeof(client_addr);

	for (;;) {
		FD_ZERO(&fdsr);
		FD_SET(sock_fd, &fdsr);

		tv.tv_sec = 3;
		tv.tv_usec = 0;

		maxsock = sock_fd;
		for (i = 0; i < BACKLOG; i++)
				{
			if (fd_arr[i] != 0) {
				FD_SET(fd_arr[i], &fdsr);
				if (maxsock < fd_arr[i]) {
					maxsock = fd_arr[i];
				}
			}
		}

		ret = select(maxsock + 1, &fdsr, NULL, NULL, &tv);
		if (ret < 0) {
			mprintf("failed to select.\r\n");
			break;
		} else if (ret == 0) {
			mprintf("timeout.\r\n");
			continue;
		} else {
			mprintf("I got client[%d], ip is %s, port is %d\r\n", ret,inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		}

		if (FD_ISSET(sock_fd, &fdsr)) {
			new_fd = accept(sock_fd, (struct sockaddr * )(&client_addr),
					&sin_size);

			if (new_fd == -1) {
				mprintf("failed to accep.t\r\n");
				//break;
				continue;
			}


			for (i = 0; i < BACKLOG; i++) {
				if (fd_arr[i] == 0) {
					fd_arr[i] = new_fd;
					break;
				}
			}

			if (new_fd > maxsock) {
				maxsock = new_fd;
			}
		}

		for (i = 0; i < BACKLOG; i++) {
			if (FD_ISSET(fd_arr[i], &fdsr)) {
				ret = recv(fd_arr[i], buf, BUF_SIZE - 1, 0);
				if (ret > 0) {
					buf[ret] = '\0';
					mprintf("msg from %d,is : %s.\r\n", fd_arr[i], buf);
					msprintf(tmpBuf, "you input :%s\r\n", buf);
					send(fd_arr[i], tmpBuf, strlen(tmpBuf), 0);
				} else if (ret == 0) {
					mprintf("client[%d] is closed.\r\n", fd_arr[i]);
					FD_CLR(fd_arr[i], &fdsr);
					close(fd_arr[i]);
					fd_arr[i] = 0;
				}
			}
		}

	}

	__exit: if (buf != NULL) {
		vPortFree(buf);
	}
	if (sock_fd > 0) {
		close(sock_fd);

		for (i = 0; i < con_amount; i++) {
			if (fd_arr[i] != 0)
				close(fd_arr[i]);
		}
	}

	vTaskDelete(NULL);

}

