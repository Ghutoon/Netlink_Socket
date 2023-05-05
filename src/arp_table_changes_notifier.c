#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <errno.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <netinet/in.h>
#define MAX_BUFFER_SIZE 1024

int main()
{
        int socket_fd;
        struct sockaddr_nl nladdr;

        socket_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
        if (socket_fd < 0)
        {
                printf("%s\n", strerror(errno));
                return -1;
        }

        memset(&nladdr, 0, sizeof(nladdr));

        nladdr.nl_family = AF_NETLINK;
        nladdr.nl_pad = 0;
        nladdr.nl_pid = getpid();
        nladdr.nl_groups = RTMGRP_NEIGH;

        int bind_status = bind(socket_fd, (struct sockaddr *)&nladdr, sizeof(nladdr));

        if (bind_status < 0)
        {
                printf("%s\n", strerror(errno));
                return -1;
        }

        printf("Waiting for ARP notifications...\n");

        while (1)
        {
                char buf[MAX_BUFFER_SIZE];
                struct iovec iov = {buf, sizeof(buf)};

                struct msghdr msg;
                msg.msg_name = &nladdr;
                msg.msg_namelen = sizeof(nladdr);
                msg.msg_iov = &iov;
                msg.msg_iovlen = 1;

                struct nlmsghdr *nh;
                struct nlmsgerr *err;
                int len;

                len = recvmsg(socket_fd, &msg, 0);
                if (len < 0)
                {
                        perror("recvmsg");
                        continue;
                }

                for (nh = (struct nlmsghdr *)buf; NLMSG_OK(nh, len); nh = NLMSG_NEXT(nh, len))
                {
                        if (nh->nlmsg_type == NLMSG_ERROR)
                        {
                                err = (struct nlmsgerr *)NLMSG_DATA(nh);
                                fprintf(stderr, "Netlink error: %s (%d)\n", strerror(-err->error), err->error);
                                continue;
                        }

                        if (nh->nlmsg_type == RTM_NEWNEIGH)
                        {
                                printf("ARP entry added\n");
                                // handle ARP notification
                        }
                        else if (nh->nlmsg_type == RTM_DELNEIGH)
                        {
                                printf("ARP entry removed\n");
                                // handle ARP notification
                        }
                        else
                        {
                                // ignore other messages
                        }
                }
        }
}