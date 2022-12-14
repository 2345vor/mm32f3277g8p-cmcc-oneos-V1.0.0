/*
 * netutils: ping implementation
 */
#include "oneos_config.h"

#ifdef LWIP_USING_ICMP    /* don't build if not configured for use in oneos_config.h */
#include <lwip/opt.h>
#include <lwip/init.h>
#include <lwip/mem.h>
#include <lwip/icmp.h>
#include <lwip/netif.h>
#include <lwip/sys.h>
#include <lwip/inet.h>
#include <lwip/inet_chksum.h>
#include <lwip/ip.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>

/**
 * PING_DEBUG: Enable debugging for PING.
 */
#ifndef PING_DEBUG
#define PING_DEBUG     LWIP_DBG_ON
#endif

/** ping receive timeout - in milliseconds */
#define PING_RCV_TIMEO (2 * OS_TICK_PER_SECOND)
/** ping delay - in milliseconds */
#define PING_DELAY     (1 * OS_TICK_PER_SECOND)

/** ping identifier - must fit on a u16_t */
#ifndef PING_ID
#define PING_ID        0xAFAF
#endif

/** ping additional data size to include in the packet */
#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE 36
#endif

/** ping additional times to include in the packet */
#ifndef PING_DATA_TIMES
#define PING_DATA_TIMES 4
#endif

/* ping variables */
static u16_t ping_seq_num;
struct _ip_addr
{
    os_uint8_t addr0, addr1, addr2, addr3;
};

/** Prepare a echo ICMP request */
static void ping_prepare_echo( struct icmp_echo_hdr *iecho, u16_t len)
{
    size_t i;
    size_t data_len = len - sizeof(struct icmp_echo_hdr);

    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id     = PING_ID;
    iecho->seqno  = htons(++ping_seq_num);

    /* fill the additional data buffer with some data */
    for (i = 0; i < data_len; i++)
    {
        ((char*) iecho)[sizeof(struct icmp_echo_hdr) + i] = (char) i;
    }

#ifdef LWIP_USING_HW_CHECKSUM
      iecho->chksum = 0;
#else
      iecho->chksum = inet_chksum(iecho, len);
#endif

}

/* Ping using the socket ip */
err_t lwip_ping_send(int s, ip_addr_t *addr, int size)
{
    int err;
    struct icmp_echo_hdr *iecho;
    struct sockaddr_in to;
    int ping_size = sizeof(struct icmp_echo_hdr) + size;
    LWIP_ASSERT("ping_size is too big", ping_size <= 0xffff);

    iecho = malloc(ping_size);
    if (iecho == OS_NULL)
    {
        return ERR_MEM;
    }

    ping_prepare_echo(iecho, (u16_t) ping_size);

    to.sin_len = sizeof(to);
    to.sin_family = AF_INET;
#if LWIP_IPV4 && LWIP_IPV6
    to.sin_addr.s_addr = addr->u_addr.ip4.addr;
#elif LWIP_IPV4
    to.sin_addr.s_addr = addr->addr;
#elif LWIP_IPV6
#error Not supported IPv6.
#endif

    err = lwip_sendto(s, iecho, ping_size, 0, (struct sockaddr*) &to, sizeof(to));
    free(iecho);

    return (err == ping_size ? ERR_OK : ERR_VAL);
}

int lwip_ping_recv(int s, unsigned char *buf, size_t buf_size, int *ttl)
{
    int fromlen = sizeof(struct sockaddr_in);
    struct sockaddr_in from;
    struct ip_hdr *iphdr;
    struct icmp_echo_hdr *iecho;
    int rcved_len;
    int len;

    len = 0;
    rcved_len = 0;
    do
    {
        len = lwip_recvfrom(s, buf + rcved_len, buf_size - rcved_len, 0, (struct sockaddr*) &from, (socklen_t*) &fromlen);

        if (len > 0)
        {
            rcved_len += len;
        }
        if (rcved_len == buf_size)
        {
            break;
        }
    } while(len > 0);

    if (rcved_len == buf_size)
    {
        iphdr = (struct ip_hdr *) buf;
        iecho = (struct icmp_echo_hdr *) (buf + (IPH_HL(iphdr) * 4));
        if ((iecho->id == PING_ID) && (iecho->seqno == htons(ping_seq_num)))
        {
            *ttl = iphdr->_ttl;
            return rcved_len - sizeof(struct ip_hdr) - sizeof(struct icmp_echo_hdr);
        }
    }

    return OS_ERROR;
}

#ifdef LWIP_USING_PING
/* using the lwIP custom ping */
os_err_t ping(char* target_name, os_uint32_t times, os_size_t size)
{
#if LWIP_VERSION_MAJOR >= 2U
    struct timeval timeout = { PING_RCV_TIMEO / OS_TICK_PER_SECOND, PING_RCV_TIMEO % OS_TICK_PER_SECOND };
#else
    int timeout = PING_RCV_TIMEO * 1000UL / OS_TICK_PER_SECOND;
#endif

    int s, ttl, recv_len;
    ip_addr_t target_addr;
    os_uint32_t send_times;
    os_uint32_t recv   = 0;
    os_uint32_t lose   = 0;
    int maxttl = -1;
    int minttl = -1;
    os_tick_t recv_start_tick;
    struct addrinfo hint, *res = NULL;
    struct sockaddr_in *h = NULL;
    struct in_addr ina;
    unsigned char *ping_recv_buf;
    int recv_buf_len;
    
    send_times   = 0;
    ping_seq_num = 0;
    ping_recv_buf = NULL;

    if (size == 0)
    {
        size = PING_DATA_SIZE;
    }

    if (times == 0)
    {
        times = PING_DATA_TIMES;
    }

    memset(&hint, 0, sizeof(hint));
    /* convert URL to IP */
    if (lwip_getaddrinfo(target_name, NULL, &hint, &res) != 0)
    {
        os_kprintf("ping: unknown host %s\r\n", target_name);
        return OS_ERROR;
    }
    memcpy(&h, &res->ai_addr, sizeof(struct sockaddr_in *));
    memcpy(&ina, &h->sin_addr, sizeof(ina));
    lwip_freeaddrinfo(res);
    if (inet_aton(inet_ntoa(ina), &target_addr) == 0)
    {
        os_kprintf("ping: unknown host %s\r\n", target_name);
        return OS_ERROR;
    }
    
    recv_buf_len = size + sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr);
    ping_recv_buf = (unsigned char *)malloc(recv_buf_len);
    if (NULL == ping_recv_buf)
    {
        os_kprintf("ping: no memory\r\n");
        return OS_ERROR;
    }
    
    /* new a socket */
    if ((s = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP)) < 0)
    {
        free(ping_recv_buf);
        ping_recv_buf = NULL;
        os_kprintf("ping: create socket failed\r\n");
        
        return OS_ERROR;
    }

    lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    while (1)
    {
        int elapsed_time;

        if (lwip_ping_send(s, &target_addr, size) == ERR_OK)
        {
            recv_start_tick = sys_jiffies();
            recv_len = lwip_ping_recv(s, ping_recv_buf, recv_buf_len, &ttl);
            if (recv_len >= 0)
            {                
                elapsed_time = (sys_jiffies() - recv_start_tick) * 1000UL / OS_TICK_PER_SECOND;
                os_kprintf("%d bytes from %s icmp_seq=%d ttl=%d time=%d ms\r\n", recv_len, inet_ntoa(ina), send_times,
                        ttl, elapsed_time);
                recv++;
                if (maxttl < 0 || ttl > maxttl)
                {
                    maxttl = ttl;
                }

                if (minttl < 0 || ttl < minttl)
                {
                    minttl = ttl;
                }
            }
            else
            {
                os_kprintf("From %s icmp_seq=%d timeout\r\n", inet_ntoa(ina), send_times);
                lose++;
            }
        }
        else
        {
            os_kprintf("Send %s - error, ping failed\r\n", inet_ntoa(ina));
            lose++;
        }

        send_times++;
        if (send_times >= times)
        {
            if (maxttl < 0)
            {
                maxttl = 0;
            }
            if (minttl < 0)
            {
                minttl = 0;
            }
            /* send ping times reached, stop */
            os_kprintf("\r\nResult: from %s, sent=%d, recv=%d, lose=%d, minttl=%d, maxttl=%d\r\n", 
                       inet_ntoa(ina),
                       times,
                       recv, 
                       lose,
                       minttl,
                       maxttl);
            break;
        }

        os_task_tsleep(PING_DELAY); /* take a delay */
    }

    lwip_close(s);
    
    free(ping_recv_buf);
    ping_recv_buf = NULL;

    return OS_EOK;
}

/*Note that:  uint32: 0~4294967295 */
static os_uint32_t strnum_to_uint(char *strnum)
{
    os_uint32_t  ret  = 0;
    char        *ptr = OS_NULL;
	
    ptr = strnum;
	while(((*ptr) >= '0') && ((*ptr) <= '9'))
	{
		ret *=10;
		ret += *ptr - '0';
		ptr++;
	}

	return ret;
}

static int cmd_ping(int argc, char **argv)
{
    os_uint16_t times = 0;
    os_uint16_t size  = 0;
    os_err_t    ret   = OS_EOK;

    switch (argc)
    {
    case 1:
    {
        /* Input: ping */
        os_kprintf("Please input: ping <host address> <times[1-1000]> <pkg_size[36-2000]>\r\n");
        ret = OS_ERROR;
        break;
    }

    case 2:
    {
        /* Input: ping <host address> */
        break;
    }

    case 3:
    {
        /* Input: ping <host address> <times> */
        times = strnum_to_uint(argv[2]);
        if ((times <= 0) || (times > 1000))
        {
            os_kprintf("Input error: ping times is out of range [1-1000], times = %d\r\n", times);
            ret = OS_ERROR;
            break;
        }
        break;
    }

    case 4:
    {
        /* Input: ping <host address> <times> <pkg_siz> */
        times = strnum_to_uint(argv[2]);
        if ((times <= 0) || (times > 1000))
        {
            os_kprintf("Input error: ping times is out of range [1-1000], times = %d\r\n", times);
            ret = OS_ERROR;
            break;
        }
        
        size = strnum_to_uint(argv[3]);
        if ((size < 36) || (size > 2000))
        {
            os_kprintf("Input error: ping size is out of range [36-2000], size = %d\r\n", size);
            ret = OS_ERROR;
            break;
        }
        break;
    }

    default:
        os_kprintf("Input error, please input: ping <host address> <times[1-1000]> <pkg_size[36-2000]>\r\n");
        ret = OS_ERROR;
        break;
    }

    if( OS_EOK == ret )
        ping(argv[1], times, size);

    return ret;
}

#ifdef OS_USING_SHELL
#include <shell.h>
SH_CMD_EXPORT(lwip_ping, cmd_ping, "ping network host");
#endif /* OS_USING_SHELL */

#endif /* LWIP_USING_PING */

#endif /* LWIP_USING_ICMP */

