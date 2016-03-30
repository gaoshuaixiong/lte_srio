
#ifndef RRC_MODULE
#define RRC_MODULE

void rcv_msg_from_us(struct sk_buff *__skb);  //netlink recv
extern void send_to_user(char *info, int size_info);  //netlink send

//test send message to us
//char *data = "a test message from rrcfsm.c";
//send_to_user(data, strlen(data)+1);


#endif

