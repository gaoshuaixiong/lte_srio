/******************************************************************************
 ** Filename: debug.h - 
 ** Copyright: uestc.
 ** Descriptions:  lte system public header
 ** v1.0
 ** ----------------------------------------------------------
 ** modification history:
 ** Modified by: 
 ** Modified date:
 ** Descriptions:
 ** 
 ******************************************************************************/

#ifndef    _DEBUG_H
#define    _DEBUG_H
#define   _SYSTEM_DEBUG
#ifdef     _SYSTEM_DEBUG
    #ifdef    PRINTK_LEVEL
        #define     PRINTK(fmt,args...) \
            int printk_level; \
            char *p_fmt=fmt;\
            printk_level = 4; \
            if(p_fmt[0] == '<' && p_fmt[1] >= '0' &&p_fmt[1] <= '7' && p_fmt[2] == '>') \
            { \
            	sscanf(fmt,"<%d>",&printk_level); \
            } \
            if(printk_level<= g_DebugLevel) \
            printk(fmt,##args)
    #else  
        #define     PRINTK(fmt,args...)      printk(fmt,##args)
    #endif
#else
    #define     PRINTK(fmt,args...)    
#endif


//detected the pointer is NULL or not
#define     ASSERT(arg) \
    do \
    { \
        if(unlikely(NULL == arg)) \
        { \
            PRINTK(KERN_CRIT  "parameters is NULL in function %s.\n",__FUNCTION__); \
            dump_stack();\
            return;\
        } \
    }while(0)

#endif
