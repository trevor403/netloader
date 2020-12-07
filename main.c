#define MAIN_FILE

#include "gamecube.h"

#include "message.h"
#include "video.h"

#include "bba.h"
#include "cs-net.h"
#include "cs-types.h"
#include "memorymap.h"
// #include "cs.h"

#include "debugmenus.h"

int main() {
    u32 x,y;
    setalign(0); // THIS MUST BE DONE BEFORE ANYTHING ELSE!! 
    mallocset(fb,320 * 480 * 4); // allocate framebuffer
    // debugEnable(true);
    // debugDrawInfo();

    PAD pad;
    exception_init();
    exi_init();
    u32 msr;
    msr = GetMSR();
    msr &= ~0x8000;
    msr |= 0x2002;
    SetMSR(msr);
    FS_PAD_Init();
    FS_PAD_ReadAllState(&pad);

    u32 zero = 0;
    char ipldesc[0x100];
    exi_select(0,1,3);
    exi_imm(0,&zero,4,EXI_WRITE);
    exi_imm_ex(0,&ipldesc,0x100,EXI_READ);
    exi_deselect(0);
    *(u32*)0x800000CC = 1;

    FS_VIDEO_Init(FS_VIDEO_640X480_NTSC_YUV16);
    FS_VIDEO_SetFrameBuffer(FS_VIDEO_FRAMEBUFFER_BOTH,(u32)fb);

    *(unsigned long*)0x8000001C = 0xC2339F3D;
    *(unsigned long*)0x80000020 = 0x0D15EA5E;
    *(unsigned long*)0x80000024 = 1;
    *(unsigned long*)0x80000028 = 0x017F8000;
    *(unsigned long*)0x8000002C = 1 + ((*(unsigned long*)0xCC00302C) >> 28);
    *(unsigned long*)0x800000EC = 0x817F8000;
    *(unsigned long*)0x800000F0 = 0x017F8000;
    *(unsigned long*)0x800000F8 = 162000000;
    *(unsigned long*)0x800000FC = 486000000;
    *(unsigned short*)0x800030E0 = 6;
    dcache_flush((void*)0x80000000,0x100);

    GRAPH_SetFramebuffer(fb);
    DEBUG_InitText(fb,COLOR_YELLOW);

    GCARSClearFrameBuffer(COLOR_BLACK);
    GRAPH_Rectangle(0,40,320,2,COLOR_QRHYGREEN);
    GRAPH_Rectangle(0,74,320,2,COLOR_QRHYGREEN);
    DEBUG_SetTextColor(COLOR_YELLOW);
    DEBUG_Print(50,50 ,"Establish BBA Network Connection");

    // GRAPH_Rectangle(0,102,320,30,COLOR_BLACK);
    DEBUG_Print(50,110,"Setting up network");
    CSDefaultNetwork();

    // GRAPH_Rectangle(0,102,320,30,COLOR_BLACK);
    // DEBUG_Print(50,110,"Stopping the disc drive");
    // dvd_stop();

    // GRAPH_Rectangle(0,102,320,30,COLOR_BLACK);

    if (exi_deviceid(0,2) != 0x04020200) {
        DEBUG_Print(50,170,"Ethernet Adapter is missing!");
        while(true) {}
    }

    DEBUG_Print(50,170,"Starting Ethernet Adapter");
    eth_init(&(CS_ENTITY->MAC),CS_ENTITY->speed);

    // GRAPH_Rectangle(0,102,320,30,COLOR_BLACK);
    // DEBUG_Print(50,110,"Clearing hook memory");
    // memset((void*)MEMORY_START,0,(u32)MEMORY_TOP - (u32)MEMORY_START);

    // if (*(u32*)0x81300000 != 0x38210000) GCARSSelectCardSlot();
    // GCARSLoadCodes();
    // *(u32*)0x81300000 = 0x38210000;

    *(u32*)0x80000028 = (u32)MEMORY_START & 0x01FFFFFF;
    *(u32*)0x80000034 = 0;
    *(u32*)0x80000038 = 0;
    *(u32*)0x800000EC = (u32)MEMORY_START;
    *(u32*)0x800000F0 = (u32)MEMORY_START & 0x01FFFFFF;
    *(u32*)0x800000F4 = 0;

    DEBUG_Print(50,230,"Contacting the DHCP server");
    CSNetDoDHCP();
    // CS_DATA->IPAddress[CS_DATA->localPad] = CS_ENTITY->IP;

    DEBUG_Print(50,430,"Done!");
    // DEBUG_ShowValueU32(160,430,CS_ENTITY->IP);

    // GRAPH_Rectangle(0,102,320,400,COLOR_BLACK);
    // CSNetInit(CS_ENTITY->IP);
    DEBUG_Print(200,450,"ARPing Default Gateway...");
    CSNetARPAddress(CS_ENTITY->MAC,&(CS_ENTITY->defGatewayMAC),CS_ENTITY->IP,CS_ENTITY->defGateway);

    while(true) {
        FS_VIDEO_WaitVSync();
    }

    return 0;
}

void __eabi(void) {}