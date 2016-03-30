#include "rlc_macfsm.h"
#include "Rlctypes.h"

extern void  findInsBylcid(u8 lcid, void **up, void**down, RlcMode *mode);
void add_data_retx(struct AM_Instance*amIns, u8 lcid);
void addpkt_recvData(struct AM_Instance *amIns);
void addPollRetxData(struct AM_Instance*amIns);
void skb_printf(FSM_PKT *pkt);
void buff_printf(struct AM_Instance *amIns);


void Am_instance_build()
{
    struct AM_Instance *amIns;
    int i;
    SV_PTR_GET(rlc_mac_sv);
	amIns =  instance_create(struct AM_Instance);
	amIns->rbId = 4;
	//fsm_printf("the rbid is %d\n", amIns->rbId);
	amIns->lcId = 4;
	amIns->reorderTimerValue= 100*100;
	amIns->statProhTimerValue =150*150;
	amIns->pollRetxTimerValue = 100*100;
	amIns->pollPDU = 32;
	amIns->pollBYTE = 1000;
	amIns->maxRetxThreshold = 4;
	amIns->amRetxBufferSize = amIns->amSduBufferSize = amIns->ctrlPduBufferSize = 0;
	amIns->amRetxBufferNum = amIns->amSduBufferNum = amIns->ctrlPduBufferNum = 0;
	amIns->pollRetxTimer = amIns->reorderTimer = amIns->statProhTimer = 0;
	amIns->currentStatVar = ST_TRAN;
	//      amIns->pollRetxTimerCode =(int)amIns % 65536;                                 //根据实体地址设置定时器
	//      amIns->reorderTimerCode = ((int)amIns + 1) %65536;
	//     amIns->statProhTimerCode = ((int)amIns + 2) %65536;
	//分配定时器code值
	while((SV(allocCode[i++]) == -1)&&(i < MAX_CODE));
	if(i >= MAX_CODE) fsm_printf("no more code to allocated \n");
	else
	{
		amIns->pollRetxTimerCode = i - 1;
		SV(allocCode[i - 1]) = -1;
	}
	while((SV(allocCode[i++]) == -1)&&(i<MAX_CODE));
	if(i >= MAX_CODE) fsm_printf("no more code to allocated \n");
	else
	{
		amIns->reorderTimerCode = i - 1;
		SV(allocCode[i - 1]) = -1;
	}
	while((SV(allocCode[i++]) == -1)&&(i<MAX_CODE));
	if(i >= MAX_CODE) fsm_printf("no more code to allocated \n");
	else
		{
		amIns->statProhTimerCode = i - 1;
		SV(allocCode[i - 1]) = -1;
	}
	amIns->vt_s = amIns->vt_a = amIns->vr_x = amIns->vr_h = amIns->vr_ms = amIns->vr_r = 0;
	amIns->vr_mr = amIns->vt_ms = AM_WINDOW_SIZE;
	INIT_LIST_HEAD(&amIns->amSduBuffer.list);
	INIT_LIST_HEAD(&amIns->amRecvBuffer.list);
	INIT_LIST_HEAD(&amIns->amRetxBuffer.list);
	INIT_LIST_HEAD(&amIns->amTransmittedBuffer.list);
	INIT_LIST_HEAD(&amIns->ctrlPduBuffer.list);
	for(i = 0;  i < 1024; ++i )
		amIns->amRecvWindow[i] = NULL;
	for(i = 0;  i < 1024; ++i )
		amIns->amTranWindow[i]= NULL;
	list_add_tail(&amIns->amList, &SV(vAm_ins).amList);
	fsm_printf(" am instance has established\n");

    FOUT;
}

void UM_instance_bulid(void)
{
	SV_PTR_GET(rlc_mac_sv);
 //    SV(umText) =  instance_create(UM_TX_Instance);
	SV(umText).snFiledLength = 5;
	SV(umText).lcid = 2;
	SV(umText).rbid = 2 ;
	SV(umText).CurrentStatVar = ST_TRAN;
	SV(umText).umSduBufferNum = SV(umText).umSduBufferSize = 0;
	INIT_LIST_HEAD(&SV(umText).umSduBuffer.list);
	//   list_add_tail(&umTxIns->umTxList, &SV(vUm_tx_ins).umTxList);
	fsm_printf("PCRLC_BULID_REQ: um tran instance has established(rbid is %d)\n",SV(umText).rbid);
}

void Am_instance_add()
{
    struct AM_Instance *amIns;
    int i = 0;
    SV_PTR_GET(rlc_mac_sv);
	amIns =  instance_create(struct AM_Instance);
	amIns->rbId = 8;
	//fsm_printf("the rbid is %d\n", amIns->rbId);
	amIns->lcId = 8;
	amIns->reorderTimerValue= 1000*100;
	amIns->statProhTimerValue = 2000*100;
	amIns->pollRetxTimerValue = 1000*100;
	amIns->pollPDU = 32;
	amIns->pollBYTE = 1000;
	amIns->maxRetxThreshold = 4;
	amIns->amRetxBufferSize = amIns->amSduBufferSize = amIns->ctrlPduBufferSize = 0;
	amIns->amRetxBufferNum = amIns->amSduBufferNum = amIns->ctrlPduBufferNum = 0;
	amIns->pollRetxTimer = amIns->reorderTimer = amIns->statProhTimer = 0;
	amIns->currentStatVar = ST_TRAN;
	//      amIns->pollRetxTimerCode =(int)amIns % 65536;                                 //根据实体地址设置定时器
	//      amIns->reorderTimerCode = ((int)amIns + 1) %65536;
	//     amIns->statProhTimerCode = ((int)amIns + 2) %65536;
	//分配定时器code值
	while((SV(allocCode[i++]) == -1)&&(i < MAX_CODE));
	if(i >=MAX_CODE) fsm_printf("no more code to allocated \n");
	else
	{
		amIns->pollRetxTimerCode = i - 1;
		SV(allocCode[i - 1]) = -1;
	}
	while((SV(allocCode[i++]) == -1)&&(i<MAX_CODE));
	if(i >= MAX_CODE) fsm_printf("no more code to allocated \n");
	else
	{
		amIns->reorderTimerCode = i - 1;
		SV(allocCode[i - 1]) = -1;
	}
	while((SV(allocCode[i++]) == -1)&&(i<MAX_CODE));
	if(i >= MAX_CODE) fsm_printf("no more code to allocated \n");
	else
	{
		amIns->statProhTimerCode = i - 1;
		SV(allocCode[i - 1]) = -1;
	}
	amIns->vt_s = amIns->vt_a = amIns->vr_x = amIns->vr_h = amIns->vr_ms = amIns->vr_r = 0;
	amIns->vr_mr = amIns->vt_ms = AM_WINDOW_SIZE;
	INIT_LIST_HEAD(&amIns->amSduBuffer.list);
	INIT_LIST_HEAD(&amIns->amRecvBuffer.list);
	INIT_LIST_HEAD(&amIns->amRetxBuffer.list);
	INIT_LIST_HEAD(&amIns->amTransmittedBuffer.list);
	INIT_LIST_HEAD(&amIns->ctrlPduBuffer.list);
	for(i = 0;  i < 1024; ++i )
		amIns->amRecvWindow[i] = NULL;
	for(i = 0;  i < 1024; ++i )
		amIns->amTranWindow[i]= NULL;
	list_add_tail(&amIns->amList, &SV(vAm_ins).amList);
	fsm_printf(" am instance has established\n");

//在重传队列里面放重传数据
	add_data_retx(amIns,8);
	//amIns->pollRetxTimer = fsm_schedule_self(200000, amIns->pollRetxTimerCode);
//      fsm_printf("the pollRetxTimerCode is %d, the pollRetxTimer is %d\n", amIns->pollRetxTimerCode, amIns->pollRetxTimer);
    FOUT;
}
void test()
{
	struct AM_Instance *amIns;
	void *up, *down;
	SV_PTR_GET(rlc_mac_sv);
	//测试轮询
	//fsm_schedule_self(200000,POLL);           //时间觉得太短，值可以调大
	//fsm_printf("POLL the code is %d\n",POLL);
	//测试控制报文接收
	//fsm_schedule_self(0,CTRL_PKT_RECV);
	//测试控制报文发送
	//fsm_schedule_self(0,CTRL_PKT_TRAN);
	//测试分段
	//fsm_schedule_self(0,SEGMENT);
	//测试再分段
	//fsm_schedule_self(0,RESEGMENT);
	//测试重组
	//fsm_schedule_self(0,REASSAMBLE);
	findInsBylcid(8, &up, &down, &SV(ins_mode));
	amIns = (AM_Instance *)up;	
	if(amIns == NULL) fsm_printf("the instance is not exist\n");
	else amIns->reorderTimer = fsm_schedule_self(200000,  amIns->reorderTimerCode);
	fsm_printf("amIns->pollRetxTimerCode is %d\n",amIns->reorderTimerCode);
}

void buff_printf(struct AM_Instance *amIns)
{
	struct AmBuffer *amTxedBf = NULL;
	struct AmBuffer *amRetxBf = NULL;
	struct AmBuffer *nposBf= NULL;
	struct  AmSduBuffer *sdupos = NULL;
	struct  AmSduBuffer *sduBf = NULL;

	//打印重传队列
	fsm_printf("printf the amRetxBuffer\n");
	fsm_printf("the amRetxBufferSize is %d\n",amIns->amRetxBufferSize);
	if( !list_empty(&amIns->amRetxBuffer.list) )
	{
		list_for_each_entry_safe(amRetxBf, nposBf, &amIns->amRetxBuffer.list,list)
		{
			fsm_printf("The PDU SN is %d\n",amRetxBf->SN);
			skb_printf(amRetxBf->pkt);
		}
	}
	else
	{
		fsm_printf("The amRetxBuffer is empty\n");
	}
	nposBf = NULL;
}

void skb_printf(FSM_PKT *pkt)
{
	fsm_printf("%s\n",(char *)(pkt->data+4));
	FOUT;
}
void add_data_retx(struct AM_Instance*amIns, u8 lcid)
{
	SV_PTR_GET(rlc_mac_sv);
	char*sdu1 = "context 1.";
	char *sdu2 = "context 2.";
	FSM_PKT *skb = fsm_pkt_create(22 + sizeof(RLC_AMDPDU_seg_ind) + sizeof(RLC_AMPDU_fixhead));	
	RLC_AMPDU_fixhead*fix = (RLC_AMPDU_fixhead*)skb->data;;
	fix->DorC = 1;
	fix->E = 1;
	fix->FIfirst = fix->FIsecond = 0;
	fix->P = 0;
	fix->RF = 0;
	fix->SN = 0;
	fsm_skb_put(skb,sizeof(RLC_AMPDU_fixhead));
	RLC_AMDPDU_seg_ind *ind = (RLC_AMDPDU_seg_ind *)skb->tail;
	ind->E = 0;
	ind->LI = 11;
	fsm_skb_put(skb,sizeof(RLC_AMDPDU_seg_ind));
	fsm_mem_cpy(skb->tail, sdu1, strlen(sdu1));
	fsm_skb_put(skb,strlen(sdu1));
	fsm_mem_cpy(skb->tail, sdu2, strlen(sdu2));
	fsm_skb_put(skb,strlen(sdu2));

	AmBuffer*ambuf = (AmBuffer*)fsm_mem_alloc(sizeof(AmBuffer));
	ambuf->pkt = skb;
	ambuf->pktstatus = FULL_SDU;
	ambuf->pos = 0;
	ambuf->segnum = 1;
	ambuf->segStart[0] = 0;
	ambuf->segEnd[0] = 20;
	ambuf->retxCount = 0;
	ambuf->SN = 0;
	ambuf->headLen = 2*2;
	ambuf->datalen = 20;
	fsm_printf("add data in retx buffer\n");
	list_add_tail(&ambuf->list, &amIns->amRetxBuffer.list);
	FOUT;
}


//分别在build()函数中调用以下四个函数可以对不同的功能函数的测试进行数据初始化
void addPollData()
{
	FSM_PKT *skb = NULL;
	struct AmBuffer *amTxedBuffer;
	unsigned short hdr = 0x8001;
	unsigned short data = 0x0002;
	SV_PTR_GET(rlc_mac_sv);
	skb = fsm_pkt_create(4);
	amTxedBuffer = (struct AmBuffer *)fsm_mem_alloc(sizeof(struct AmBuffer));

	fsm_mem_cpy(skb->tail,&hdr,2);
	fsm_skb_put(skb,2);
	fsm_mem_cpy(skb->tail,&data,2);
	fsm_skb_put(skb,2);
	amTxedBuffer->pkt = skb;
	amTxedBuffer->SN = 0x0001;
	amTxedBuffer->pktstatus = 1;
	amTxedBuffer->headLen = 2;
	amTxedBuffer->retxCount = 0;
	amTxedBuffer->segnum = 0;
	SV(amText).pduWithoutPoll = 32;
	SV(amText).byteWithoutPoll = 1000;
	SV(amText).amRetxBufferNum++;
	SV(amText).amRetxBufferSize += skb->len;
	list_add_tail(&(amTxedBuffer->list),&SV(amText).amRetxBuffer.list);
	FOUT;
}
void addPollRetxData(struct AM_Instance*amIns)
{
	char *sdu1 = "Hello world";
	char *sdu2 = " hahahahaho";
	struct AmBuffer *ambuf;
	FSM_PKT *skb = fsm_pkt_create(22 + sizeof(RLC_AMDPDU_seg_ind) + sizeof(RLC_AMPDU_fixhead));	
	RLC_AMPDU_fixhead*fix = (RLC_AMPDU_fixhead*)skb->tail;
	fix->DorC = 1;
	fix->E = 1;
	fix->FIfirst = fix->FIsecond = 1;
	fix->P = 0;
	fix->RF = 1;
	fix->SN = 0;
	fsm_skb_put(skb,sizeof(RLC_AMPDU_fixhead));
	RLC_AMDPDU_seg_ind *ind = (RLC_AMDPDU_seg_ind *)skb->tail;
	ind->E = 0;
	ind->LI = 11;
	fsm_skb_put(skb,sizeof(RLC_AMDPDU_seg_ind));
	fsm_mem_cpy(skb->tail, sdu1, 11);
	fsm_skb_put(skb,11);
	fsm_mem_cpy(skb->tail, sdu2, 11);
	fsm_skb_put(skb,11);

	ambuf = (struct AmBuffer*)fsm_mem_alloc(sizeof(struct AmBuffer));
	ambuf->pkt = skb;
	ambuf->pktstatus = ANY_SEGMENT;
	ambuf->pos = 0;
	ambuf->segnum = 1;
	ambuf->segStart[0] = 3;
	ambuf->segEnd[0] = 18;
	ambuf->retxCount = 0;
	ambuf->SN = 0;
	ambuf->headLen = 2*2;
	ambuf->datalen = 22;
	//fsm_printf("add data in retx buffer\n");
	//list_add_tail(&ambuf->list, &amIns->amRetxBuffer.list);
	list_add_tail(&ambuf->list, &amIns->amTransmittedBuffer.list);
	amIns->amTransmittedBufferNum++;
	amIns->amTransmittedBufferSize += skb->len;
	amIns->vt_s++;

	/*FSM_PKT *skb = NULL;
	struct AmBuffer *amTxedBuffer;
	unsigned short hdr = 0x8000;
	unsigned char *data = "hello world";
	//SV_PTR_GET(rlc_mac_sv);
	skb = fsm_pkt_create(13);
	amTxedBuffer = (struct AmBuffer *)fsm_mem_alloc(sizeof(struct AmBuffer));

	fsm_mem_cpy(skb->tail,&hdr,2);
	fsm_skb_put(skb,2);
	fsm_mem_cpy(skb->tail,data,13);
	fsm_skb_put(skb,13);
	amTxedBuffer->pkt = skb;
	amTxedBuffer->SN = 0x0000;
	amTxedBuffer->pktstatus = FULL_SDU;
	amTxedBuffer->headLen = 2;
	amTxedBuffer->retxCount = 0;
	amTxedBuffer->segnum = 0;

	amIns->amTransmittedBufferNum++;
	amIns->amTransmittedBufferSize += skb->len;
	amIns->vt_s++;
	list_add_tail(&(amTxedBuffer->list),&amIns->amTransmittedBuffer.list);*/
	
	FOUT;
}


void addpkt_recvData(struct AM_Instance *amIns)
{
	FSM_PKT *skb1;
	struct AmBuffer *amTxedBuffer1;
	FSM_PKT *skb2;
	struct AmBuffer *amTxedBuffer2;
	FSM_PKT *skb3;
	struct AmBuffer *amTxedBuffer3;
	FSM_PKT *skb4;
	struct AmBuffer *amTxedBuffer4;
	unsigned short hdr;
	unsigned char *str1 = "pkt1";
	unsigned char *str2 = "pkt2";
	unsigned char *str3 = "pkt3";
	unsigned char *str4 = "pkt4";
	SV_PTR_GET(rlc_mac_sv);

	skb1 = fsm_pkt_create(6);
	amTxedBuffer1= (struct AmBuffer *)fsm_mem_alloc(sizeof(struct AmBuffer));	
	skb2 = fsm_pkt_create(6);
	amTxedBuffer2 = (struct AmBuffer *)fsm_mem_alloc(sizeof(struct AmBuffer));
	skb3 = fsm_pkt_create(6);
	amTxedBuffer3 = (struct AmBuffer *)fsm_mem_alloc(sizeof(struct AmBuffer));
	skb4 = fsm_pkt_create(6);
	amTxedBuffer4 = (struct AmBuffer *)fsm_mem_alloc(sizeof(struct AmBuffer));

	hdr = 0x8000;
	fsm_mem_cpy(skb1->tail,&hdr,2);
	fsm_skb_put(skb1,2);
	fsm_mem_cpy(skb1->tail,str1,4);
	fsm_skb_put(skb1,4);
	amTxedBuffer1->pkt = skb1;
	amTxedBuffer1->SN = 0x0000;
	amTxedBuffer1->pktstatus = FULL_SDU;
	amTxedBuffer1->headLen = 2;
	amTxedBuffer1->datalen = 4;
	amTxedBuffer1->retxCount = -1;
	amTxedBuffer1->segnum = 0;
	amTxedBuffer1->pos = 0;
	amTxedBuffer1->segEnd[0] = -1;
	amTxedBuffer1->segStart[0] = -1;
	amIns->amTransmittedBufferNum++;
	amIns->amTransmittedBufferSize += skb1->len;
	amIns->vt_s++;
	list_add_tail(&amTxedBuffer1->list,&amIns->amTransmittedBuffer.list);

	hdr = 0x8001;
	fsm_mem_cpy(skb2->tail,&hdr,2);
	fsm_skb_put(skb2,2);
	fsm_mem_cpy(skb2->tail,str2,4);
	fsm_skb_put(skb2,4);
	amTxedBuffer2->pkt = skb2;
	amTxedBuffer2->SN = 0x0001;
	amTxedBuffer2->pktstatus = FULL_SDU;
	amTxedBuffer2->headLen = 2;
	amTxedBuffer2->datalen = 4;
	amTxedBuffer2->retxCount = -1;
	amTxedBuffer2->segnum = 0;
	amTxedBuffer2->pos = 0;
	amTxedBuffer2->segEnd[0] = -1;
	amTxedBuffer2->segStart[0] = -1;
	amIns->amTransmittedBufferNum++;
	amIns->amTransmittedBufferSize += skb2->len;
	amIns->vt_s++;
	list_add_tail(&amTxedBuffer2->list,&amIns->amTransmittedBuffer.list);

	hdr = 0x8002;
	fsm_mem_cpy(skb3->tail,&hdr,2);
	fsm_skb_put(skb3,2);
	fsm_mem_cpy(skb3->tail,str3,4);
	fsm_skb_put(skb3,4);
	amTxedBuffer3->pkt = skb3;
	amTxedBuffer3->SN = 0x0002;
	amTxedBuffer3->pktstatus = ANY_SEGMENT;
	amTxedBuffer3->headLen = 2;
	amTxedBuffer3->datalen = 4;
	amTxedBuffer3->retxCount = -1;
	amTxedBuffer3->segnum = 1;
	amTxedBuffer3->pos = 0;
	amTxedBuffer3->segEnd[0] = 6;
	amTxedBuffer3->segStart[0] = 2;
	amIns->amTransmittedBufferNum++;
	amIns->amTransmittedBufferSize += skb3->len;
	amIns->vt_s++;
	list_add_tail(&amTxedBuffer3->list,&amIns->amTransmittedBuffer.list);

	hdr = 0x8003;
	fsm_mem_cpy(skb4->tail,&hdr,2);
	fsm_skb_put(skb4,2);
	fsm_mem_cpy(skb4->tail,str4,4);
	fsm_skb_put(skb4,4);
	amTxedBuffer4->pkt = skb4;
	amTxedBuffer4->SN = 0x0003;
	amTxedBuffer4->pktstatus = ANY_SEGMENT;
	amTxedBuffer4->headLen = 2;
	amTxedBuffer4->datalen = 4;
	amTxedBuffer4->retxCount = -1;
	amTxedBuffer4->segnum = 2;
	amTxedBuffer4->pos = 0;
	amTxedBuffer4->segEnd[0] = 7;
	amTxedBuffer4->segStart[0] = 5;
	amTxedBuffer4->segEnd[1] = 10;
	amTxedBuffer4->segStart[1] = 8;

	amIns->amTransmittedBufferNum++;
	amIns->amTransmittedBufferSize += skb4->len;
	amIns->vt_s++;
	list_add_tail(&amTxedBuffer4->list,&amIns->amTransmittedBuffer.list);
	fsm_printf("has added data for ctrl_pkt_recv\n");
	FOUT;
}

void addPktTranData(struct AM_Instance*amIns)
{
	FSM_PKT *skb1 = NULL;
	struct AmBuffer *amRecvBuffer1 = NULL;
	FSM_PKT *skb2 = NULL;
	struct AmBuffer *amRecvBuffer2 = NULL;

	skb1= fsm_pkt_create(2+4);
	amRecvBuffer1 = (struct AmBuffer *)fsm_mem_alloc(sizeof(struct AmBuffer));
	skb2= fsm_pkt_create(2+6);
	amRecvBuffer2 = (struct AmBuffer *)fsm_mem_alloc(sizeof(struct AmBuffer));

	unsigned short data = 0x0000;
	unsigned short hdr = 0X0000;
	SV_PTR_GET(rlc_mac_sv);

	hdr = 0x8000;
	data = 0x0001;	
	fsm_mem_cpy(skb1->tail,&hdr,2);
	fsm_skb_put(skb1,2);
	fsm_mem_cpy(skb1->tail,&data,2);
	fsm_skb_put(skb1,2);
	amRecvBuffer1->pkt = skb1;
	amRecvBuffer1->SN = 0x0000;
	amRecvBuffer1->pktstatus = 1;
	amRecvBuffer1->headLen = 2;
	amRecvBuffer1->retxCount = 0;
	amRecvBuffer1->segnum = 0;
	
	amIns->vr_r++;
	amIns->amRecvWindow[0] = amRecvBuffer1;

	amIns->amRecvWindow[1] = NULL;

	hdr = 0x8002;
	data = 0x0003;	
	fsm_mem_cpy(skb2->tail,&hdr,2);
	fsm_skb_put(skb2,2);
	fsm_mem_cpy(skb2->tail,&data,2);
	fsm_skb_put(skb2,2);
	data = 0x0004;
	fsm_mem_cpy(skb2->tail,&data,2);
	fsm_skb_put(skb2,2);

	amRecvBuffer2->pkt = skb2;
	amRecvBuffer2->SN = 0x0002;
	amRecvBuffer2->pktstatus = 2;
	amRecvBuffer2->headLen = 2;
	amRecvBuffer2->retxCount = 0;
	amRecvBuffer2->segnum = 3;
	amRecvBuffer2->segStart[0] = 0;
	amRecvBuffer2->segEnd[0] = 1;
	amRecvBuffer2->segStart[1] = -1;
	amRecvBuffer2->segEnd[1] = -1;
	amRecvBuffer2->segStart[2] = -1;
	amRecvBuffer2->segEnd[2] = -1;
	amIns->amRecvWindow[2] = amRecvBuffer2;
	amIns->vr_h = 3;
	amIns->vr_ms = 1;
}