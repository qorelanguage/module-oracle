/*
   +----------------------------------------------------------------------+
   |                                                                      |
   |                     OCILIB - C Driver for Oracle                     |
   |                                                                      |
   |                      (C Wrapper for Oracle OCI)                      |
   |                                                                      |
   +----------------------------------------------------------------------+
   |                      Website : http://www.ocilib.net                 |
   +----------------------------------------------------------------------+
   |               Copyright (c) 2007-2010 Vincent ROGIER                 |
   +----------------------------------------------------------------------+
   | This library is free software; you can redistribute it and/or        |
   | modify it under the terms of the GNU Lesser General Public           |
   | License as published by the Free Software Foundation; either         |
   | version 2 of the License, or (at your option) any later version.     |
   |                                                                      |
   | This library is distributed in the hope that it will be useful,      |
   | but WITHOUT ANY WARRANTY; without even the implied warranty of       |
   | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    |
   | Lesser General Public License for more details.                      |
   |                                                                      |
   | You should have received a copy of the GNU Lesser General Public     |
   | License along with this library; if not, write to the Free           |
   | Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.   |
   +----------------------------------------------------------------------+
   |          Author: Vincent ROGIER <vince.rogier@ocilib.net>            |
   +----------------------------------------------------------------------+
*/

/* ------------------------------------------------------------------------ *
 * $Id: ocilib_internal.h, v 3.7.0 2010-07-20 17:45 Vincent Rogier $
 * ------------------------------------------------------------------------ */

#ifndef OCILIB_OCILIB_INTERNAL_H_INCLUDED
#define OCILIB_OCILIB_INTERNAL_H_INCLUDED

#include "ocilib_types.h"
#include "ocilib_checks.h"

#ifdef __cplusplus
//extern "C" {
#endif

/* ************************************************************************ *
                         PRIVATE FUNCTIONS PROTOTYPES
 * ************************************************************************ */

boolean OCI_API OCI_SetTrace
(
    OCI_Library *pOCILib,
    OCI_Connection *con,
    unsigned int    trace,
    const mtext    *value
);

boolean OCI_API OCI_Ping(OCI_Library *pOCILib, OCI_Connection *con, ExceptionSink* xsink);


/* --------------------------------------------------------------------------------------------- *
 * agent.c
 * --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- *
 * agent.c
 * --------------------------------------------------------------------------------------------- */

OCI_Agent * OCI_AgentInit
(
    OCI_Library *pOCILib,
    OCI_Connection *con,
    OCI_Agent     **pagent,
    OCIAQAgent     *handle,
    const mtext    *name,
    const mtext    *address
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_AgentCreate
 * --------------------------------------------------------------------------------------------- */

OCI_Agent * OCI_API OCI_AgentCreate
(
    OCI_Library *pOCILib,
    OCI_Connection *con,
    const mtext    *name,
    const mtext    *address
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_AgentFree
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_AgentFree
(
    OCI_Library *pOCILib,
    OCI_Agent *agent
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_AgentGetName
 * --------------------------------------------------------------------------------------------- */

const mtext * OCI_API OCI_AgentGetName
(
    OCI_Library *pOCILib,
    OCI_Agent *agent
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_AgentSetName
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_AgentSetName
(
    OCI_Library *pOCILib,
    OCI_Agent   *agent,
    const mtext *name
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_AgentGetAddress
 * --------------------------------------------------------------------------------------------- */

const mtext * OCI_API OCI_AgentGetAddress
(
    OCI_Library *pOCILib,
    OCI_Agent *agent
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_AgentSetAddress
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_AgentSetAddress
(
    OCI_Library *pOCILib,
    OCI_Agent   *agent,
    const mtext *address
);

/* --------------------------------------------------------------------------------------------- *
 * callback.c
 * --------------------------------------------------------------------------------------------- */

ub4 OCI_ProcNotifyMessages
(
    void            *ctx,
    OCISubscription *subscrhp,
    void            *payload,
    ub4              paylen,
    void            *desc,
    ub4              mode
);


/* ------------------------------------------------------------------------ *
 * collection.c
 * ------------------------------------------------------------------------ */

OCI_Coll * OCI_CollInit
(
    OCI_Library *pOCILib,
    OCI_Connection *con,
    OCI_Coll **pcoll,
    void *handle,
    OCI_TypeInfo *typeinf
);

boolean OCI_CollAssign(OCI_Library *pOCILib, OCI_Coll *coll, OCI_Coll *coll_src);

/* ------------------------------------------------------------------------ *
 * column.c
 * ------------------------------------------------------------------------ */

boolean OCI_ColumnMap
(
    OCI_Column *col,
    OCI_Statement *stmt
);

boolean OCI_ColumnDescribe
(
    OCI_Column *col,
    OCI_Connection *con,
    OCI_Statement *stmt,
    void *handle,
    int index,
    int ptype
);


/* ------------------------------------------------------------------------ *
 * date.c
 * ------------------------------------------------------------------------ */
/*
OCI_Date * OCI_DateInit
(
    OCI_Connection *con,
    OCI_Date **pdate,
    OCIDate *buffer,
    boolean allocate,
    boolean ansi
);
*/

/* dequeue.c */
/* --------------------------------------------------------------------------------------------- *
 * OCI_DequeueCreate
 * --------------------------------------------------------------------------------------------- */

OCI_Dequeue * OCI_API OCI_DequeueCreate
(
    OCI_Library *pOCILib,
    OCI_TypeInfo *typinf,
    const mtext  *name,
    ExceptionSink* xsink
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_DequeueFree
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_DequeueFree
(
    OCI_Library *pOCILib,
    OCI_Dequeue *dequeue
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_DequeueListen
 * --------------------------------------------------------------------------------------------- */

OCI_Agent * OCI_API OCI_DequeueListen
(
    OCI_Library *pOCILib,
    OCI_Dequeue *dequeue,
    int          timeout
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_DequeueGet
 * --------------------------------------------------------------------------------------------- */

OCI_Msg * OCI_API OCI_DequeueGet
(
    OCI_Library *pOCILib,
    OCI_Dequeue *dequeue,
    ExceptionSink* xsink
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_DequeueGetConsumerName
 * --------------------------------------------------------------------------------------------- */

const mtext * OCI_API OCI_DequeueGetConsumer
(
    OCI_Library *pOCILib,
    OCI_Dequeue *dequeue
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_DequeueSetConsumerName
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_DequeueSetConsumer
(
    OCI_Library *pOCILib,
    OCI_Dequeue *dequeue,
    const mtext *consumer
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_DequeueGetVisibility
 * --------------------------------------------------------------------------------------------- */

unsigned int OCI_API OCI_DequeueGetVisibility
(
    OCI_Library *pOCILib,
    OCI_Dequeue *dequeue
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_DequeueSetVisibility
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_DequeueSetVisibility
(
    OCI_Library *pOCILib,
    OCI_Dequeue *dequeue,
    unsigned int visibility
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_DequeueGetMode
 * --------------------------------------------------------------------------------------------- */

unsigned int OCI_API OCI_DequeueGetMode
(
    OCI_Library *pOCILib,
    OCI_Dequeue *dequeue
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_DequeueSetMode
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_DequeueSetMode
(
    OCI_Library *pOCILib,
    OCI_Dequeue *dequeue,
    unsigned int mode
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_DequeueGetNavigation
 * --------------------------------------------------------------------------------------------- */

unsigned int OCI_API OCI_DequeueGetNavigation
(
    OCI_Library *pOCILib,
    OCI_Dequeue *dequeue
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_DequeueSetNavigation
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_DequeueSetNavigation
(
    OCI_Library *pOCILib,
    OCI_Dequeue *dequeue,
    unsigned int position
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_DequeueGetWaitTime
 * --------------------------------------------------------------------------------------------- */

int OCI_API OCI_DequeueGetWaitTime
(
    OCI_Library *pOCILib,
    OCI_Dequeue *dequeue
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_DequeueSetWaitTime
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_DequeueSetWaitTime
(
    OCI_Library *pOCILib,
    OCI_Dequeue *dequeue,
    int          timeout,
    ExceptionSink* xsink
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_DequeueSetAgentList
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_DequeueSetAgentList
(
    OCI_Library *pOCILib,
    OCI_Dequeue *dequeue,
    OCI_Agent  **consumers,
    unsigned int count
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_DequeueSubscribe
 * --------------------------------------------------------------------------------------------- */

OCI_EXPORT boolean OCI_API  OCI_DequeueSubscribe
(
    OCI_Library *pOCILib,
    OCI_Dequeue    *dequeue,
    unsigned int    port,
    unsigned int    timeout,
    POCI_NOTIFY_AQ  callback,
    ExceptionSink* xsink
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_DequeueUnsubscribe
 * --------------------------------------------------------------------------------------------- */

OCI_EXPORT boolean OCI_API OCI_DequeueUnsubscribe
(
    OCI_Library *pOCILib,
    OCI_Dequeue *dequeue,
    ExceptionSink* xsink
);


/* ------------------------------------------------------------------------ *
 * element.c
 * ------------------------------------------------------------------------ */
/*
boolean OCI_ElemGetNumber
(
    OCI_Elem *elem,
    void *value,
    uword size,
    uword flag
);

boolean OCI_ElemSetNumber
(
    OCI_Elem  *elem,
    void *value,
    uword size,
    uword flag
);

OCI_Elem * OCI_ElemInit
(
    OCI_Connection *con,
    OCI_Elem **pelem,
    void *handle,
    OCIInd *pind,
    OCI_TypeInfo *typeinf
);
*/
boolean OCI_ElemSetNullIndicator
(
    OCI_Elem *elem,
    OCIInd value
);

/* enqueue.c */
/* --------------------------------------------------------------------------------------------- *
 * OCI_EnqueueCreate
 * --------------------------------------------------------------------------------------------- */

OCI_Enqueue * OCI_API OCI_EnqueueCreate
(
    OCI_Library *pOCILib,
    OCI_TypeInfo *typinf,
    const mtext  *name
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_EnqueueFree
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_EnqueueFree
(
    OCI_Library *pOCILib,
    OCI_Enqueue *enqueue
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_EnqueuePut
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_EnqueuePut
(
    OCI_Library *pOCILib,
    OCI_Enqueue *enqueue,
    OCI_Msg     *msg,
    ExceptionSink *xsink
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_EnqueueGetVisibility
 * --------------------------------------------------------------------------------------------- */

unsigned int OCI_API OCI_EnqueueGetVisibility
(
    OCI_Library *pOCILib,
    OCI_Enqueue *enqueue
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_EnqueueSetVisibility
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_EnqueueSetVisibility
(
    OCI_Library *pOCILib,
    OCI_Enqueue *enqueue,
    unsigned int visibility
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_EnqueueGetSequenceDeviation
 * --------------------------------------------------------------------------------------------- */

unsigned int OCI_API OCI_EnqueueGetSequenceDeviation
(
    OCI_Library *pOCILib,
    OCI_Enqueue *enqueue
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_EnqueueSetDeviation
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_EnqueueSetSequenceDeviation
(
    OCI_Library *pOCILib,
    OCI_Enqueue *enqueue,
    unsigned int sequence
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_EnqueueSetRelativeMsgID
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_EnqueueGetRelativeMsgID
(
    OCI_Library *pOCILib,
    OCI_Enqueue  *enqueue,
    void         *id,
    unsigned int *len
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_EnqueueSetRelativeMsgID
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_EnqueueSetRelativeMsgID
(
    OCI_Library *pOCILib,
    OCI_Enqueue  *enqueue,
    const void   *id,
    unsigned int  len
);

/* ------------------------------------------------------------------------ *
 * error.c
 * ------------------------------------------------------------------------ */

void OCI_ErrorFree
(
    OCI_Error *err
);

void OCI_ErrorReset
(
    OCI_Error *err
);

OCI_Error * OCI_ErrorGet
(
    boolean check,
    boolean warning
);

OCI_Error * OCI_ErrorCreate
(
    void
);

/* ------------------------------------------------------------------------ *
 * exception.c
 * ------------------------------------------------------------------------ */
/*
OCI_Error * OCI_ExceptionGetError
(
    boolean warning
);

void OCI_ExceptionRaise
(
    OCI_Error *err
);

void OCI_ExceptionOCI
(
    OCIError *p_err,
    OCI_Connection *con,
    OCI_Statement *stmt,
    boolean warning
);

void OCI_ExceptionMemory
(
    int type,
    size_t nb_bytes,
    OCI_Connection *con,
    OCI_Statement *stmt
);

void OCI_ExceptionNotAvailable
(
    OCI_Connection *con,
    int feature
);

void OCI_ExceptionNullPointer
(
    int type
);

void OCI_ExceptionDatatypeNotSupported
(
    OCI_Connection *con,
    OCI_Statement *stmt,
    int code
);

void OCI_ExceptionParsingToken
(
    OCI_Connection *con,
    OCI_Statement *stmt,
    mtext token
);

void OCI_ExceptionMappingArgument
(
    OCI_Connection *con,
    OCI_Statement *stmt,
    int arg
);

void OCI_ExceptionNotInitialized(void);

void OCI_ExceptionLoadingSharedLib(void);

void OCI_ExceptionLoadingSymbols(void);

void OCI_ExceptionNotMultithreaded(void);

void OCI_ExceptionOutOfBounds
(
    OCI_Connection *con,
    int value
);

void OCI_ExceptionUnfreedData
(
    int type_elem,
    int nb_elem
);

void OCI_ExceptionMaxBind
(
    OCI_Statement *stmt
);

void OCI_ExceptionAttributeNotFound
(
    OCI_Connection *con,
    const mtext *attr
);

void OCI_ExceptionMinimumValue
(
    OCI_Connection *con,
    OCI_Statement *stmt,
    int min
);

void OCI_ExceptionTypeNotCompatible
(
    OCI_Connection *con
);

void OCI_ExceptionStatementState
(
    OCI_Statement *stmt,
    int state
);

void OCI_ExceptionStatementNotScrollable
(
    OCI_Statement *stmt
);

void OCI_ExceptionBindAlreadyUsed
(
    OCI_Statement *stmt,
    const mtext * bind
);

void OCI_ExceptionBindArraySize
(
    OCI_Statement *stmt,
    unsigned int maxsize,
    unsigned int cursize,
    unsigned int newsize
);

void OCI_ExceptionDirPathColNotFound
(
    OCI_DirPath *dp,
    const mtext * column,
    const mtext *table
);

void OCI_ExceptionDirPathState
(
    OCI_DirPath *dp,
    int state
);
*/
void OCI_ExceptionOCIEnvironment2(OCI_Library *pOCILib);



/* ------------------------------------------------------------------------ *
 * hash.c
 * ------------------------------------------------------------------------ */

unsigned int OCI_HashCompute
(
    OCI_HashTable *table,
    const mtext *str
);

boolean OCI_HashAdd
(
    OCI_HashTable *table,
    const mtext *key,
    OCI_Variant value,
    unsigned int type
);

/* ------------------------------------------------------------------------ *
 * interval.c
 * ------------------------------------------------------------------------ */

OCI_Interval  * OCI_IntervalInit
(
    OCI_Library *pOCILib,
    OCI_Connection *con,
    OCI_Interval **pitv,
    OCIInterval *buffer,
    ub4 type
);

/* ------------------------------------------------------------------------ *
 * library.c
 * ------------------------------------------------------------------------ */

void OCI_SetLastError
(
    OCI_Error err
 );

void OCI_SetStatus
(
   OCI_Library *pOCILib,
    boolean res
 );

//boolean OCI_KeyMapFree(void);

/* ------------------------------------------------------------------------ *
 * list.c
 * ------------------------------------------------------------------------ */

OCI_List * OCI_ListCreate
(
    int type
);

boolean OCI_ListFree
(
   OCI_Library *pOCILib,
   OCI_List *list
);

OCI_Item * OCI_ListCreateItem
(
    OCI_Library *pOCILib,
    int type,
    int size
);

OCI_Item * OCI_ListAppend
(
    OCI_Library *pOCILib,
    OCI_List *list,
    int size
);

boolean OCI_ListClear
(
   OCI_Library *pOCILib,
    OCI_List *list
);

boolean OCI_ListForEach
(
   OCI_Library *pOCILib,
    OCI_List *list,
    boolean (*proc)(void *)
);

boolean OCI_ListRemove
(
   OCI_Library *pOCILib,
    OCI_List *list,
    void *data
);

/* ------------------------------------------------------------------------ *
 * lob.c
 * ------------------------------------------------------------------------ */

OCI_Lob * OCI_LobInit
(
    OCI_Library *pOCILib,
    OCI_Connection *con,
    OCI_Lob **plob,
    OCILobLocator *handle,
    ub4 type
);

boolean OCI_LobAssign(OCI_Library *pOCILib, OCI_Lob *lob, OCI_Lob *lob_src);

/* ------------------------------------------------------------------------ *
 * long.c
 * ------------------------------------------------------------------------ */

OCI_Long * OCI_LongInit
(
    OCI_Statement *stmt,
    OCI_Long **plg,
    OCI_Define *def,
    unsigned int type
);

/* ------------------------------------------------------------------------ *
 * memory.c
 * ------------------------------------------------------------------------ */

void * OCI_MemAlloc2
(
   OCI_Library *pOCILib,
    int ptr_type,
    size_t block_size,
    size_t block_count,
    boolean zero_fill
);

void * OCI_MemRealloc2
(
   OCI_Library *pOCILib,
    void * ptr_mem,
    int ptr_type,
    size_t block_size,
    size_t block_count
);

void OCI_MemFree
(
    void * ptr_mem
);

sword OCI_HandleAlloc2
(
   OCI_Library *pOCILib,
    CONST dvoid *parenth,
    dvoid **hndlpp,
    CONST ub4 type,
    CONST size_t xtramem_sz,
    dvoid **usrmempp
);

sword OCI_HandleFree2
(
   OCI_Library *pOCILib,
    dvoid *hndlp,
    CONST ub4 type
);

sword OCI_DescriptorAlloc2
(
   OCI_Library *pOCILib,
    CONST dvoid *parenth,
    dvoid **descpp,
    CONST ub4 type,
    CONST size_t xtramem_sz,
    dvoid **usrmempp
);

sword OCI_DescriptorArrayAlloc2
(
   OCI_Library *pOCILib,
    CONST dvoid *parenth,
    dvoid **descpp,
    CONST ub4 type,
    ub4 nb_elem,
    CONST size_t xtramem_sz,
    dvoid **usrmempp
);

sword OCI_DescriptorFree2
(
   OCI_Library *pOCILib,
    void *descp,
    CONST ub4 type
);

sword OCI_DescriptorArrayFree2
(
   OCI_Library *pOCILib,
    void **descp,
    CONST ub4 type,
    ub4 nb_elem
);


sword OCI_ObjectNew
(
    OCIEnv *env,
    OCIError *err,
    CONST OCISvcCtx *svc,
    OCITypeCode typecode,
    OCIType *tdo,
    dvoid *table,
    OCIDuration duration,
    boolean value,
    dvoid **instance
);
/*
sword OCI_OCIObjectFree
(
    OCIEnv *env,
    OCIError *err,
    dvoid *instance,
    ub2 flags
);
*/

/* ------------------------------------------------------------------------ *
 * msg.c
 * ------------------------------------------------------------------------ *
 */

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgCreate
 * --------------------------------------------------------------------------------------------- */

OCI_Msg * OCI_API OCI_MsgCreate
(
    OCI_Library *pOCILib,
    OCI_TypeInfo *typinf,
    ExceptionSink* xsink
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgFree
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgFree
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgReset
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgReset
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetObject
 * --------------------------------------------------------------------------------------------- */

OCI_Object * OCI_API OCI_MsgGetObject
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetObject
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgSetObject
(
    OCI_Library *pOCILib,
    OCI_Msg    *msg,
    OCI_Object *obj
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetAttemptCount
 * --------------------------------------------------------------------------------------------- */

int OCI_API OCI_MsgGetAttemptCount
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetEnqueueDelay
 * --------------------------------------------------------------------------------------------- */

int OCI_API OCI_MsgGetEnqueueDelay
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgSetEnqueueDelay
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgSetEnqueueDelay
(
    OCI_Library *pOCILib,
    OCI_Msg *msg,
    int      value
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetEnqueueTime
 * --------------------------------------------------------------------------------------------- */

OCI_Date * OCI_API OCI_MsgGetEnqueueTime
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetExpiration
 * --------------------------------------------------------------------------------------------- */

int OCI_API OCI_MsgGetExpiration
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgSetExpiration
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgSetExpiration
(
    OCI_Library *pOCILib,
    OCI_Msg *msg,
    int      value
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetState
 * --------------------------------------------------------------------------------------------- */

unsigned int OCI_API OCI_MsgGetState
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetPriority
 * --------------------------------------------------------------------------------------------- */

int OCI_API OCI_MsgGetPriority
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgSetPriority
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgSetPriority
(
    OCI_Library *pOCILib,
    OCI_Msg *msg,
    int      value
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetID
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgGetID
(
    OCI_Library *pOCILib,
    OCI_Msg      *msg,
    void         *id,
    unsigned int *len
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetOriginalID
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgGetOriginalID
(
    OCI_Library *pOCILib,
    OCI_Msg      *msg,
    void         *id,
    unsigned int *len
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgSetOriginalID
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgSetOriginalID
(
    OCI_Library *pOCILib,
    OCI_Msg      *msg,
    const void   *id,
    unsigned int len
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetCorrelation
 * --------------------------------------------------------------------------------------------- */

const mtext * OCI_API OCI_MsgGetCorrelation
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgSetCorrelation
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgSetCorrelation
(
    OCI_Library *pOCILib,
    OCI_Msg     *msg,
    const mtext *correlation
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetExceptionQueue
 * --------------------------------------------------------------------------------------------- */

const mtext * OCI_API OCI_MsgGetExceptionQueue
(
    OCI_Library *pOCILib,
    OCI_Msg *msg
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgSetExceptionQueue
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgSetExceptionQueue
(
    OCI_Library *pOCILib,
    OCI_Msg     *msg,
    const mtext *queue
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgGetSender
 * --------------------------------------------------------------------------------------------- */

OCI_Agent * OCI_API OCI_MsgGetSender
(
    OCI_Library *pOCILib,
    OCI_Msg   *msg
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgSetSender
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgSetSender
(
    OCI_Library *pOCILib,
    OCI_Msg   *msg,
    OCI_Agent *sender
);

/* --------------------------------------------------------------------------------------------- *
 * OCI_MsgSetConsumers
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_MsgSetConsumers
(
    OCI_Library *pOCILib,
    OCI_Msg     *msg,
    OCI_Agent  **consumers,
    unsigned int count
);

/* ------------------------------------------------------------------------ *
 * number.c
 * ------------------------------------------------------------------------ */
/*
boolean OCI_NumberGet
(
    OCI_Connection *con,
    OCINumber *data,
    void *value,
    uword size,
    uword flag)
;

boolean OCI_NumberSet
(
    OCI_Connection *con,
    OCINumber *data,
    void *value,
    uword size,
    uword flag
);

boolean OCI_NumberConvertStr
(
    OCI_Connection *con,
    OCINumber *num,
    const dtext *str,
    int str_size,
    const mtext* fmt,
    ub4 fmt_size
);

boolean OCI_NumberGetFromStr
    (
    OCI_Connection *con,
    void *value,
    uword size,
    uword type,
    const dtext *str,
    int str_size,
    const mtext* fmt,
    ub4 fmt_size
);
*/
/* ------------------------------------------------------------------------ *
 * object.c
 * ------------------------------------------------------------------------ */

boolean OCI_ObjectGetAttrInfo
(
    OCI_TypeInfo *typinf,
    int index,
    size_t *p_size, 
    int *p_type
);

size_t OCI_ObjectGetStructSize
(
    OCI_TypeInfo *typinf
);

ub2 OCI_ObjectGetIndOffset
(
    OCI_TypeInfo *typinf,
    int index
);

OCI_Object * OCI_ObjectInit
(
    OCI_Library *pOCILib,
    OCI_Connection *con,
    OCI_Object **pobj,
    void *handle,
    OCI_TypeInfo *typinf,
    OCI_Object *parent,
    int index,
    boolean reset,
    ExceptionSink* xsink
);

void OCI_ObjectReset
(
    OCI_Library *pOCILib,
    OCI_Object *obj
);
/*
int OCI_ObjectGetAttrIndex
(
    OCI_Object *obj,
    const mtext *attr,
    int type
);
*/
void * OCI_ObjectGetAttr
(
    OCI_Object *obj,
    unsigned int index,
    OCIInd **pind
);
/*
boolean OCI_ObjectSetNumber
(
    OCI_Object *obj,
    const mtext *attr,
    void *value,
    uword size, uword flag
);

boolean OCI_ObjectGetNumber
(
    OCI_Object *obj,
    const mtext *attr,
    void *value,
    uword size, uword flag
);
*/

/* ------------------------------------------------------------------------ *
 * string.c
 * ------------------------------------------------------------------------ */

int OCI_StringCopy4to2bytes
(
    const unsigned int* src,
    size_t src_size,
    unsigned short* dst,
    size_t dst_size
);

int OCI_StringCopy2to4bytes
(
    const unsigned short* src,
    size_t src_size,
    unsigned int* dst,
    size_t dst_size
);


void * OCI_GetInputString
(
    OCI_Library *pOCILib,
    void *src,
    int *size,
    size_t size_char_in,
    size_t size_char_out
);

void OCI_GetOutputString
(
    void *src,
    void *dest,
    int *size,
    size_t size_char_in,
    size_t size_char_out
);

void OCI_MoveString
(
    void *src,
    void *dst,
    int char_count,
    size_t size_char_in,
    size_t size_char_out
);

void OCI_ConvertString
(
    void *str,
    int char_count,
    size_t size_char_in,
    size_t size_char_out
);

void OCI_CopyString
(
    void *src,
    void *dest,
    int *size,
    size_t size_char_in,
    size_t size_char_out
);

void OCI_ReleaseMetaString
(
    void *ptr
);

void OCI_ReleaseDataString
(
    void *ptr
);

size_t OCI_StringLength
(
    void *ptr,
    size_t size_elem
);

int OCI_StringUTF8Length
(
   const char *str
);


#define OCI_GetInputMetaString(ocilib, s, n)     OCI_GetInputString(ocilib, (void *) s, n, \
                                                             sizeof(mtext),    \
                                                             sizeof(omtext))

#define OCI_GetOutputMetaString(s, d, n) OCI_GetOutputString((void *) s, d, n, \
                                                             sizeof(omtext),   \
                                                             sizeof(mtext))

#define OCI_GetInputDataString(ocilib, s, n)     OCI_GetInputString(ocilib, (void *) s, n, \
                                                             sizeof(dtext),    \
                                                             sizeof(odtext))

#define OCI_GetOutputDataString(s, d, n) OCI_GetOutputString((void *) s, d, n, \
                                                              sizeof(odtext),  \
                                                              sizeof(dtext))

void * OCI_StringFromStringPtr
(
    OCI_Library *pOCILib,
    OCIString *str,
    void ** buf,
    int *buflen
);

boolean OCI_StringToStringPtr
(
    OCI_Library *pOCILib,
    OCIString **str,
    OCIError *err,
    void *value,
    void **buf,
    int *buflen
);

boolean OCI_StringGetFromAttrHandle
(
    OCI_Library *pOCILib,
    OCI_Connection *con,
    void           *handle,
    unsigned int    type,
    unsigned int    attr,
    mtext         **str
);

boolean OCI_StringSetToAttrHandle
(
    OCI_Library *pOCILib,
    OCI_Connection *con,
    void           *handle,
    unsigned int    type,
    unsigned int    attr,
    mtext         **str,
    const mtext    *value
);

/* ------------------------------------------------------------------------ *
 * timestamp.c
 * ------------------------------------------------------------------------ */

OCI_Timestamp * OCI_TimestampInit
(
    OCI_Library *pOCILib,
    OCI_Connection *con,
    OCI_Timestamp **ptmsp,
    OCIDateTime *buffer,
    ub4 type
);

/* ------------------------------------------------------------------------ *
 * typeinf.c
 * ------------------------------------------------------------------------ */

boolean OCI_TypeInfoClose
(
    OCI_TypeInfo *typeinf
);

boolean OCI_ThreadKeyFree(OCI_Library *pOCILib, OCI_ThreadKey *key);
boolean OCI_ThreadKeyGet(OCI_Library *pOCILib, OCI_ThreadKey* key, void **value);
boolean OCI_ThreadKeySet(OCI_Library *pOCILib, OCI_ThreadKey *key, void *value);
OCI_Mutex * OCI_MutexCreateInternal(OCI_Library * pOCILib);
boolean OCI_API OCI_MutexFree(OCI_Library * pOCILib, OCI_Mutex *mutex);
boolean OCI_API OCI_MutexAcquire(OCI_Library * pOCILib, OCI_Mutex *mutex);
boolean OCI_API OCI_MutexRelease(OCI_Library * pOCILib, OCI_Mutex *mutex);

OCI_Ref * OCI_RefInit
(
    OCI_Library *pOCILib,
    OCI_Connection *con,
    OCI_TypeInfo *typeinf,
    OCI_Ref **pref,
    void *handle
);

boolean OCI_RefPin
(
    OCI_Library *pOCILib,
    OCI_Ref *ref,
    ExceptionSink* xsink
);

boolean OCI_RefUnpin
(
    OCI_Library *pOCILib,
    OCI_Ref *ref
);

boolean OCI_FileFree
(
    OCI_Library *pOCILib,
    OCI_File *file
);

boolean OCI_RefFree
(
    OCI_Library *pOCILib,
    OCI_Ref *ref
);                                                                                                                                                                           
boolean OCI_RefIsNull
(
    OCI_Library *pOCILib,
    OCI_Ref *ref
);

const mtext *OCI_GetDefaultFormatNumeric
(
    OCI_Library *pOCILib,
    OCI_Connection *con
);

boolean OCI_SetDefaultFormatNumeric
(
    OCI_Connection *con,
    const mtext *format
);                                                                                                                                                                           
#ifdef  __cplusplus
//}
#endif


#endif    /* OCILIB_COMMON_FUNCTIONS_H_INCLUDED */

