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
 */

#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/Assert.h>
#include <ti/ipc/MultiProc.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/srvmgr/NameMap.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ti/ipc/rpmsg/MessageQCopy.h>
#include "IpcResource.h"
#include "_IpcResource.h"

#define DEFAULT_TIMEOUT 500
#define MAXMSGSIZE 128
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

static UInt16 IpcResource_resLen(IpcResource_Type type)
{
    switch (type) {
    case IpcResource_TYPE_GPTIMER:
        return sizeof(IpcResource_Gpt);
    case IpcResource_TYPE_AUXCLK:
        return sizeof(IpcResource_Auxclk);
    case IpcResource_TYPE_REGULATOR:
        return sizeof(IpcResource_Regulator);
    case IpcResource_TYPE_GPIO:
        return sizeof(IpcResource_Gpio);
    case IpcResource_TYPE_SDMA:
        return sizeof(IpcResource_Sdma);
    case IpcResource_TYPE_I2C:
        return sizeof(IpcResource_I2c);
    }
    return 0;
}

static Char *IpcResource_names[] = {
    "omap-gptimer",
    "iva",
    "iva_seq0",
    "iva_seq1",
    "l3bus",
    "omap-iss",
    "omap-fdif",
    "omap-sl2if",
    "omap-auxclk",
    "regulator",
    "gpio",
    "omap-sdma",
    "ipu",
    "dsp",
    "i2c"
};

static Char *IpcResource_toName(IpcResource_Type type)
{
    if (type < ARRAY_SIZE(IpcResource_names))
        return IpcResource_names[type];
    return NULL;
}

static Int _IpcResource_translateError(Int kstatus)
{
    switch (kstatus) {
    case 0:
        return IpcResource_S_SUCCESS;
    case -ENOENT:
        return IpcResource_E_NORESOURCE;
    case -ENOMEM:
        return IpcResource_E_RETMEMORY;
    case -EBUSY:
        return IpcResource_E_BUSY;
    case -EINVAL:
        return IpcResource_E_INVALARGS;
    }
    return IpcResource_E_FAIL;
}

IpcResource_Handle IpcResource_connect(UInt timeout)
{
    UInt16 dstProc;
    UInt16 len;
    IpcResource_Handle handle;
    IpcResource_Ack ack;
    Int status;

    handle = Memory_alloc(NULL, sizeof(*handle), 0, NULL);
    if (!handle) {
        System_printf("IpcResource_connect: No memory");
        return NULL;
    }

    handle->timeout = (!timeout) ? DEFAULT_TIMEOUT :
                      (timeout == IpcResource_FOREVER) ? MessageQCopy_FOREVER :
                      timeout;

    dstProc = MultiProc_getId("HOST");
    handle->sem = Semaphore_create(1, NULL, NULL);

    MessageQCopy_init(dstProc);
    handle->msgq= MessageQCopy_create(MessageQCopy_ASSIGN_ANY,
                                      &handle->endPoint);
    if (!handle->msgq) {
        System_printf("IpcResource_connect: MessageQCopy_create failed\n");
        goto err;
    }

    NameMap_register("rpmsg-resmgr", handle->endPoint);

    /* wait for the connection ack from the host */
    status = MessageQCopy_recv(handle->msgq, &ack, &len, &handle->remote,
                               handle->timeout);
    if (status) {
        System_printf("IpcResource_connect: MessageQCopy_recv "
                      "failed status %d\n", status);
        goto err_disc;
    }

    if (len < sizeof(ack)) {
        System_printf("IpcResource_connect: Bad ack message len = %d\n", len);
        goto err_disc;
    }

    status = _IpcResource_translateError(ack.status);
    if (status) {
        System_printf("IpcResource_connect: A9 Resource Manager "
                      "failed status %d\n", status);
        goto err_disc;
    }

    return handle;
err_disc:
    NameMap_unregister("rpmsg-resmgr", handle->endPoint);
    MessageQCopy_delete(&handle->msgq);
err:
    Memory_free(NULL, handle, sizeof(*handle));
    return NULL;
}

Int IpcResource_disconnect(IpcResource_Handle handle)
{
    Int status;

    if (!handle) {
        System_printf("IpcResource_disconnect: handle is NULL\n");
        return IpcResource_E_INVALARGS;
    }

    NameMap_unregister("rpmsg-resmgr", handle->endPoint);

    status = MessageQCopy_delete(&handle->msgq);
    if (status) {
        System_printf("IpcResource_disconnect: MessageQCopy_delete "
                      "failed status %d\n", status);
        return status;
    }

    Semaphore_delete(&handle->sem);

    Memory_free(NULL, handle, sizeof(*handle));

    return IpcResource_S_SUCCESS;
}

Int IpcResource_request(IpcResource_Handle handle,
                        IpcResource_ResHandle *resHandle,
                        IpcResource_Type type, Void *resParams)
{
    Char msg[MAXMSGSIZE];
    IpcResource_Act *act = (Void *)msg;
    IpcResource_Req *req = (Void *)act->data;
    IpcResource_Ack *ack = (Void *)msg;
    IpcResource_ReqAck *rack = (Void *)ack->data;
    _IpcResource_Auxclk _auxclk;
    IpcResource_Regulator *reg;
    _IpcResource_Regulator _reg;
    IpcResource_Auxclk *auxclk;
    UInt16 hlen = sizeof(*req) + sizeof(*act);
    UInt16 alen = sizeof(*ack);
    UInt16 rlen = IpcResource_resLen(type);
    UInt16 len;
    UInt32 remote;
    Int status;
    Char *name;
    /* names to translate auxclk parent IDs */
    Char *auxclkSrcName[] = {
        "sys_clkin_ck",
        "dpll_core_m3x2_ck",
        "dpll_per_m3x2_ck",
    };

    if (!handle || !resHandle) {
        System_printf("IpcResource_request: Invalid paramaters\n");
        return IpcResource_E_INVALARGS;
    }

    name = IpcResource_toName(type);
    if (!name) {
        System_printf("IpcResource_request: resource type %d "
                      "no valid\n", type);
        return IpcResource_E_INVALARGS;
    }

    if (rlen && !resParams) {
        System_printf("IpcResource_request: resource type %d "
                      "needs parameters\n", type);
        return IpcResource_E_INVALARGS;
    }

    strncpy(req->resName, name, 16);
    act->action = IpcResource_REQ_TYPE_ALLOC;

    switch(type) {
    case IpcResource_TYPE_AUXCLK:
        auxclk = resParams;
	/* build auxclk name based on the ID */
        sprintf(_auxclk.name, "auxclk%d_ck", auxclk->clkId);
        _auxclk.clkRate = auxclk->clkRate;

        if (auxclk->parentSrcClk >=
                sizeof(auxclkSrcName) / sizeof(*auxclkSrcName))
            return IpcResource_E_INVALARGS;

        strcpy(_auxclk.parentName, auxclkSrcName[auxclk->parentSrcClk]);
        _auxclk.parentSrcClkRate = auxclk->parentSrcClkRate;
	resParams = &_auxclk;
	rlen = sizeof(_auxclk);
        break;
    case IpcResource_TYPE_REGULATOR:
        reg = resParams;

	/* we only support cam2pwr regulator at this moment */
        if (reg->regulatorId != 1)
            return IpcResource_E_INVALARGS;

        strcpy(_reg.name, "cam2pwr");
        _reg.minUV = reg->minUV;
        _reg.maxUV = reg->maxUV;
	resParams = &_reg;
	rlen = sizeof(_reg);
    }

    memcpy(req->resParams, resParams, rlen);
    Semaphore_pend(handle->sem, BIOS_WAIT_FOREVER);
    status = MessageQCopy_send(MultiProc_getId("HOST"), handle->remote,
                        handle->endPoint, act, hlen + rlen);
    if (status) {
        Semaphore_post(handle->sem);
        System_printf("IpcResource_request: MessageQCopy_send "
                      "failed status %d\n", status);
        status = IpcResource_E_FAIL;
        goto end;
    }

    status = MessageQCopy_recv(handle->msgq, ack, &len, &remote,
                               handle->timeout);
    Semaphore_post(handle->sem);
    if (remote != handle->remote) {
        System_printf("IpcResource_request: MessageQCopy_recv "
                      "invalid message source %d, channel source %d\n",
                      remote, handle->remote);
        status = IpcResource_E_FAIL;
        goto end;
    }

    if (status) {
        System_printf("IpcResource_request: MessageQCopy_recv "
                      "failed status %d\n", status);
        status = (status == MessageQCopy_E_TIMEOUT) ? IpcResource_E_TIMEOUT :
                 IpcResource_E_FAIL;
        goto end;
    }

    status = _IpcResource_translateError(ack->status);
    if (status) {
        System_printf("IpcResource_request: error from Host "
                      "failed status %d\n", status);
        goto end;
    }

    Assert_isTrue(len == rlen + alen + sizeof(*rack), NULL);

    *resHandle = rack->resHandle;
    memcpy(resParams, rack->resParams, rlen);
end:
    return status;
}

Int IpcResource_setConstraints(IpcResource_Handle handle,
                               IpcResource_ResHandle resHandle,
                               UInt32 action,
                               Void *constraints)
{
    Char msg[MAXMSGSIZE];
    IpcResource_Ack *ack = (Void *)msg;
    IpcResource_Act *act = (Void *)msg;
    IpcResource_Constraint *c = (Void *)act->data;
    UInt16 rlen = sizeof(*c);
    UInt16 alen = sizeof(*ack);
    UInt16 len;
    UInt32 remote;
    Int status;

    if (!handle) {
        System_printf("IpcResource_setConstraints: Invalid paramaters\n");
        return IpcResource_E_INVALARGS;
    }

    if (!constraints) {
        System_printf("IpcResource_setConstraints: needs parameters\n");
        return IpcResource_E_INVALARGS;
    }

    act->action = action;
    c->resHandle = resHandle;

    memcpy(&c->cdata, constraints, rlen);
    Semaphore_pend(handle->sem, BIOS_WAIT_FOREVER);
    status = MessageQCopy_send(MultiProc_getId("HOST"), handle->remote,
                        handle->endPoint, act, sizeof(*act) + rlen);
    if (status) {
        Semaphore_post(handle->sem);
        System_printf("IpcResource_setConstraints: MessageQCopy_send "
                      "failed status %d\n", status);
        status = IpcResource_E_FAIL;
        goto end;
    }

    if (action == IpcResource_REQ_TYPE_REL_CONSTRAINTS) {
        Semaphore_post(handle->sem);
        goto end;
    }

    status = MessageQCopy_recv(handle->msgq, ack, &len, &remote,
                               handle->timeout);
    Semaphore_post(handle->sem);
    if (status) {
        System_printf("IpcResource_setConstraints: MessageQCopy_recv "
                      "failed status %d\n", status);
        status = (status == MessageQCopy_E_TIMEOUT) ? IpcResource_E_TIMEOUT :
                 IpcResource_E_FAIL;
        goto end;
    }

    if (remote != handle->remote) {
        System_printf("IpcResource_setConstraints: MessageQCopy_recv "
                      "invalid message source %d, channel source %d\n",
                      remote, handle->remote);
        status = IpcResource_E_FAIL;
        goto end;
    }

    status = _IpcResource_translateError(ack->status);
    if (status) {
        System_printf("IpcResource_setConstraints: error from Host "
                      "failed status %d\n", status);
        goto end;
    }

    Assert_isTrue(len == (rlen + alen), NULL);
end:
    return status;
}

Int IpcResource_requestConstraints(IpcResource_Handle handle,
                                   IpcResource_ResHandle resHandle,
                                   Void *constraints)
{
    return IpcResource_setConstraints(handle,
                                      resHandle,
                                      IpcResource_REQ_TYPE_REQ_CONSTRAINTS,
                                      constraints);
}

Int IpcResource_releaseConstraints(IpcResource_Handle handle,
                                   IpcResource_ResHandle resHandle,
                                   Void *constraints)
{
    return IpcResource_setConstraints(handle,
                                      resHandle,
                                      IpcResource_REQ_TYPE_REL_CONSTRAINTS,
                                      constraints);
}

Int IpcResource_release(IpcResource_Handle handle,
                        IpcResource_ResHandle resHandle)
{
    Int status;
    Char msg[sizeof(IpcResource_Act) + sizeof(IpcResource_Rel)];
    IpcResource_Act *act = (Void *)msg;
    IpcResource_Rel *rel = (Void *)act->data;

    if (!handle) {
        System_printf("IpcResource_release: handle is NULL\n");
        return IpcResource_E_INVALARGS;
    }

    act->action = IpcResource_REQ_TYPE_FREE;
    rel->resHandle = resHandle;

    status = MessageQCopy_send(MultiProc_getId("HOST"), handle->remote,
                      handle->endPoint, act, sizeof(msg));
    if (status) {
        System_printf("IpcResource_release: MessageQCopy_send "
                      "failed status %d\n", status);
    }

    return status;
}
