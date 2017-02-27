#ifndef DLP_HHHH
#define DLP_HHHH

#define DLP_MAGIC 'K'
#define DLP_NORMAL _IO(DLP_MAGIC, 0)
#define DLP_HORIZONTAL  _IO(DLP_MAGIC,1) 
#define DLP_VERTICAL  _IO(DLP_MAGIC, 2)
#define DLP_SEQ _IOW(DLP_MAGIC, 3,unsigned int)
#define DLP_IF _IOW(DLP_MAGIC, 4,unsigned int)
#define DLP_RESOLUTION _IOW(DLP_MAGIC, 5,unsigned int)
#define DLP_CHANNEL _IOW(DLP_MAGIC, 6,unsigned int)




#define DLP_MAJOR		255



#endif
