/*
 * Copyright (c) 2011, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 *  ======== Deh.c ========
 *
 */
#include <xdc/runtime/Assert.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Startup.h>
#include <xdc/runtime/Memory.h>
#include <ti/sysbios/family/arm/m3/Hwi.h>
#include <ti/sysbios/family/arm/ducati/Core.h>
#include <ti/sysbios/hal/ammu/AMMU.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/knl/Task.h>
#include "package/internal/Deh.xdc.h"
#include <ti/ipc/rpmsg/VirtQueue.h>
#include <xdc/cfg/global.h>
#include <ti/ipc/MultiProc.h>

/*
 *  ======== Deh_Module_startup ========
 */
Int Deh_Module_startup(Int phase)
{
    if (AMMU_Module_startupDone() == TRUE) {
        return Startup_DONE;
    }
    return Startup_NOTDONE;
}

/* write buffer to the crash-dump buffer */
Void Deh_writeBuf(char *t)
{
    while (*t != '\0') {
        module->outbuf[module->index++] = *t++;
        if (module->index == Deh_bufSize) {
            module->index = 0;
        }
    }

    module->outbuf[module->index] = '\0';
}

/* read data from HWI exception handler and print it to crash dump */
/* buffer. Notify host exception has occurred                      */
Void Deh_excHandler(UInt *excStack, UInt lr)
{
    Hwi_ExcContext  exc;
    Char           *ttype;
    UInt            excNum;
    UInt8          *pc;

    /* copy registers from stack to excContext */
    exc.r0 = (Ptr)excStack[8];      /* r0 */
    exc.r1 = (Ptr)excStack[9];      /* r1 */
    exc.r2 = (Ptr)excStack[10];     /* r2 */
    exc.r3 = (Ptr)excStack[11];     /* r3 */
    exc.r4 = (Ptr)excStack[0];      /* r4 */
    exc.r5 = (Ptr)excStack[1];      /* r5 */
    exc.r6 = (Ptr)excStack[2];      /* r6 */
    exc.r7 = (Ptr)excStack[3];      /* r7 */
    exc.r8 = (Ptr)excStack[4];      /* r8 */
    exc.r9 = (Ptr)excStack[5];      /* r9 */
    exc.r10 = (Ptr)excStack[6];     /* r10 */
    exc.r11 = (Ptr)excStack[7];     /* r11 */
    exc.r12 = (Ptr)excStack[12];    /* r12 */
    exc.sp  = (Ptr)(Uint32)(excStack+16); /* sp */
    exc.lr  = (Ptr)excStack[13];    /* lr */
    exc.pc  = (Ptr)excStack[14];    /* pc */
    exc.psr = (Ptr)excStack[15];    /* psr */

    exc.threadType = BIOS_getThreadType();
    switch (exc.threadType) {
        case BIOS_ThreadType_Task:
            if (BIOS_taskEnabled == TRUE) {
                exc.threadHandle = (Ptr)Task_self();
                exc.threadStack = (Task_self())->stack;
                exc.threadStackSize = (Task_self())->stackSize;
            }
            break;
        case BIOS_ThreadType_Swi:
            if (BIOS_swiEnabled == TRUE) {
                exc.threadHandle = (Ptr)Swi_self();
                exc.threadStack = module->isrStackBase;
                exc.threadStackSize = module->isrStackSize;
            }
            break;
        case BIOS_ThreadType_Hwi:
        case BIOS_ThreadType_Main:
            exc.threadHandle = NULL;
            exc.threadStack = module->isrStackBase;
            exc.threadStackSize = module->isrStackSize;
            break;
        default:
            exc.threadHandle = NULL;
            exc.threadStack = NULL;
            exc.threadStackSize = 0;
            break;
    }

    exc.ICSR = (Ptr)Hwi_nvic.ICSR;
    exc.MMFSR = (Ptr)Hwi_nvic.MMFSR;
    exc.BFSR = (Ptr)Hwi_nvic.BFSR;
    exc.UFSR = (Ptr)Hwi_nvic.UFSR;
    exc.HFSR = (Ptr)Hwi_nvic.HFSR;
    exc.DFSR = (Ptr)Hwi_nvic.DFSR;
    exc.MMAR = (Ptr)Hwi_nvic.MMAR;
    exc.BFAR = (Ptr)Hwi_nvic.BFAR;
    exc.AFSR = (Ptr)Hwi_nvic.AFSR;

    /* Force MAIN threadtype So we can safely call System_printf */
    BIOS_setThreadType(BIOS_ThreadType_Main);

    switch (lr) {
        case 0xfffffff1:
            System_printf("Exception occurred. Core in ISR context.\n");
            break;
        case 0xfffffff9:
        case 0xfffffffd:
            System_printf("Exception occurred. Core in thread context\n");
            break;
        default:
            System_printf("Unknown Exception occurred. LR: %08x.\n", lr);
            break;
    }

    switch (exc.threadType) {
        case BIOS_ThreadType_Task: {
            ttype = "Task";
            break;
        }
        case BIOS_ThreadType_Swi: {
            ttype = "Swi";
            break;
        }
        case BIOS_ThreadType_Hwi: {
            ttype = "Hwi";
            break;
        }
        case BIOS_ThreadType_Main: {
            ttype = "Main";
            break;
        }
        default:
            ttype = "Invalid!";
            break;
    }

    System_printf("Exception occurred in BIOS ThreadType_%s.\n", ttype);
    System_printf("BIOS %s handle: 0x%x.\n", ttype, exc.threadHandle);
    System_printf("BIOS %s stack base: 0x%x.\n", ttype, exc.threadStack);
    System_printf("BIOS %s stack size: 0x%x.\n\n", ttype,
        exc.threadStackSize);

    excNum = Hwi_nvic.ICSR & 0xff;
    switch (excNum) {
        case 2:
            System_printf("Hwi_E_NMI\n");
            break;
        case 3:
            Deh_excHardFault();
            break;
        case 4:
            Deh_excMemFault();
            break;
        case 5:
            Deh_excBusFault();
            break;
        case 6:
            Deh_excUsageFault();
            break;
        case 11:
            pc = (UInt8 *)excStack[14];
            System_printf("Hwi_E_svCall ID:%d\n", pc[-2]);
            break;
        case 12:
            System_printf("Hwi_E_debugMon DFSR: %08x\n", Hwi_nvic.DFSR);
            break;
        case 7:
        case 8:
        case 9:
        case 10:
        case 13:
            System_printf("Hwi_E_reserved: Num:%d\n", excNum);
            break;
        default:
            System_printf("Hwi_E_noIsr: Num:%d PC:%d\n", excNum, exc.pc);
            break;
    }

    System_printf ("\nR0 = 0x%08x  R8  = 0x%08x\n", exc.r0, exc.r8);
    System_printf ("R1 = 0x%08x  R9  = 0x%08x\n", exc.r1, exc.r9);
    System_printf ("R2 = 0x%08x  R10 = 0x%08x\n", exc.r2, exc.r10);
    System_printf ("R3 = 0x%08x  R11 = 0x%08x\n", exc.r3, exc.r11);
    System_printf ("R4 = 0x%08x  R12 = 0x%08x\n", exc.r4, exc.r12);
    System_printf ("R5 = 0x%08x  SP(R13) = 0x%08x\n", exc.r5, exc.sp);
    System_printf ("R6 = 0x%08x  LR(R14) = 0x%08x\n", exc.r6, exc.lr);
    System_printf ("R7 = 0x%08x  PC(R15) = 0x%08x\n", exc.r7, exc.pc);
    System_printf ("PSR = 0x%08x\n", exc.psr);
    System_printf ("ICSR = 0x%08x\n", Hwi_nvic.ICSR);
    System_printf ("MMFSR = 0x%02x\n", Hwi_nvic.MMFSR);
    System_printf ("BFSR = 0x%02x\n", Hwi_nvic.BFSR);
    System_printf ("UFSR = 0x%04x\n", Hwi_nvic.UFSR);
    System_printf ("HFSR = 0x%08x\n", Hwi_nvic.HFSR);
    System_printf ("DFSR = 0x%08x\n", Hwi_nvic.DFSR);
    System_printf ("MMAR = 0x%08x\n", Hwi_nvic.MMAR);
    System_printf ("BFAR = 0x%08x\n", Hwi_nvic.BFAR);
    System_printf ("AFSR = 0x%08x\n", Hwi_nvic.AFSR);
    Deh_writeBuf ("Core crashed.\nSee TraceBuffer for register-dump.\n");
    System_abort("Terminating execution...\n");

}

/*! Decode memory fault registers */
Void Deh_excMemFault(Void)
{
    Char *fault;

    if (Hwi_nvic.MMFSR) {
        if (Hwi_nvic.MMFSR & 0x10) {
            fault = "MSTKERR:Access violation (RD/WR failed). Stack Push";
        }
        else if (Hwi_nvic.MMFSR & 0x08) {
            fault = "MUNSTKERR:Access violation (RD/WR failed). Stack Pop";
        }
        else if (Hwi_nvic.MMFSR & 0x02) {
            fault = "DACCVIOL:Access violation (RD/WR failed). Stack Push";
        }
        else if (Hwi_nvic.MMFSR & 0x01) {
            fault = "IACCVIOL:Instruction violation. Invalid region fetch";
        }
        else {
            fault = "Unknown";
        }
        System_printf("Hwi_E_memFault %s", fault);
        if (Hwi_nvic.MMFSR & 0x80) {
            System_printf("Hwi_E_memFault: location: %08x\n", Hwi_nvic.MMAR);
        }
    }
}

/*! Decode bus fault registers */
Void Deh_excBusFault(Void)
{
    Char *fault;

    if (Hwi_nvic.BFSR) {
        if (Hwi_nvic.BFSR & 0x10) {
            fault = "STKERR:Bus Fault caused by Stack Push";
        }
        else if (Hwi_nvic.BFSR & 0x08) {
            fault = "UNSTKERR:Bus Fault caused by Stack Pop";
        }
        else if (Hwi_nvic.BFSR & 0x04) {
            fault = "IMPRECISERR:Delayed Bus Fault. Exact addr unknown";
        }
        else if (Hwi_nvic.BFSR & 0x02) {
            fault = "PRECISERR:Bus Fault";
        }
        else if (Hwi_nvic.BFSR & 0x01) {
            fault = "IBUSERR:Instruction violation. Invalid region fetch";
        }
        else {
            fault = "Unknown";
        }
        System_printf("Hwi_E_busFault %s\n", fault);
        if (Hwi_nvic.BFSR & 0x80) {
            System_printf("Hwi_E_busFault: location: %08x\n", Hwi_nvic.BFAR);
        }
    }
}

/*! Decode usage fault registers */
Void Deh_excUsageFault(Void)
{
    Char *fault;

    if (Hwi_nvic.UFSR) {
        if (Hwi_nvic.UFSR & 0x0001) {
            fault = "UNDEFINSTR:Undefined instruction";
        }
        else if (Hwi_nvic.UFSR & 0x0002) {
            fault = "INVSTATE:Invalid EPSR and instruction combination";
        }
        else if (Hwi_nvic.UFSR & 0x0004) {
            fault = "INVPC:Invalid PC";
        }
        else if (Hwi_nvic.UFSR & 0x0008) {
            fault = "NOCP:Attempting to use co-processor";
        }
        else if (Hwi_nvic.UFSR & 0x0100) {
            fault = "UNALIGNED:Unaligned memory access";
        }
        else if (Hwi_nvic.UFSR & 0x0200) {
            fault = "DIVBYZERO";
        }
        else {
            fault = "Unknown";
        }
        System_printf("Hwi_E_usageFault %s\n", fault);
    }
}

/*! Decode hardfault registers */
Void Deh_excHardFault(Void)
{
    Char *fault;

    if (Hwi_nvic.HFSR) {
        if (Hwi_nvic.HFSR & 0x40000000) {
            System_printf("Hwi_E_hardFault: FORCED\n");
            Deh_excUsageFault();
            Deh_excBusFault();
            Deh_excMemFault();
            return;
        }
        else if (Hwi_nvic.HFSR & 0x80000000) {
            fault = "DEBUGEVT";
        }
        else if (Hwi_nvic.HFSR & 0x00000002) {
            fault = "VECTBL";
        }
        else {
            fault = "Unknown";
        }
        System_printf("Hwi_E_hardFault %s\n", fault);
    }
}
