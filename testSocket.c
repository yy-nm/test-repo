#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void handle_client(int);

int main( int argc, void **args)
{
	struct sockaddr_in addr;
	struct sockaddr_in client;
	char buf[1024];
	
	if (argc != 3) {
		printf("must have 3 args, now: %d", argc);
		return -1;
	}
	char *ip = args[1];
	char *port_str = args[2];
	short port = (short)atoi(port_str);
	if (0 == port) {
		printf("error port\n");
		return -2;
	}
	
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);
	printf("socket start, ip: %s, port: %d\n", ip, port);
	int s_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (0 > s_fd) {
		printf("socket error\n");
		return -3;
	}
	
	printf("bind start\n");
	if (0 > bind(s_fd, (struct sockaddr *) &addr, sizeof(struct sockaddr))) {
		printf("bind error\n");
		return -10;
	}

	printf("listen start\n");
	if (0 > listen(s_fd, 1024)) {
		printf("listen error\n");
		return -4;
	}
	
	for (;;) {
		printf("accept start\n");
		memset(&client, 0, sizeof(client));
		int len = sizeof(client);
		int ret = accept(s_fd, (struct sockaddr *) &client, &len);
		if (0 > ret) {
			printf("accept error\n");
			return -5;
		}
		printf("client info: ip: %s, port: %d\n", inet_ntop(AF_INET, &client.sin_addr, buf, sizeof(buf)), ntohs(client.sin_port));
		printf("recv client start\n");
		int pid = fork();
		if (0 == pid) {
			handle_client(ret);
			exit(0);
		}
	}
	
	return 0;
}

void handle_client(int client_fd)
{
	char buf[1024];
	for (;;) {
		int retn = read(client_fd, buf, sizeof(buf - 1));
		if (0 > retn) {
			printf("read error\n");
			return;
		} else if (0 == retn) {
			printf("client exit\n");
			close(client_fd);
			break;
		}
		buf[retn] = '\0';
		printf("recv: %s\n", buf);
	}
	
}
