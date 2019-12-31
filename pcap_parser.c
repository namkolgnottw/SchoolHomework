#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pcap.h>
#include <time.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#define ETHER_HEADER_LENGTH 14
#define IP_HEADER_LENGTH 20
#define IPV6_HEADER_LENGTH 40

int packet_nums = 0;
int ip_packet_nums = 0;
int table_size = 0;
struct ip_pkt_info{
  struct in_addr ip_src;
  struct in_addr ip_dst;
  int version;
  int count;
};

struct ip_pkt_info ip_pkt_table[3000];

void AddToTable(const struct in_addr src, const struct in_addr dst, int version) {
  int i;
  for(i=0; i<table_size; i++) {
    if ( memcmp(&ip_pkt_table[i].ip_src, &src, sizeof(struct in_addr)) == 0 )
      if ( memcmp(&ip_pkt_table[i].ip_dst, &dst, sizeof(struct in_addr)) == 0 ) {
        ip_pkt_table[i].count++;
        return;
      }
  }
  // a new pair of ip packt added.
  memcpy(&ip_pkt_table[table_size].ip_src, &src, sizeof(struct in_addr));
  memcpy(&ip_pkt_table[table_size].ip_dst, &dst, sizeof(struct in_addr));
  ip_pkt_table[table_size].count = 1;
  ip_pkt_table[table_size].version = version;
  table_size++;
}

void PrintTable() {
  int i;
  printf("IP Packets Statistics :\n");
  printf("Total ip packets :%d\n", ip_packet_nums);
  for (i=0; i<table_size; i++) {
    char ip_src_address[64];
    char ip_dst_address[64];
    if (ip_pkt_table[i].version == 4) {
      inet_ntop(AF_INET, (const void*)&ip_pkt_table[i].ip_src, ip_src_address, 64);
      printf("IPv4 src: %s,  ", ip_src_address);
      inet_ntop(AF_INET, (const void*)&ip_pkt_table[i].ip_dst, ip_dst_address, 64);
      printf("dest: %s,  ", ip_dst_address);
      printf("total nums: %d\n", ip_pkt_table[i].count);

    } else if (ip_pkt_table[i].version == 6) {
      inet_ntop(AF_INET6, (const void*)&ip_pkt_table[i].ip_src, ip_src_address, 64);
      printf("IPv6 src: %s,  ", ip_src_address);
      inet_ntop(AF_INET6, (const void*)&ip_pkt_table[i].ip_dst, ip_dst_address, 64);
      printf("dest: %s,  ", ip_dst_address);
      printf("total nums: %d\n", ip_pkt_table[i].count);
    }
  }
}


void packet_handler(u_char *args, const struct pcap_pkthdr *header, 
                    const u_char *packet) {
 packet_nums++;
 char output_buf[300];
 // proccess struct timeval 
  time_t raw_time = header->ts.tv_sec; 
  struct tm *packet_time = localtime(&raw_time);
  char time_buf[64];
  strftime(time_buf, sizeof(time_buf),  "%c", packet_time);
  printf("Packet%d:\n%s\n", packet_nums, time_buf);

  // Print ethernet mac source and mac destination addrss
  struct ether_header *eth_header = (struct ether_header*)packet;
  printf("Mac src: %s,  ", ether_ntoa((struct ether_addr*)eth_header->ether_shost));
  printf("dest: %s\n", ether_ntoa((struct ether_addr*)eth_header->ether_dhost));

  // Check is it a ip packet.
  if ( ntohs(eth_header->ether_type) == ETHERTYPE_IP || ntohs(eth_header->ether_type) == ETHERTYPE_IPV6 ) {
  } else {
    printf("\n\n");
    return;
  }
  int ip_version = 4;
  int actual_ip_header_length = 20;
  if ( ntohs(eth_header->ether_type) == ETHERTYPE_IPV6 )  {
    actual_ip_header_length = IPV6_HEADER_LENGTH;
    ip_version = 6;
  }
  ip_packet_nums++;
  printf("IP  ");
  const struct ip* ip_header = (struct ip*)(packet + ETHER_HEADER_LENGTH);
  char ip_src_address[64];
  char ip_dst_address[64];
  if (inet_ntop(AF_INET, (const void*)&ip_header->ip_src, ip_src_address, 64)==NULL)
    printf("Wrong IP addess format.\n");
  printf("src address: %s,  ", ip_src_address);
  switch(ip_version) {
    case 4 : inet_ntop(AF_INET, (const void*)&ip_header->ip_dst, ip_dst_address, 64);
    break;
    case 6 : inet_ntop(AF_INET6, (const void*)&ip_header->ip_dst, ip_dst_address, 64);
    break;
  }
  printf("dest: %s\n", ip_dst_address);
  AddToTable(ip_header->ip_src, ip_header->ip_dst, ip_version);
  
  // Check is it a tcp or udp

  if (ip_header->ip_p==IPPROTO_TCP) {  // it is tcp
    struct tcphdr* tcp_header = (struct tcphdr*)(packet + ETHER_HEADER_LENGTH + actual_ip_header_length);  
    unsigned int src_port, dst_port;
    src_port = (unsigned int)ntohs(tcp_header->th_sport);
    dst_port = (unsigned int)ntohs(tcp_header->th_dport);
    printf("TCP port src: %u, dest: %u\n\n\n", src_port, dst_port);
    


  } else if (ip_header->ip_p==IPPROTO_UDP) {  // It is udp
    struct udphdr* udp_header = (struct udphdr*)(packet + ETHER_HEADER_LENGTH + actual_ip_header_length);  
    unsigned int src_port, dst_port;
    src_port = (unsigned int)ntohs(udp_header->uh_sport);
    dst_port = (unsigned int)ntohs(udp_header->uh_dport);
    printf("UDP port src: %u, port dest: %u\n\n\n",src_port, dst_port);
  }

}

int main(int argc, char** args) {

  char error_buf[PCAP_ERRBUF_SIZE];
  pcap_t* handle;
  const u_char *packet;
  struct pcap_pkthdr packet_header;
  if (argc!=3) {
    printf("Wrong argument numbers.\n");
    return 0;
  }
  if (strcmp(args[1], "-r")!=0) {
    printf("Wrong command option.\n");
    return 0;
  }

  if ( (handle = pcap_open_offline(args[2], error_buf)) == NULL ) {
    printf("%s\n", error_buf);
    return 0;
  }

  memset(ip_pkt_table, 0, 3000*sizeof(struct ip_pkt_info)); // Init 0.
  pcap_loop(handle, 1000, packet_handler, NULL);
  PrintTable();

}

