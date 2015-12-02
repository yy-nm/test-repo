/*=============================================================================
#     FileName: testtun.c
#         Desc: use tun to capture all net traffic on specify eth
#       Author: mardyu
#        Email: michealyxd@hotmail.com

#      Version: 0.0.1
#   LastChange: 2015-12-01 13:07:43
#      History:
=============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <err.h>

typedef struct sockaddr_in SOCKADDR;
typedef struct in_addr INADDR;

#define MAXADDRCOUNT 20
#define MAXPORTCOUNT 20
#define IP_LENGTH 129
// must large than mtu
#define MAX_PACKAGE_SZ 1500

typedef struct multi_tunnel_tcp {
	INADDR remote;
	INADDR local;
	uint16_t local_ports[MAXPORTCOUNT];
	int local_count;
	int local_fds[MAXPORTCOUNT];
	uint16_t remote_ports[MAXPORTCOUNT];
	int remote_count;
	SOCKADDR remotes[MAXPORTCOUNT];
	int tun_fd;
} multi_tunnel_tcp_t;


void close_port(int *, int);

int open_tun_dev(char *ifname)
{
	struct ifreq ifr;
	int fd, err;
	if (-1 == (fd = open("/dev/net/tun", O_RDWR))) {
		perror("open /dev/net/tun device error");
		return fd;
	}
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if (0 > (err = ioctl(fd, TUNSETIFF, (void *)&ifr))) {
		perror("ioctl error");
		close(fd);
		return err;
	}
	strcpy(ifname, ifr.ifr_name);
	return fd;
}

void tun_to_net(multi_tunnel_tcp_t *path)
{
	char buf[MAX_PACKAGE_SZ];
	SOCKADDR sock_addr;
	int tun_fd = path->tun_fd;
	int net_fd = path->local_fds[rand() % path->local_count];
	SOCKADDR *addr = &path->remotes[rand() % path->remote_count];
	ssize_t ret_read, ret_send;
	ret_read= read(tun_fd, buf, MAX_PACKAGE_SZ);
	if (-1 == ret_read) {
		if (EAGAIN == errno || EWOULDBLOCK == errno) {
			return;
		} else {
			perror("read tun fd error");
			exit(20);
		}
	}
	fprintf(stdout, "need send to %s:%d, size: %ld\n"
			, inet_ntoa(addr->sin_addr)
			, ntohs(addr->sin_port)
			,ret_read);
	/*write(STDOUT_FILENO, buf, ret_read);*/
	for (;;) {
		ret_send = sendto(net_fd, buf, ret_read, 0
				, (struct sockaddr *)addr
				, sizeof(SOCKADDR));
		if (ret_send == -1) {
			if (EAGAIN == errno || EWOULDBLOCK == errno) {
				fprintf(stderr, "sendto block");
				break;
			} else {
				break;
			}
		} else {
			break;
		}
	}

}

void net_to_tun(multi_tunnel_tcp_t *path, int recv_fd)
{
	char buf[MAX_PACKAGE_SZ];
	int tun_fd = path->tun_fd;
	int net_fd = recv_fd;
	ssize_t n, retn;
	int ret;
	SOCKADDR sock_addr;
	socklen_t addr_len = sizeof(sock_addr);
	n = retn = 0;
	retn = recvfrom(net_fd, buf, MAX_PACKAGE_SZ, 0
			, (struct sockaddr *)&sock_addr
			, &addr_len);
	if (retn == -1) {
		if (EAGAIN != errno && EWOULDBLOCK != errno) {
			perror("recvfrom error");
			exit(10);
		} else {
			return;
		}
	}
	fprintf(stdout, "recv from %s:%d, size: %ld\n"
			, inet_ntoa(sock_addr.sin_addr)
			, ntohs(sock_addr.sin_port)
			, retn);
	/*write(STDOUT_FILENO, buf, retn);*/
	for (;;) {
		ret = write(tun_fd, buf, retn);
		if (-1 == ret) {
			if (EAGAIN == errno || EWOULDBLOCK == errno) {
				fprintf(stderr, "write to tun will block");
				break;
			} else {
				fprintf(stderr, "fd: %d len: %ld\n"
						, tun_fd, retn);
				perror("write to tun error");
				exit(11);
			}
		} else {
			break;
		}
	}
}

void forward(int efd, multi_tunnel_tcp_t *path)
{
	int max_event = MAXPORTCOUNT + 1;
	struct epoll_event events[MAXPORTCOUNT + 1];
	int i, n;
	for (;;) {
		n = epoll_wait(efd, events, max_event, -1);
		for (i = 0; i < n; ++i) {
			if ((events[i].events & EPOLLERR)
					|| (events[i].events & EPOLLHUP)
					|| !(events[i].events & EPOLLIN)) {
				fprintf(stderr, "epoll error\n");
				close(events[i].data.fd);
				continue;
			} else if (path->tun_fd == events[i].data.fd) {
				tun_to_net(path);
			} else {
				net_to_tun(path, events[i].data.fd);
			}
		}
			
	}

}
		
int set_non_block_fd(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (-1 == flags) {
		return flags;
	}
	flags |= O_NONBLOCK;
	if (-1 == fcntl(fd, F_SETFL, flags)) {
		return -1;
	}
	return 0;
}

void start_forward(multi_tunnel_tcp_t *path)
{
	int i;
	for (i = 0; i < path->local_count; ++i) {
		if (set_non_block_fd(path->local_fds[i]) == -1) {
			perror("set non block error");
			close_port(path->local_fds, path->local_count);
			exit(2);
		}
	}
	// tun_fd is not socket fd, cannot set nonblock
	/*if (-1 == set_non_block_fd(path->tun_fd)) {*/
		/*perror("set tun non block error");*/
		/*close_port(path->local_fds, path->local_count);*/
		/*exit(4);*/
	/*}*/
	struct epoll_event event;
	int epoll_fd;

	if (-1 == (epoll_fd = epoll_create1(0))) {
		perror("create epoll fd error");
		exit(4);
	}
	event.data.fd = path->tun_fd;
	event.events = EPOLLIN;
	if (-1 == epoll_ctl(epoll_fd, EPOLL_CTL_ADD, path->tun_fd, &event)) {
		perror("epoll ctl add error");
		exit(4);
	}

	for (i = 0; i < path->local_count; ++i) {
		event.data.fd = path->local_fds[i];
		event.events = EPOLLIN;
		if (-1 == epoll_ctl(epoll_fd, EPOLL_CTL_ADD
					, path->local_fds[i], &event)) {
			perror("epoll ctl add error");
			exit(4);
		}
	}

	forward(epoll_fd, path);
	close(epoll_fd);
}

void ifconfig(const char *ifname, const char *src_addr, const char *dst_addr)
{
	char cmd[1024];
	snprintf(cmd, sizeof(cmd), "ifconfig %s %s netmask 255.255.255.255 pointopoint %s"
			, ifname, src_addr, dst_addr);
	if (0 > system(cmd)) {
		perror(cmd);
		exit(-1);
	}
}

// ./tun -e tun0 -s 10.0.0.1 -d 10.0.0.2 -i 12345 -i 12346 -o 12345 \
-l 10.0.2.15 -r 192.168.1.46
int parse_path(int count, char **args, multi_tunnel_tcp_t *path)
{
	int op;
	char ifname[IFNAMSIZ];
	char v_src_addr[IP_LENGTH];
	char v_dst_addr[IP_LENGTH];
	memset(ifname, 0, sizeof(ifname));
	memset(v_src_addr, 0, sizeof(v_src_addr));
	memset(v_dst_addr, 0, sizeof(v_dst_addr));
	int port = 0;

	while (0 < (op = getopt(count, args, "e:s:d:i:o:l:r:"))) {
		switch(op) {
		case 'e':
			strncpy(ifname, optarg, IFNAMSIZ - 1);
			break;
		case 's':
			strncpy(v_src_addr, optarg, IP_LENGTH - 1);
			break;
		case 'd':
			strncpy(v_dst_addr, optarg, IP_LENGTH - 1);
			break;
		case 'l':

			/*path->local.s_addr = inet_addr(optarg);*/
			if (0 >= inet_pton(AF_INET, optarg, &path->local)) {
				fprintf(stderr, "error ip %s\n", optarg);
				return -1;
			}
			break;
		case 'r':
			/*path->remote.s_addr = inet_addr(optarg);*/
			if (0 >= inet_pton(AF_INET, optarg, &path->remote)) {
				fprintf(stderr, "error ip %s\n", optarg);
				return -1;
			}
			break;
		case 'i':
		case 'o':
			port = strtol(optarg, NULL, 0);
			if ('i' == op) {
				if (path->local_count >= MAXPORTCOUNT) {
					fprintf(stderr
						, "Too many local port\n");
					return -1;
				}
				path->local_ports[path->local_count++] = port;
			} else {
				if (path->remote_count >= MAXPORTCOUNT) {
					fprintf(stderr
						, "Too many remoteport\n");
					return -1;
				}
				path->remote_ports[path->remote_count++] = port;
			}
			break;
		}
	}

	if (0 == path->local_count || 0 == path->remote_count 
			|| '\0' == ifname[0] || '\0' == v_src_addr[0]
			|| '\0' == v_dst_addr[0]) {
		fprintf(stderr, "err params\n");
		return -1;
	}

	if (0 > (path->tun_fd = open_tun_dev(ifname))) {
		return -1;
	}
	ifconfig(ifname, v_src_addr, v_dst_addr);

}
		

int bind_port(INADDR *addr, uint16_t port)
{
	int reuse = 1;
	SOCKADDR sock_addr;
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket error");
		return fd;
	}
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr = *addr;
	sock_addr.sin_port = htons(port);

	if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse
				, sizeof(reuse))) {
		perror("set reuse addr error");
		close(fd);
		return -1;
	}
	if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (void *)&reuse
				, sizeof(reuse))) {
		perror("set reuse port error");
		close(fd);
		return -1;
	}

	if (-1 == bind(fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr))) {
		perror("bind error");
		close(fd);
		return -1;
	}

	return fd;
}

void close_port(int *fds, int length)
{
	while(length -- > 0) {
		close(*fds++);
	}
}

int main(int argc, char **args)
{
	multi_tunnel_tcp_t path;
	memset(&path, 0, sizeof(path));
	if (0 > parse_path(argc, args, &path)) {
		fprintf(stderr, "parse_path error\n");
		return 101;
	}
	
	int i;
	for (i = 0; i < path.local_count; ++i) {
		int fd = bind_port(&path.local, path.local_ports[i]);
		if (0 > fd) {
			close_port(path.local_fds, i);
			return 1;
		}
		path.local_fds[i] = fd;
	}
	for (i = 0; i < MAXPORTCOUNT; ++i) {
		path.remotes[i].sin_family = AF_INET;
		path.remotes[i].sin_addr.s_addr = path.remote.s_addr;
		path.remotes[i].sin_port 
			= htons(path.remote_ports[i % path.local_count]);
	}

	start_forward(&path);

	return 0;
}

