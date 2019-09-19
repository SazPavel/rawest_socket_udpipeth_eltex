#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <netinet/ip.h> 
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include <net/if.h>

#define eth_head_len 14
#define ip_head_len 20
#define udp_head_len 8

struct UdpHead
{
    short src_port;
    short dest_port;
    short len;
    short checksum;
};

short checksum (short *addr, int len);

int main()
{
    char buf[] = "HI!\n";
    char buf2[128];
    char message[128];
    char interf[8];  
    struct sockaddr_ll addr, server_addr;
    int sock, bytes_read, val = 1, cycle = 1, size = sizeof(server_addr);
    struct UdpHead udp_header;
    struct ip ip_header;
    struct ifreq ifr;
    unsigned char src_mac[6], dst_mac[6];
    short port;
    
    strcpy(interf, "enp0s3");
    sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(sock < 0)
    {
        perror("sock");
        exit(-1);
    }

    //get MAC address
    memset(&ifr, 0, sizeof(struct ifreq));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", interf);
    if(ioctl(sock, SIOCGIFHWADDR, &ifr) < 0)
    {
        perror("ioctl");
        exit(-1);
    }
    memcpy(src_mac, ifr.ifr_hwaddr.sa_data, 6 * sizeof(char));

    memset(&addr, 0, sizeof(struct sockaddr_ll));

    //sockaddr_ll
    addr.sll_family = AF_PACKET;   
    addr.sll_ifindex = if_nametoindex(interf); 
    addr.sll_protocol = htons(ETH_P_ALL);
    addr.sll_halen = 6;

    dst_mac[0] = addr.sll_addr[0] = 0x08;
    dst_mac[1] = addr.sll_addr[1] = 0x00;
    dst_mac[2] = addr.sll_addr[2] = 0x27;
    dst_mac[3] = addr.sll_addr[3] = 0x96;
    dst_mac[4] = addr.sll_addr[4] = 0x74;
    dst_mac[5] = addr.sll_addr[5] = 0x94;
   
    //UDP head
    udp_header.src_port = htons(12345);
    udp_header.dest_port = htons(3333);
    udp_header.len = htons(udp_head_len + sizeof(buf));
    udp_header.checksum = 0;

    //IP head
    ip_header.ip_hl = 5;
    ip_header.ip_v = 4;
    ip_header.ip_tos = 0;
    ip_header.ip_len = htons(ip_head_len + udp_head_len + sizeof(buf));
    ip_header.ip_id = 0;
    ip_header.ip_off = 0;
    ip_header.ip_ttl = 64;
    ip_header.ip_p = IPPROTO_UDP;
    ip_header.ip_sum = 0;
    inet_pton(AF_INET, "10.0.2.15", &(ip_header.ip_src));
    inet_pton(AF_INET, "10.0.2.4", &(ip_header.ip_dst));
    ip_header.ip_sum = checksum ((short *) &ip_header, ip_head_len);

    //final assembly
    memcpy(message, dst_mac, 6);
    memcpy(message + 6, src_mac, 6);
    message[12] = ETH_P_IP / 256;
    message[13] = ETH_P_IP % 256;
    memcpy(message + eth_head_len, (char *)&ip_header, ip_head_len);
    memcpy(message + eth_head_len + ip_head_len, (char *)&udp_header, udp_head_len);
    memcpy(message + eth_head_len + ip_head_len + udp_head_len, buf, sizeof(buf));

    if(sendto(sock, message, sizeof(buf) + ip_head_len + udp_head_len + eth_head_len, 0, (struct sockaddr *)&addr, sizeof(addr)) <= 0)
    {
        perror("sendto");
        exit(-1);
    }

    while(cycle)
    {
        if(bytes_read = recv(sock, buf2, sizeof(buf2), 0) < 0)
        {
            perror("recv");
            exit(-1);
        }
        port = ntohs(*((int *)(buf2 + 2 + ip_head_len + eth_head_len)));
        if(port == 12345)
        {
            cycle = 0;
            printf("CLIENT: port %d , message: %s\n", port, buf2 + udp_head_len + ip_head_len + eth_head_len);
        }
    }
    close(sock);
    exit(0);
}


short checksum (short *addr, int len)
{
  int count = len;
  int sum = 0;
  short answer = 0;

  // Sum up 2-byte values until none or only one byte left.
  while (count > 1) {
    sum += *(addr++);
    count -= 2;
  }

  // Add left-over byte, if any.
  if (count > 0) {
    sum += *(char *) addr;
  }

  // Fold 32-bit sum into 16 bits; we lose information by doing this,
  // increasing the chances of a collision.
  // sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }

  // Checksum is one's compliment of sum.
  answer = ~sum;

  return (answer);
}

