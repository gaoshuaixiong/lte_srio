#include "rlc_macfsm.h"
#include "Rlctypes.h"

//extern void  findInsBylcid(u16 rnti,u8 lcid, void **up, void**down, RlcMode *mode);
void add_data_retx(struct AM_Instance*amIns, u8 lcid);



void Am_instance_build(void)
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
        amIns->currentStatVar = ST_IDLE; //modify in 20150805
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
     //   list_add_tail(&amIns->amList, &SV(vAm_ins).amList);
        fsm_printf(" am instance has established\n");

    FOUT;
}

void UM_instance_bulid(void)
{
	 SV_PTR_GET(rlc_mac_sv);
 //       SV(umText) =  instance_create(UM_TX_Instance);
        SV(umText).snFiledLength = 5;
        SV(umText).lcid = 2;
        SV(umText).rbid = 2 ;
        SV(umText).CurrentStatVar = ST_IDLE; //modify in 20150805
       SV(umText).umSduBufferNum = SV(umText).umSduBufferSize = 0;
        INIT_LIST_HEAD(&SV(umText).umSduBuffer.list);
     //   list_add_tail(&umTxIns->umTxList, &SV(vUm_tx_ins).umTxList);
        fsm_printf("PCRLC_BULID_REQ: um tran instance has established(rbid is %d)\n",SV(umText).rbid);
}

void Am_instance_add(void)
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
        amIns->currentStatVar = ST_IDLE; //modify in 20150805
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
  //      list_add_tail(&amIns->amList, &SV(vAm_ins).amList);
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
	fsm_schedule_self(200000,POLL);           //时间觉得太短，值可以调大
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
	/*findInsBylcid(0,8, &up, &down, &SV(ins_mode));
	amIns = (AM_Instance *)up;	
	if(amIns == NULL) fsm_printf("the instance is not exist\n");
	else amIns->reorderTimer = fsm_schedule_self(200000,  amIns->reorderTimerCode);
	fsm_printf("amIns->pollRetxTimerCode is %d\n",amIns->reorderTimerCode);*/
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
	RLC_AMDPDU_seg_ind *ind = (RLC_AMDPDU_seg_ind *)skb->data;
	ind->E = 0;
	ind->LI = 11;
	fsm_skb_put(skb,sizeof(RLC_AMDPDU_seg_ind));
	fsm_mem_cpy(skb->tail, sdu1, 11);
	fsm_skb_put(skb,11);
	fsm_mem_cpy(skb->tail, sdu2, 11);
	fsm_skb_put(skb,11);

	AmBuffer*ambuf = (AmBuffer*)fsm_mem_alloc(sizeof(AmBuffer));
	ambuf->pkt = skb;
	ambuf->pktstatus = FULL_SDU;
	ambuf->pos = 0;
	ambuf->segnum = 1;
	ambuf->segStart[0] = 0;
	ambuf->segEnd[0] = 22;
	ambuf->retxCount = 0;
	ambuf->SN = 0;
	ambuf->headLen = 2*2;
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
	skb = fsm_pkt_create(sizeof(FSM_PKT)+4);
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
void addPollRetxData()
{
	FSM_PKT *skb = NULL;
	struct AmBuffer *amTxedBuffer;
	unsigned short hdr = 0x8000;
	unsigned short data = 0x0002;
	SV_PTR_GET(rlc_mac_sv);
	skb = fsm_pkt_create(sizeof(FSM_PKT)+4);
	amTxedBuffer = (struct AmBuffer *)fsm_mem_alloc(sizeof(struct AmBuffer));
	
	fsm_mem_cpy(skb->tail,&hdr,2);
	fsm_skb_put(skb,2);
	fsm_mem_cpy(skb->tail,&data,2);
	fsm_skb_put(skb,2);
	amTxedBuffer->pkt = skb;
	amTxedBuffer->SN = 0x0000;
	amTxedBuffer->pktstatus = 1;
	amTxedBuffer->headLen = 2;
	amTxedBuffer->retxCount = 0;
	amTxedBuffer->segnum = 0;
	SV(amText).pduWithoutPoll++;
	SV(amText).byteWithoutPoll += skb->len;
	SV(amText).amTransmittedBufferNum++;
	SV(amText).amTransmittedBufferSize += skb->len;
	SV(amText).vt_s++;
	list_add_tail(&(amTxedBuffer->list),&SV(amText).amTransmittedBuffer.list);
	
	//fsm_printf("before poll_retx amTransmittedBufferNum is : %d\n",SV(amText).amTransmittedBufferNum);
	//fsm_printf("before poll_retx amTransmittedBufferSize is : %d\n",SV(amText).amTransmittedBufferSize);
	//fsm_printf("before poll_retx amRetxBufferNum is : %d\n",SV(amText).amRetxBufferNum);
	//fsm_printf("before poll_retx amRetxBufferSize is : %d\n",SV(amText).amRetxBufferSize);
	FOUT;
}


void addpkt_recvData()
{
	FSM_PKT *skb = NULL;
	struct Buffer *ctrlBf = NULL;
	FSM_PKT *skb2 = NULL;
	struct AmBuffer *amTxedBuffer = NULL;
	FSM_PKT *skb3 = NULL;
	struct AmBuffer *amTxedBuffer2 = NULL;
	FSM_PKT *skb4 = NULL;
	struct AmBuffer *amTxedBuffer3 = NULL;
	unsigned short data = 0x0000;
	unsigned short hdr = 0X0000;
	unsigned char mhdr = 0x0000;
	SV_PTR_GET(rlc_mac_sv);

	skb = fsm_pkt_create(sizeof(FSM_PKT)+8);
	ctrlBf = (struct Buffer *)fsm_mem_alloc(sizeof(struct Buffer));	

	skb2 = fsm_pkt_create(sizeof(FSM_PKT)+4);
	amTxedBuffer = (struct AmBuffer *)fsm_mem_alloc(sizeof(struct AmBuffer));

	skb3 = fsm_pkt_create(sizeof(FSM_PKT)+4);
	amTxedBuffer2 = (struct AmBuffer *)fsm_mem_alloc(sizeof(struct AmBuffer));

	skb4 = fsm_pkt_create(sizeof(FSM_PKT)+4);
	amTxedBuffer3 = (struct AmBuffer *)fsm_mem_alloc(sizeof(struct AmBuffer));

	
	hdr = 0x0006;
	fsm_mem_cpy(skb->tail,&hdr,2);	fsm_mem_cpy(&hdr,skb->data,2);fsm_printf("skb->data is %d\n",hdr);
	fsm_skb_put(skb,2);
	hdr = 0x0140;
	fsm_mem_cpy(skb->tail,&hdr,2);
	fsm_skb_put(skb,2);
	hdr = 0x1a00;
	fsm_mem_cpy(skb->tail,&hdr,2);
	fsm_skb_put(skb,2);
	hdr = 0x0400;
	fsm_mem_cpy(skb->tail,&hdr,2);
	fsm_skb_put(skb,2);
	mhdr = 0x40;
	fsm_mem_cpy(skb->tail,&mhdr,1);
	fsm_skb_put(skb,1);
	ctrlBf->pkt = skb;
	SV(amText).ctrlPduBufferNum++;
	SV(amText).ctrlPduBufferSize += skb->len;
	SV(amText).pollSn = 100;
	list_add_tail(&(ctrlBf->list),&SV(amText).ctrlPduBuffer.list);

	hdr = 0x8000;
	data = 0x0002;
	fsm_mem_cpy(skb2->tail,&hdr,2);
	fsm_skb_put(skb2,2);
	fsm_mem_cpy(skb2->tail,&data,2);
	fsm_skb_put(skb2,2);
	amTxedBuffer->pkt = skb2;
	amTxedBuffer->SN = 0x0000;
	amTxedBuffer->pktstatus = 1;
	amTxedBuffer->headLen = 2;
	amTxedBuffer->retxCount = 0;
	amTxedBuffer->segnum = 0;
	SV(amText).pduWithoutPoll++;
	SV(amText).byteWithoutPoll += skb2->len;
	SV(amText).amTransmittedBufferNum++;
	SV(amText).amTransmittedBufferSize += skb2->len;
	SV(amText).vt_s++;
	list_add_tail(&(amTxedBuffer->list),&SV(amText).amTransmittedBuffer.list);

	hdr = 0x8002;
	data = 0x0003;	
	fsm_mem_cpy(skb3->tail,&hdr,2);
	fsm_skb_put(skb3,2);
	fsm_mem_cpy(skb3->tail,&data,2);
	fsm_skb_put(skb3,2);
	amTxedBuffer2->pkt = skb3;
	amTxedBuffer2->SN = 0x0002;
	amTxedBuffer2->pktstatus = 1;
	amTxedBuffer2->headLen = 2;
	amTxedBuffer2->retxCount = 0;
	amTxedBuffer2->segnum = 0;
	SV(amText).pduWithoutPoll++;
	SV(amText).byteWithoutPoll += skb3->len;
	SV(amText).amTransmittedBufferNum++;
	SV(amText).amTransmittedBufferSize += skb3->len;
	SV(amText).vt_s++;
	list_add_tail(&(amTxedBuffer2->list),&SV(amText).amTransmittedBuffer.list);

	hdr = 0x8003;
	data = 0x0004;	
	fsm_mem_cpy(skb4->tail,&hdr,2);
	fsm_skb_put(skb4,2);
	fsm_mem_cpy(skb4->tail,&data,2);
	fsm_skb_put(skb4,2);
	amTxedBuffer3->pkt = skb4;
	amTxedBuffer3->SN = 0x0003;
	amTxedBuffer3->pktstatus = 2;
	amTxedBuffer3->headLen = 2;
	amTxedBuffer3->retxCount = 0;
	amTxedBuffer3->segnum = 0;
	SV(amText).pduWithoutPoll++;
	SV(amText).byteWithoutPoll += skb4->len;
	SV(amText).amTransmittedBufferNum++;
	SV(amText).amTransmittedBufferSize += skb4->len;
	SV(amText).vt_s++;
	list_add_tail(&(amTxedBuffer3->list),&SV(amText).amTransmittedBuffer.list);
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
