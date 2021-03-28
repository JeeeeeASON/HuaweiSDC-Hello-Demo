
#ifndef SDC_IOCTL_H
#define SDC_IOCTL_H

#include <sys/ioctl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* 服务通信的句柄可以用ioctl来设置通信缓冲区的大小
*/
#define SDC_FT_BUFFER  2

/** 
* 设置读写缓冲区大小, size值而非地址作为ioctl的第三个参数
* ioctl( fd, SDC_BUFF_SETWRSIZE, size)
*/
#define SDC_BUFF_SETWRSIZE _IO(SDC_FT_BUFFER,0x11)
#define SDC_BUFF_SETRDSIZE _IO(SDC_FT_BUFFER,0x13)

#define SDC_BUFF_GETWRSIZE _IOR(SDC_FT_BUFFER,0x12,unsigned int)
#define SDC_BUFF_GETRDSIZE _IOR(SDC_FT_BUFFER,0x14,unsigned int)
#define SDC_BUFF_GETMSGSIZE _IOR(SDC_FT_BUFFER,0x15,unsigned int)


#ifdef __cplusplus
}
#endif

#endif

