// GCLIB (Free GameCube SDK Libraries)
//
// GC_BBA.c by Costis
//
// Version 1.0
//
// Revisions:
//
// V1.0 - Added all of the current functions, etc.
//          Note: They are all synchronous and do not
//                use any interrupts.
//
//
//    Currently, these libraries only have UDP support,
//    as the TCP protocol is far more complicated
//    protocol and I have not yet had the chance to
//    implement it.

#define FUZZIQER_TYPES
#include "types.h"
#include "bba.h"
#include "exi.h"
#ifndef __IS_HOOK
#include "video.h"
#endif

#define BBA_CMD_IRMASKALL		0x00
#define BBA_CMD_IRMASKNONE		0xF8

#define BBA_NCRA				0x00		/* Network Control Register A, RW */
#define BBA_NCRA_RESET			(1<<0)	/* RESET */
#define BBA_NCRA_ST0			(1<<1)	/* ST0, Start transmit command/status */
#define BBA_NCRA_ST1			(1<<2)	/* ST1,  " */
#define BBA_NCRA_SR				(1<<3)	/* SR, Start Receive */

#define BBA_IR 0x09		/* Interrupt Register, RW, 00h */
#define BBA_IR_FRAGI       (1<<0)	/* FRAGI, Fragment Counter Interrupt */
#define BBA_IR_RI          (1<<1)	/* RI, Receive Interrupt */
#define BBA_IR_TI          (1<<2)	/* TI, Transmit Interrupt */
#define BBA_IR_REI         (1<<3)	/* REI, Receive Error Interrupt */
#define BBA_IR_TEI         (1<<4)	/* TEI, Transmit Error Interrupt */
#define BBA_IR_FIFOEI      (1<<5)	/* FIFOEI, FIFO Error Interrupt */
#define BBA_IR_BUSEI       (1<<6)	/* BUSEI, BUS Error Interrupt */
#define BBA_IR_RBFI        (1<<7)	/* RBFI, RX Buffer Full Interrupt */

#define BBA_RWP  0x16/*+0x17*/	/* Receive Buffer Write Page Pointer Register */
#define BBA_RRP  0x18/*+0x19*/	/* Receive Buffer Read Page Pointer Register */

#define BBA_WRTXFIFOD 0x48/*-0x4b*/	/* Write TX FIFO Data Port Register */

#define BBA_RX_STATUS_BF      (1<<0)
#define BBA_RX_STATUS_CRC     (1<<1)
#define BBA_RX_STATUS_FAE     (1<<2)
#define BBA_RX_STATUS_FO      (1<<3)
#define BBA_RX_STATUS_RW      (1<<4)
#define BBA_RX_STATUS_MF      (1<<5)
#define BBA_RX_STATUS_RF      (1<<6)
#define BBA_RX_STATUS_RERR    (1<<7)

#define BBA_INIT_TLBP	0x00
#define BBA_INIT_BP		0x01
#define BBA_INIT_RHBP	0x0f
#define BBA_INIT_RWP	BBA_INIT_BP
#define BBA_INIT_RRP	BBA_INIT_BP

unsigned char BBA_ReceiveCommand (unsigned long reg, unsigned char *abuf, unsigned long size)
{
    unsigned long aval;
    exi_select (0, 2, 5);
    aval = BBA_COMMAND_READ | (reg << 8);
    exi_imm (0, (unsigned char*)&aval, 4, EXI_CONTROL_TYPE_WRITE);
    exi_imm_ex (0, abuf, size, EXI_CONTROL_TYPE_READ);
    exi_deselect (0);
}

unsigned char BBA_SendCommand (unsigned long reg, unsigned char *abuf, unsigned long size)
{
    unsigned long aval;
    exi_select (0, 2, 5);
    aval = BBA_COMMAND_WRITE | (reg << 8);
    exi_imm (0, (unsigned char*)&aval, 4, EXI_CONTROL_TYPE_WRITE);
    exi_imm_ex (0, abuf, size, EXI_CONTROL_TYPE_WRITE);
    exi_deselect (0);
}

unsigned char BBA_ReceiveCommandShort (unsigned long reg, unsigned char *abuf, unsigned long size)
{
    unsigned short aval;
    exi_select (0, 2, 5);
    aval = BBA_COMMAND_READS | (reg << 8);
    exi_imm (0, (unsigned char*)&aval, 2, EXI_CONTROL_TYPE_WRITE);
    exi_imm_ex (0, abuf, size, EXI_CONTROL_TYPE_READ);
    exi_deselect (0);
}

unsigned char BBA_SendCommandShort (unsigned long reg, unsigned char *abuf, unsigned long size)
{
    unsigned short aval;
    exi_select (0, 2, 5);
    aval = BBA_COMMAND_WRITES | (reg << 8);
    exi_imm (0, (unsigned char*)&aval, 2, EXI_CONTROL_TYPE_WRITE);
    exi_imm_ex (0, abuf, size, EXI_CONTROL_TYPE_WRITE);
    exi_deselect (0);
}

unsigned char eth_send(u8 *abuf, u16 size)
{
    u8 getbuf, aval;
    u32 i;

    if (size < 60) size = 60;
    BBA_SendCommand (0x48, abuf, size);
    BBA_ReceiveCommand (0x00,&getbuf,1);
    getbuf |= 4;
    aval = 0;
    BBA_SendCommand (0x00,&aval,1);
    BBA_SendCommand (0x00,&getbuf,1);
    while ((volatile u8)(getbuf) & 0x06) BBA_ReceiveCommand (0x00,&getbuf,1);
    return 1;
}

unsigned short eth_receive(unsigned char *abuf)
{
    struct {
        unsigned char aval;
        unsigned char unused1[3];
        unsigned short len,bval,p_read,unused2,p_write,unused3;
        unsigned char descr[4];
    } as;

	/* get current receiver pointers */
    BBA_ReceiveCommand(BBA_RWP, (unsigned char*)&as.p_write, 2);
    BBA_ReceiveCommand(BBA_RRP, (unsigned char*)&as.p_read, 2);
    
    if (as.p_read == as.p_write) return 0;

    bba_page_t page;
    bba_header_t *bba = (bba_header_t *)page;
    size_t size = sizeof(bba_page_t);
    //debug
    DEBUG_PrintChar(0,350,'a');
    DEBUG_ShowValueU32(50,350,size);

    dcache_flush(page, size);
    BBA_ReceiveCommand(as.p_read << 8, page, size);
    as.len = bba->length - sizeof(*bba);
    //debug
    DEBUG_PrintChar(0,370,'b');
    DEBUG_ShowValueU32(50,370,as.len);

    // as.aval = 0x02;
    // BBA_SendCommand (0x3A, &as.aval, 1);
    // BBA_ReceiveCommand (0x3A, &as.aval, 1);

    // if (as.aval & 2) return 0;
    // DEBUG_Print(50,350,"PAST");

    // BBA_ReceiveCommand (as.p_read, as.descr, 4);
    // as.len = (as.descr[1] >> 4) | (as.descr[2] << 4);

    if ((as.p_read + 4 + as.len) <= 0x1000)
        BBA_ReceiveCommand (as.p_read + 4, abuf, as.len);
    else {
        as.bval = 0x1000 - (as.p_read + 4);
        BBA_ReceiveCommand (as.p_read + 4, abuf, as.bval);
        BBA_ReceiveCommand (0x100, abuf + as.bval, as.len - as.bval);
    }

    u16 p_read = 0x0100;
    BBA_SendCommand(BBA_RWP,(unsigned char*)&p_read,2);
    BBA_SendCommand(BBA_RRP,(unsigned char*)&p_read,2);
    as.descr[1] = 0;

    // u16 next = bba->next;
    // BBA_SendCommand (BBA_RRP, &next, 2);

    as.aval = 0x02;
    BBA_SendCommand(BBA_IR, &as.aval, 1);

    as.aval = 0xF8;
    BBA_SendCommandShort(0x02, &as.aval, 1);

    return as.len;
}

#ifndef __IS_HOOK
unsigned char eth_init(u48* mac,u8 speed)
{
    int i;
    unsigned char a;
    unsigned short b;

    a = 0;
    BBA_SendCommand (0x60, (unsigned char*)&a, 1);
    for (i = 0; i < 4; i++) FS_VIDEO_WaitVSync ();
    BBA_ReceiveCommand (0xF, &a, 1);
    for (i = 0; i < 4; i++) FS_VIDEO_WaitVSync ();
    a = 1;
    BBA_SendCommand (0, (unsigned char*)&a, 1);
    for (i = 0; i < 30; i++) FS_VIDEO_WaitVSync ();
    a = 0;
    BBA_SendCommand (0, (unsigned char*)&a, 1);
    BBA_ReceiveCommand (1, (unsigned char*)&a, 1);

    BBA_SendCommand(0x30,(unsigned char*)&speed,1);

    BBA_SendCommandShort(4,"\xD1\x07\x75\x75",2); // MOD LINE 
    a = 0x4E;
    BBA_SendCommandShort(5,&a,1); // MOD LINE 

    BBA_ReceiveCommand (0x5B, (unsigned char*)&a, 1);
    a &= ~(1<<7);
    BBA_SendCommand (0x5B, (unsigned char*)&a, 1);    

    a = 1;
    BBA_SendCommand (0x5E, (unsigned char*)&a, 1);    
    BBA_ReceiveCommand (0x5C, (unsigned char*)&a, 1);
    a |= 4;
    BBA_SendCommand (0x5C, (unsigned char*)&a, 1);    
    a = 0x11;
    BBA_SendCommand (1, (unsigned char*)&a, 1);
    a = 0x80;
    BBA_SendCommand (0x50, (unsigned char*)&a, 1);    
    a = 0xFF;
    BBA_SendCommand (8, (unsigned char*)&a, 1);
    BBA_SendCommand (9, (unsigned char*)&a, 1);
    a = 1;
    
    a = 0;
    BBA_SendCommand (2, (unsigned char*)&a, 1);
    for (i = 0; i < 4; i++) FS_VIDEO_WaitVSync ();

    b = 0x0100;
    BBA_SendCommand (10, (unsigned char*)&b, 1);
    BBA_SendCommand (0x16, (unsigned char*)&b, 2);
    BBA_SendCommand (0x18, (unsigned char*)&b, 2);
    b = 0x0F00;
    BBA_SendCommand (0x1A, (unsigned char*)&b, 2);

    a = 8;
    BBA_SendCommand (0, (unsigned char*)&a, 1);

    for (i = 0; i < 4; i++) FS_VIDEO_WaitVSync (); // MOD LINE 
    //BBA_ReceiveCommand(1,&a,1); // MOD LINE 
    a = 0x11; // MOD LINE 
    BBA_SendCommand(1,&a,1); // MOD LINE 
    //a = 8; // MOD LINE 
    //BBA_SendCommand(0x32,&a,1); // MOD LINE 
    //a = 0xFF; // MOD LINE 
    //BBA_SendCommandShort(2,&a,1); // MOD LINE 
    //BBA_SendCommandShort(3,&a,1); // MOD LINE 

    BBA_ReceiveCommand (0x20,(unsigned char*)mac,6);
    return 1;
}

u8 eth_getspeed()
{
    unsigned char res;
    BBA_ReceiveCommand(0x30,(unsigned char*)&res,1);
    return res;
}

void eth_setspeed(u8 speed)
{
    unsigned char res = speed;
    BBA_SendCommand(0x30,(unsigned char*)&res,1);
}
#endif

