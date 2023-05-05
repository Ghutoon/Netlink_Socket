#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_arp.h>
#include <errno.h>

#define MAX_BUFFER_SIZE 1024

void print_arp_changes(struct nlmsghdr *nh)
{
        struct ndmsg *ndm = (struct ndmsg *)NLMSG_DATA(nh);
        struct rtattr *rta = (struct rtattr *)RTM_RTA(ndm);
        int rtl = RTM_PAYLOAD(nh);

        while (rtl && RTA_OK(rta, rtl))
        {
                if (rta->rta_type == NDA_LLADDR)
                {
                        unsigned char *mac = (unsigned char *)RTA_DATA(rta);
                        printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x     ", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                }
                else if (rta->rta_type == NDA_DST)
                {
                        struct in_addr *ip = (struct in_addr *)RTA_DATA(rta);
                        printf("IP address: %s    ", inet_ntoa(*ip));
                }

                printf("\n");
                rta = RTA_NEXT(rta, rtl);
        }
}

int main()
{
        int socket_fd;
        struct sockaddr_nl nladdr;

        /*Create netlink socket*/
        socket_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
        if (socket_fd < 0)
        {
                printf("%s\n", strerror(errno));
                return -1;
        }

        /*Filling up sockaddr_nl structure fields*/
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

                /*In struct msghdr we store the message recieved from the kernel*/
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
                /*Using struct nlmsghdr to parse the recieved message*/

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
                                print_arp_changes(nh);
                        }
                        else if (nh->nlmsg_type == RTM_DELNEIGH)
                        {
                                printf("ARP entry removed\n");
                                print_arp_changes(nh);
                        }
                        else
                        {
                                /*Ignore other messages*/
                        }
                }
        }
}