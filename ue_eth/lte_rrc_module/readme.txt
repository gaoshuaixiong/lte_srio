2014/10/20 刘应涛
在rrc_utils.c中添加函数check_free_ptr,并在rrcfsm.c中调用。该函数用于判断指针是否为NULL，若不是NULL则释放其内存，并将指针指向NULL。这样做的目的是，如果在对一个指针变量进行重新申请内存之前不对其进行检查，那么在重新申请内存之后该指针会指向新的地址，那么该指针以前指向的内存空间便不能访问了，造成内存泄露。