#ifndef _MAIN_H_
#define _MAIN_H_

#define KBD_NODE_LEFT
#define KBD_NODE_NAME "kbd_left"
#define KBD_NODE_IP LWIP_MAKEU32(192,168,4,2)
#define KBD_AP_IP LWIP_MAKEU32(1,4,168,192) // strangely reversed for udp_connect

#define CYW43_DEFAULT_IP_STA_ADDRESS KBD_NODE_IP

#endif /* _MAIN_H_  */
