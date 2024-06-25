#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

static int parse_netlink_msg_count = 0;

typedef void (*my_recvmsg_msg_cb_t)(struct nlmsghdr *msg, int len, void *arg);

static void my_parse_netdev_one(struct ifinfomsg *ifinfo)
{
    printf("\t\tifindex is %d\n", ifinfo->ifi_index);
    return ;
}

static void my_recvmsg_msg_cb(struct nlmsghdr *nlh, int len, void *arg)
{
    struct ifinfomsg *data;
    /* 收到NLMSG_DONE消息，应该停止recvmsg了 */
    if (nlh->nlmsg_type == NLMSG_DONE)
    {
        printf("\trecv NLMSG_DONE, stop recvmsg\n");
        *(int *)arg = 1;
        return ;
    }
        
    while (NLMSG_OK(nlh, len))
    {
        data = NLMSG_DATA(nlh);
        printf("\t[%s][%d] parse_netlink_msg_count is %d\n", __func__, __LINE__, ++parse_netlink_msg_count);
        my_parse_netdev_one(data);

        nlh = NLMSG_NEXT(nlh, len);
    }

    return ;
}

static void my_recvmsg_one(int sock, my_recvmsg_msg_cb_t cb, void *user)
{
    int n;
    struct sockaddr_nl nla = {0};
    struct iovec iov;
    struct msghdr msg = {
        .msg_name = (void *)&nla,
        .msg_namelen = sizeof(struct sockaddr_nl),
        .msg_iov = &iov,
        .msg_iovlen = 1,
    };
    iov.iov_len = getpagesize();
    iov.iov_base = malloc(iov.iov_len);
    n = recvmsg(sock, &msg, 0);
 
    printf("recvmsg len is %d\n", n);
    cb(iov.iov_base, n, user);
    putchar('\n');
    free(iov.iov_base);
}

static void my_sendmsg(int sock, void *nlh, int len)
{
    struct iovec iov = {
        .iov_base = nlh,
        .iov_len = len,
    };
    struct sockaddr_nl peer = {
        .nl_family = AF_NETLINK,
        .nl_pid = 0,
    };
    struct msghdr hdr = {
        .msg_name = (void *)&peer,
        .msg_namelen = sizeof(struct sockaddr_nl),
        .msg_iov = &iov,
        .msg_iovlen = 1,
    };
    sendmsg(sock, &hdr, 0);
}

int main(int argc, char **argv)
{
    int done = 0;
    int sock;
    sock = socket(AF_NETLINK, SOCK_RAW , NETLINK_ROUTE);

    struct my_request {
        struct nlmsghdr nlh;
        struct ifinfomsg ifh;
    } r = {
            .nlh = {
                .nlmsg_len = sizeof(struct my_request),
                .nlmsg_type = RTM_GETLINK,
                .nlmsg_flags = NLM_F_REQUEST|NLM_F_ACK|NLM_F_DUMP,
                .nlmsg_seq = 1,
                .nlmsg_pid = 0,
            },
            .ifh = { .ifi_family = AF_UNSPEC}
    };
    my_sendmsg(sock, &r, sizeof(struct my_request));
    
    while (!done)
        my_recvmsg_one(sock, my_recvmsg_msg_cb, &done);

    close(sock);
    return 0;
}