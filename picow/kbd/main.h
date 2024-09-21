#ifndef _MAIN_H_
#define _MAIN_H_

/* #define KBD_NODE_AP */
/* #define KBD_NODE_LEFT */
/* #define KBD_NODE_RIGHT */

#ifdef KBD_NODE_AP

#define KBD_NODE_NAME "kbd_ap"
#define KBD_NODE_IP LWIP_MAKEU32(192, 168, 4, 1)
#define CYW43_DEFAULT_IP_AP_ADDRESS KBD_NODE_IP

#endif

#ifdef KBD_NODE_LEFT

#define KBD_NODE_NAME "kbd_left"
#define KBD_NODE_IP LWIP_MAKEU32(192, 168, 4, 2)
#define KBD_AP_IP LWIP_MAKEU32(1, 4, 168, 192) // strangely reversed for udp_connect
#define CYW43_DEFAULT_IP_STA_ADDRESS KBD_NODE_IP

#endif

#ifdef KBD_NODE_RIGHT

#define KBD_NODE_NAME "kbd_right"
#define KBD_NODE_IP LWIP_MAKEU32(192, 168, 4, 3)
#define KBD_AP_IP LWIP_MAKEU32(1, 4, 168, 192) // strangely reversed for udp_connect
#define CYW43_DEFAULT_IP_STA_ADDRESS KBD_NODE_IP

#endif

#endif /* _MAIN_H_  */
