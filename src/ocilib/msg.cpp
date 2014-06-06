/*
    +-----------------------------------------------------------------------------------------+
    |                                                                                         |
    |                               OCILIB - C Driver for Oracle                              |
    |                                                                                         |
    |                                (C Wrapper for Oracle OCI)                               |
    |                                                                                         |
    |                              Website : http://www.ocilib.net                            |
    |                                                                                         |
    |             Copyright (c) 2007-2012 Vincent ROGIER <vince.rogier@ocilib.net>            |
    |                                                                                         |
    +-----------------------------------------------------------------------------------------+
    |                                                                                         |
    |             This library is free software; you can redistribute it and/or               |
    |             modify it under the terms of the GNU Lesser General Public                  |
    |             License as published by the Free Software Foundation; either                |
    |             version 2 of the License, or (at your option) any later version.            |
    |                                                                                         |
    |             This library is distributed in the hope that it will be useful,             |
    |             but WITHOUT ANY WARRANTY; without even the implied warranty of              |
    |             MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU           |
    |             Lesser General Public License for more details.                             |
    |                                                                                         |
    |             You should have received a copy of the GNU Lesser General Public            |
    |             License along with this library; if not, write to the Free                  |
    |             Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.          |
    |                                                                                         |
    +-----------------------------------------------------------------------------------------+
*/

/* --------------------------------------------------------------------------------------------- *
 * $Id: msg.c, Vincent Rogier $
 * --------------------------------------------------------------------------------------------- */

#include "ocilib_internal.h"

/* ********************************************************************************************* *
 *                            PUBLIC FUNCTIONS
 * ********************************************************************************************* */

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgCreate
 * --------------------------------------------------------------------------------------------- */

OCI_Msg * OCI_API OCI_MsgCreate
(
    OCI_Library *pOCILib,
    OCI_TypeInfo *typinf,
    ExceptionSink* xsink
)
{
    OCI_Msg *msg = NULL;
    boolean res  = TRUE;

    OCI_CHECK_INITIALIZED2(pOCILib, NULL);

    OCI_CHECK_PTR(pOCILib, OCI_IPC_TYPE_INFO, typinf, NULL);

    /* allocate message structure */

    msg = (OCI_Msg *) OCI_MemAlloc2(pOCILib, OCI_IPC_MSG, sizeof(*msg), (size_t) 1, TRUE);

    if (msg != NULL)
    {
        msg->typinf = typinf;
        msg->ind    = OCI_IND_NULL;

        /* allocate message properties descriptor */

        res = (OCI_SUCCESS == OCI_DescriptorAlloc2(pOCILib, (dvoid * ) pOCILib->env,
                                                  (dvoid **) &msg->proph,
                                                  OCI_DTYPE_AQMSG_PROPERTIES,
                                                  (size_t) 0, (dvoid **) NULL));

        if (res == TRUE)
        {
            /* allocate internal OCI_Object handle if payload is not RAW */

            if (msg->typinf->tcode != OCI_UNKNOWN)
            {
 	        msg->obj = OCI_ObjectCreate2(pOCILib, typinf->con, typinf, xsink);

                res = (msg->obj != NULL);
            }
        }
    }
    else
    {
        res = FALSE;
    }

    /* check for failure */

    if (res == FALSE)
    {
       OCI_MsgFree(pOCILib, msg, xsink);
        msg = NULL;
    }

    return msg;
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgFree
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgFree
(
    OCI_Library *pOCILib,
    OCI_Msg *msg,
    ExceptionSink* xsink
)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, FALSE);

    /* free local OCI_Agent object */

    if (msg->sender != NULL)
    {
        OCI_AgentFree(pOCILib, msg->sender);
    }

    /* free internal OCI_Object handle if payload is not RAW */

    if (msg->obj != NULL)
    {
        msg->obj->hstate =  OCI_OBJECT_ALLOCATED;

        OCI_ObjectFree2(pOCILib, msg->obj, xsink);

        msg->obj = NULL;
    }

    /* free message RAW payload if necessary */

    if ((msg->typinf->tcode == OCI_UNKNOWN)&& ( msg->id != NULL))
    {
        OCI_CALL2Q
        (
            pOCILib, res, msg->typinf->con,

            OCIRawResize(pOCILib->env, pOCILib->err, 0, (OCIRaw **) &msg->payload),

	    xsink
        )
    }

    /* free message ID */

    if (msg->id != NULL)
    {

        OCI_CALL2Q
        (
            pOCILib, res, msg->typinf->con,

            OCIRawResize(pOCILib->env, pOCILib->err, 0, (OCIRaw **) &msg->id),

	    xsink
        )
    }

    msg->id = NULL;

    /* free OCI descriptor */

    OCI_DescriptorFree2(pOCILib, (dvoid *) msg->proph, OCI_DTYPE_AQMSG_PROPERTIES);

    OCI_FREE(msg);

    return res;
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgReset
 * --------------------------------------------------------------------------------------------- */
boolean OCI_API OCI_MsgReset
(
    OCI_Library *pOCILib,
    OCI_Msg *msg,
    ExceptionSink *xsink
)
{
    boolean res = TRUE;

    unsigned int len = 0;
    void  *null_ptr  = NULL;

    res =   (
       OCI_MsgSetExpiration(pOCILib, msg, -1, xsink)            &&
       OCI_MsgSetEnqueueDelay(pOCILib, msg, 0, xsink)           &&
       OCI_MsgSetPriority(pOCILib, msg, 0, xsink)                &&
       OCI_MsgSetOriginalID(pOCILib, msg, null_ptr, len, xsink) &&
       OCI_MsgSetSender(pOCILib, msg, NULL, xsink)              &&
       OCI_MsgSetConsumers(pOCILib, msg, (OCI_Agent**)null_ptr, len, xsink)  &&
       OCI_MsgSetCorrelation(pOCILib, msg, NULL, xsink)         &&
       OCI_MsgSetExceptionQueue(pOCILib, msg, NULL, xsink)
       );

    if (res == TRUE)
    {
        if (msg->typinf->tcode == OCI_UNKNOWN)
        {
// TODO/FIXME: raw!            res = OCI_MsgSetRaw(pOCILib, msg, null_ptr, len);
        }
        else
        {
	   res = OCI_MsgSetObject(pOCILib, msg, (OCI_Object*)null_ptr, xsink);
        }
    }

    return res;
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetObject
 * --------------------------------------------------------------------------------------------- */

OCI_Object * OCI_API OCI_MsgGetObject
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
)
{
    OCI_Object *obj = NULL;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, NULL);
    OCI_CHECK_COMPAT(pOCILib, msg->typinf->con, msg->typinf->tcode != OCI_UNKNOWN, NULL);

    if (msg->ind != OCI_IND_NULL)
    {
        obj = (OCI_Object *) msg->obj;
    }

    OCI_RESULT(pOCILib, TRUE);

    return obj;
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetObject
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgSetObject
(
    OCI_Library *pOCILib,
    OCI_Msg    *msg,
    OCI_Object *obj,
    ExceptionSink* xsink
)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, FALSE);

    OCI_CHECK_COMPATQ(pOCILib, msg->typinf->con, msg->typinf->tcode != OCI_UNKNOWN, FALSE, xsink);

    if (obj != NULL)
    {
        /* assign the given object to the message internal object */

       res = OCI_ObjectAssign2(pOCILib, (OCI_Object *) msg->obj, obj, xsink);

        if (res == TRUE)
        {
            msg->ind = OCI_IND_NOTNULL;
        }
    }
    else
    {
        msg->ind = OCI_IND_NULL;
    }

    OCI_RESULT(pOCILib, res);

    return res;
}


/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetRaw
 * --------------------------------------------------------------------------------------------- */
/*
boolean OCI_API OCI_MsgGetRaw
(
    OCI_Library *pOCILib,
    OCI_Msg      *msg,
    void         *raw,
    unsigned int *size
)
{
    unsigned int raw_size = 0;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, FALSE);
    OCI_CHECK_PTR(pOCILib, OCI_IPC_VOID, raw, FALSE);
    OCI_CHECK_PTR(pOCILib, OCI_IPC_VOID, size, FALSE);

    OCI_CHECK_COMPAT(pOCILib, msg->typinf->con, msg->typinf->tcode == OCI_UNKNOWN, FALSE);

    if ((msg->payload != NULL) && (msg->ind != OCI_IND_NULL))
    {
        raw_size = OCIRawSize(pOCILib->env, (OCIRaw *) msg->payload);

        if (*size > raw_size)
        {
            *size = raw_size;
        }

        memcpy(raw, OCIRawPtr(pOCILib->env, msg->payload), (size_t) (*size));
    }
    else
    {
        *size = 0;
    }

    OCI_RESULT(pOCILib, TRUE);

    return TRUE;
}
*/
/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgSetRaw
 * --------------------------------------------------------------------------------------------- */
/*
boolean OCI_API OCI_MsgSetRaw
(
    OCI_Library *pOCILib,
    OCI_Msg      *msg,
    const void   *raw,
    unsigned int  size
)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, FALSE);

    OCI_CHECK_COMPAT(pOCILib, msg->typinf->con, msg->typinf->tcode == OCI_UNKNOWN, FALSE);

    OCI_CALL2
    (
        pOCILib, res, msg->typinf->con,

        OCIRawAssignBytes(pOCILib->env, pOCILib->err,
                          (ub1*) raw, (ub4) size, (OCIRaw **) &msg->payload)
    )

    if ((res == TRUE) && (msg->payload != NULL) && (size > 0))
    {
        msg->ind = OCI_IND_NOTNULL;
    }
    else
    {
        msg->ind = OCI_IND_NULL;
    }

    OCI_RESULT(pOCILib, res);

    return res;
}
*/
/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetAttemptCount
 * --------------------------------------------------------------------------------------------- */
/*
int OCI_API OCI_MsgGetAttemptCount
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
)
{
    boolean res = TRUE;
    sb4 ret     = 0;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, 0);

    OCI_CALL2
    (
        pOCILib, res, msg->typinf->con,

        OCIAttrGet((dvoid *) msg->proph,
                   (ub4    ) OCI_DTYPE_AQMSG_PROPERTIES,
                   (dvoid *) &ret,
                   (ub4   *) NULL,
                   (ub4    ) OCI_ATTR_ATTEMPTS,
                   pOCILib->err)
    )

    OCI_RESULT(pOCILib, res);

    return (int) ret;
}
*/
/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetEnqueueDelay
 * --------------------------------------------------------------------------------------------- */
/*
int OCI_API OCI_MsgGetEnqueueDelay
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
)
{
    boolean res = TRUE;
    sb4 ret     = 0;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, 0);

    OCI_CALL2
    (
        pOCILib, res, msg->typinf->con,

        OCIAttrGet((dvoid *) msg->proph,
                   (ub4    ) OCI_DTYPE_AQMSG_PROPERTIES,
                   (dvoid *) &ret,
                   (ub4   *) NULL,
                   (ub4    ) OCI_ATTR_DELAY,
                   pOCILib->err)
    )

    OCI_RESULT(pOCILib, res);

    return (int) ret;
}
*/
/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgSetEnqueueDelay
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgSetEnqueueDelay
(
    OCI_Library *pOCILib,
    OCI_Msg *msg,
    int      value,
    ExceptionSink* xsink
)
{
    boolean res = TRUE;
    sb4 sval    = (sb4) value;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, 0);

    OCI_CALL2Q
    (
        pOCILib, res, msg->typinf->con,

        OCIAttrSet((dvoid *) msg->proph,
                   (ub4    ) OCI_DTYPE_AQMSG_PROPERTIES,
                   (dvoid *) &sval,
                   (ub4    ) sizeof(sval),
                   (ub4    ) OCI_ATTR_DELAY,
                   pOCILib->err),

	xsink
    )

    OCI_RESULT(pOCILib, res);

    return res;
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetEnqueueTime
 * --------------------------------------------------------------------------------------------- */
/*
OCI_Date * OCI_API OCI_MsgGetEnqueueTime
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
)
{
    boolean res    = TRUE;
    OCI_Date *date = NULL;
    OCIDate oci_date;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, NULL);

    OCI_CALL2
    (
        pOCILib, res, msg->typinf->con,

        OCIAttrGet((dvoid *) msg->proph,
                   (ub4    ) OCI_DTYPE_AQMSG_PROPERTIES,
                   (dvoid *) &oci_date,
                   (ub4   *) NULL,
                   (ub4    ) OCI_ATTR_ENQ_TIME,
                   pOCILib->err)
    )

    if (res == TRUE)
    {
        date = OCI_DateInit2(pOCILib, msg->typinf->con, &msg->date, &oci_date, FALSE, FALSE);

        res = (date != NULL);
    }

    OCI_RESULT(pOCILib, res);

    return date;
}
*/
/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetExpiration
 * --------------------------------------------------------------------------------------------- */
/*
int OCI_API OCI_MsgGetExpiration
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
)
{
    boolean res = TRUE;
    sb4 ret     = 0;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, 0);

    OCI_CALL2
    (
        pOCILib, res, msg->typinf->con,

        OCIAttrGet((dvoid *) msg->proph,
                   (ub4    ) OCI_DTYPE_AQMSG_PROPERTIES,
                   (dvoid *) &ret,
                   (ub4   *) NULL,
                   (ub4    ) OCI_ATTR_EXPIRATION,
                   pOCILib->err)
    )

    OCI_RESULT(pOCILib, res);

    return (int) ret;
}
*/
/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgSetExpiration
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgSetExpiration
(
    OCI_Library *pOCILib,
    OCI_Msg *msg,
    int      value,
    ExceptionSink* xsink
)
{
    boolean res = TRUE;
    sb4 sval    = (sb4) value;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, FALSE);

    OCI_CALL2Q
    (
        pOCILib, res, msg->typinf->con,

        OCIAttrSet((dvoid *) msg->proph,
                   (ub4    ) OCI_DTYPE_AQMSG_PROPERTIES,
                   (dvoid *) &sval,
                   (ub4    ) sizeof(sval),
                   (ub4    ) OCI_ATTR_EXPIRATION,
                   pOCILib->err),

	xsink
    )

    OCI_RESULT(pOCILib, res);

    return res;
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetState
 * --------------------------------------------------------------------------------------------- */
/*
unsigned int OCI_API OCI_MsgGetState
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
)
{
    boolean res = TRUE;
    sb4 ret     = 0;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, OCI_UNKNOWN);

    OCI_CALL2
    (
        pOCILib, res, msg->typinf->con,

        OCIAttrGet((dvoid *) msg->proph,
                   (ub4    ) OCI_DTYPE_AQMSG_PROPERTIES,
                   (dvoid *) &ret,
                   (ub4   *) NULL,
                   (ub4    ) OCI_ATTR_MSG_STATE,
                   pOCILib->err)
    )

    // increment value to handle return code OCI_UNKNOWN on failure

    if (res == TRUE)
    {
        ret++;
    }
    else
    {
        ret = OCI_UNKNOWN;
    }

    OCI_RESULT(pOCILib, res);

    return (int) ret;
}
*/

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetPriority
 * --------------------------------------------------------------------------------------------- */
/*
int OCI_API OCI_MsgGetPriority
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
)
{
    boolean res = TRUE;
    sb4 ret     = 0;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, 0);

    OCI_CALL2
    (
        pOCILib, res, msg->typinf->con,

        OCIAttrGet((dvoid *) msg->proph,
                   (ub4    ) OCI_DTYPE_AQMSG_PROPERTIES,
                   (dvoid *) &ret,
                   (ub4   *) NULL,
                   (ub4    ) OCI_ATTR_PRIORITY,
                   pOCILib->err)
    )

    OCI_RESULT(pOCILib, res);

    return (int) ret;
}
*/

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgSetPriority
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgSetPriority
(
    OCI_Library *pOCILib,
    OCI_Msg *msg,
    int      value,
    ExceptionSink* xsink
)
{
    boolean res = TRUE;
    sb4 sval    = (sb4) value;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, FALSE);

    OCI_CALL2Q
    (
        pOCILib, res, msg->typinf->con,

        OCIAttrSet((dvoid *) msg->proph,
                   (ub4    ) OCI_DTYPE_AQMSG_PROPERTIES,
                   (dvoid *) &sval,
                   (ub4    ) sizeof(sval),
                   (ub4    ) OCI_ATTR_PRIORITY,
                   pOCILib->err),

	xsink
    )

    OCI_RESULT(pOCILib, res);

    return res;
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetID
 * --------------------------------------------------------------------------------------------- */
/*
boolean OCI_API OCI_MsgGetID
(
    OCI_Library *pOCILib,
    OCI_Msg      *msg,
    void         *id,
    unsigned int *len
)
{
    boolean res   = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG,  msg, FALSE);
    OCI_CHECK_PTR(pOCILib, OCI_IPC_VOID, id,  FALSE);
    OCI_CHECK_PTR(pOCILib, OCI_IPC_VOID, len, FALSE);

    if (msg->id != NULL)
    {
        ub4 raw_len = 0;

        raw_len = OCIRawSize(pOCILib->env, msg->id);

        if (*len > raw_len)
        {
            *len = raw_len;
        }

        memcpy(id, OCIRawPtr(pOCILib->env, msg->id), (size_t) (*len));
    }
    else
    {
        *len = 0;
    }

    OCI_RESULT(pOCILib, res);

    return res;
}
*/
/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetOriginalID
 * --------------------------------------------------------------------------------------------- */
/*
boolean OCI_API OCI_MsgGetOriginalID
(
    OCI_Library *pOCILib,
    OCI_Msg      *msg,
    void         *id,
    unsigned int *len
)
{
    boolean res   = TRUE;
    OCIRaw *value = NULL;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG,  msg, FALSE);
    OCI_CHECK_PTR(pOCILib, OCI_IPC_VOID, id,  FALSE);
    OCI_CHECK_PTR(pOCILib, OCI_IPC_VOID, len, FALSE);

    OCI_CALL2
    (
        pOCILib, res, msg->typinf->con,

        OCIAttrGet((dvoid *) msg->proph,
                   (ub4    ) OCI_DTYPE_AQMSG_PROPERTIES,
                   (dvoid *) &value,
                   (ub4   *) NULL,
                   (ub4    ) OCI_ATTR_ORIGINAL_MSGID,
                   pOCILib->err)
    )

    if (value != NULL)
    {
        ub4 raw_len = 0;

        raw_len = OCIRawSize(pOCILib->env, value);

        if (*len > raw_len)
        {
            *len = raw_len;
        }

        memcpy(id, OCIRawPtr(pOCILib->env, value), (size_t) (*len));
    }
    else
    {
        *len = 0;
    }

    OCI_RESULT(pOCILib, res);

    return res;
}
*/
/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgSetOriginalID
 * --------------------------------------------------------------------------------------------- */
boolean OCI_API OCI_MsgSetOriginalID
(
    OCI_Library *pOCILib,
    OCI_Msg      *msg,
    const void   *id,
    unsigned int len,
    ExceptionSink *xsink
)
{
    boolean res   = TRUE;
    OCIRaw *value = NULL;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG,  msg, FALSE);

    OCI_CALL2Q
    (
        pOCILib, res, msg->typinf->con,

        OCIRawAssignBytes(pOCILib->env, pOCILib->err,
                          (ub1*) id, (ub4) len, (OCIRaw **) &value),

	xsink
    )

    OCI_CALL2Q
    (
        pOCILib, res, msg->typinf->con,

        OCIAttrSet((dvoid *) msg->proph,
                   (ub4    ) OCI_DTYPE_AQMSG_PROPERTIES,
                   (dvoid *) &value,
                   (ub4    ) 0,
                   (ub4    ) OCI_ATTR_ORIGINAL_MSGID,
                   pOCILib->err),

	xsink
    )

    OCI_RESULT(pOCILib, res);

    return res;
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetCorrelation
 * --------------------------------------------------------------------------------------------- */
/*
const mtext * OCI_API OCI_MsgGetCorrelation
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, NULL);

    if (msg->correlation == NULL)
    {
        res = OCI_StringGetFromAttrHandle(pOCILib, msg->typinf->con,
                                          msg->proph,
                                          OCI_DTYPE_AQMSG_PROPERTIES,
                                          OCI_ATTR_CORRELATION,
                                          &msg->correlation);
    }

    OCI_RESULT(pOCILib, res);

    return msg->correlation;
}
*/
/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgSetCorrelation
 * --------------------------------------------------------------------------------------------- */
boolean OCI_API OCI_MsgSetCorrelation
(
    OCI_Library *pOCILib,
    OCI_Msg     *msg,
    const mtext *correlation,
    ExceptionSink* xsink
)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, FALSE);

    res =  OCI_StringSetToAttrHandle(pOCILib, msg->typinf->con,
                                     msg->proph,
                                     OCI_DTYPE_AQMSG_PROPERTIES,
                                     OCI_ATTR_CORRELATION,
                                     &msg->correlation,
                                     correlation, xsink);

    OCI_RESULT(pOCILib, res);

    return res;
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetExceptionQueue
 * --------------------------------------------------------------------------------------------- */
/*
const mtext * OCI_API OCI_MsgGetExceptionQueue
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, NULL);

    if (msg->except_queue == NULL)
    {
        res = OCI_StringGetFromAttrHandle(pOCILib, msg->typinf->con,
                                          msg->proph,
                                          OCI_DTYPE_AQMSG_PROPERTIES,
                                          OCI_ATTR_EXCEPTION_QUEUE,
                                          &msg->except_queue);
    }

    OCI_RESULT(pOCILib, res);

    return msg->except_queue;
}
*/
/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgSetExceptionQueue
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgSetExceptionQueue
(
    OCI_Library *pOCILib,
    OCI_Msg     *msg,
    const mtext *queue,
    ExceptionSink *xsink
)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, FALSE);

    res =  OCI_StringSetToAttrHandle(pOCILib, msg->typinf->con,
                                     msg->proph,
                                     OCI_DTYPE_AQMSG_PROPERTIES,
                                     OCI_ATTR_EXCEPTION_QUEUE,
                                     &msg->except_queue,
                                     queue, xsink);

    OCI_RESULT(pOCILib, res);

    return res;
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetSender
 * --------------------------------------------------------------------------------------------- */
/*
OCI_Agent * OCI_API OCI_MsgGetSender
(
    OCI_Library *pOCILib,
    OCI_Msg   *msg
)
{
    boolean res = TRUE;
    OCIAQAgent *handle = NULL;
    OCI_Agent  *sender = NULL;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, NULL);

    OCI_CALL2
    (
        pOCILib, res, msg->typinf->con,

        OCIAttrGet((dvoid *) msg->proph,
                   (ub4    ) OCI_DTYPE_AQMSG_PROPERTIES,
                   (dvoid *) &handle,
                   (ub4   *) 0,
                   (ub4    ) OCI_ATTR_SENDER_ID,
                   pOCILib->err)
    )

    if ((res == TRUE) && (handle != NULL))
    {
        sender = OCI_AgentInit(pOCILib, msg->typinf->con, &msg->sender, handle, NULL, NULL);
    }

    OCI_RESULT(pOCILib, res);

    return sender;
}
*/
/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgSetSender
 * --------------------------------------------------------------------------------------------- */
boolean OCI_API OCI_MsgSetSender
(
    OCI_Library *pOCILib,
    OCI_Msg   *msg,
    OCI_Agent *sender,
    ExceptionSink *xsink
)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, FALSE);

    OCI_CALL2Q
    (
        pOCILib, res, msg->typinf->con,

        OCIAttrSet((dvoid *) msg->proph,
                   (ub4    ) OCI_DTYPE_AQMSG_PROPERTIES,
                   (dvoid *) (sender ? sender->handle : NULL),
                   (ub4    ) 0,
                   (ub4    ) OCI_ATTR_SENDER_ID,
                   pOCILib->err),

	xsink
    )

    OCI_RESULT(pOCILib, res);

    return res;
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgSetConsumers
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgSetConsumers
(
    OCI_Library *pOCILib,
    OCI_Msg     *msg,
    OCI_Agent  **consumers,
    unsigned int count,
    ExceptionSink *xsink
)
{
    boolean res          = TRUE;
    OCIAQAgent **handles = NULL;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MSG, msg, FALSE);

    // allocate local array of OCIAQAgent handles if needed

    if ((consumers != NULL) && (count > 0))
    {
        handles = (OCIAQAgent **) OCI_MemAlloc2(pOCILib, OCI_IPC_ARRAY,sizeof(OCIAQAgent *), count, FALSE);

        if (handles != NULL)
        {
            unsigned int i;

            for(i = 0; i < count; i++)
            {
                handles[i] = consumers[i]->handle;
            }
        }
    }
    else
    {
        count = 0;
    }

    OCI_CALL2Q
    (
        pOCILib, res, msg->typinf->con,

        OCIAttrSet((dvoid *) msg->proph,
                   (ub4    ) OCI_DTYPE_AQMSG_PROPERTIES,
                   (dvoid *) handles,
                   (ub4    ) count,
                   (ub4    ) OCI_ATTR_RECIPIENT_LIST,
                   pOCILib->err),

	xsink
    )


    // free local array of OCIAQAgent handles if needed

    if (handles != NULL)
    {
        OCI_FREE(handles);
    }

    OCI_RESULT(pOCILib, res);

    return res;
}
