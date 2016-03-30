/*RRC ICI send to RLC*/
struct lte_rrc_head
{
	/*0:PCCH_Message, 1:BCCH_BCH_Message, 2:BCCH_DL_SCH_Message,  
	3:DL_CCCH_Message, 4.DL_DCCH_Message, 5:UL_CCCH_Message, 6:UL_DCCH_Message */
	u32 message_type;
    //unsigned char test[32];
}__attribute__((packed));


struct lte_test_rlc_head
{
    u8 test[32];
}__attribute__((packed));


struct lte_test_mac_head
{
    u8 test[32];
}__attribute__((packed));

/*
struct lte_test_srio_head
{
  unsigned int type;		//0 for IP, 1 for srio
  unsigned int len;
  u16 rnti;
}__attribute__((packed));
*/
struct lte_test_srio_head
{
	u32 type;		//0 for IP, 1 for srio
	u32 len;
	u16 rnti;
	u16 sfn;
	u16 subframeN;
	u32 spare;
}__attribute__((packed));
