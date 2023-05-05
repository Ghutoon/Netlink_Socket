#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <errno.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

int main()
{
        int socket_fd;
        struct sockaddr_nl user_space_socket;

        socket_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
        if (socket_fd < 0)
        {
                printf("%s\n", strerror(errno));
                return -1;
        }

        user_space_socket.nl_family = AF_NETLINK;
        user_space_socket.nl_pad = 0;
        user_space_socket.nl_pid = getpid();
}