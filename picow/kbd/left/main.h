#ifndef _MAIN_H_
#define _MAIN_H_

#define KBD_NODE_LEFT
#define KBD_NODE_NAME "kbd_left"

#define CYW43_DEFAULT_IP_STA_ADDRESS LWIP_MAKEU32(192, 168, 4, 2)

#define SET_MY_IP4_ADDR(addr_ptr) IP4_ADDR(ip_2_ip4(addr_ptr), 192, 168, 4, 2)

#define ACCESS_POINT_IP "192.168.4.1"

#endif /* _MAIN_H_  */
