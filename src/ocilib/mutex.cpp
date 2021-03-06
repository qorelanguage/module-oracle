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
 * $Id: mutex.c, v 3.7.0 2010-07-26 21:10 Vincent Rogier $
 * ------------------------------------------------------------------------ */

#include "ocilib_internal.h"

/* ************************************************************************ *
 *                            PRIVATE FUNCTIONS
 * ************************************************************************ */

/* ------------------------------------------------------------------------ *
 * OCI_MutexCreateInternal
 * ------------------------------------------------------------------------ */

OCI_Mutex * OCI_MutexCreateInternal(OCI_Library * pOCILib, ExceptionSink* xsink)
{
    OCI_Mutex *mutex = NULL;
    boolean res      = TRUE;

    /* allocate mutex structure */

    mutex = (OCI_Mutex *) OCI_MemAlloc2(pOCILib, OCI_IPC_MUTEX, sizeof(*mutex), 
                                       (size_t) 1, TRUE);

    if (mutex != NULL)
    {
        /* allocate error handle */

       res = (OCI_SUCCESS == OCI_HandleAlloc2(pOCILib, pOCILib->env, 
                                              (dvoid **) (void *) &mutex->err,
                                              OCI_HTYPE_ERROR, (size_t) 0,
                                              (dvoid **) NULL));

        /* allocate mutex handle */

        OCI_CALL3Q
        (
            pOCILib, res, mutex->err,
            
            OCIThreadMutexInit(pOCILib->env, mutex->err, &mutex->handle),

	    xsink
        )
    }
    else
        res = FALSE;

    if (res == FALSE)
    {
        OCI_MutexFree(pOCILib, mutex);
        mutex = NULL;
    }

    return mutex;
}

/* ************************************************************************ *
 *                            PUBLIC FUNCTIONS
 * ************************************************************************ */

/* ------------------------------------------------------------------------ *
 * OCI_MutexCreate
 * ------------------------------------------------------------------------ */

OCI_Mutex * OCI_API OCI_MutexCreate(OCI_Library * pOCILib, ExceptionSink* xsink)
{
    OCI_Mutex *mutex = NULL;

    OCI_CHECK_INITIALIZED2(pOCILib, NULL);

    mutex = OCI_MutexCreateInternal(pOCILib);

    OCI_RESULT(pOCILib, mutex != NULL);

    return mutex;
}

/* ------------------------------------------------------------------------ *
 * OCI_MutexFree
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_MutexFree(OCI_Library * pOCILib, OCI_Mutex *mutex, ExceptionSink* xsink)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MUTEX, mutex, FALSE);

    /* close mutex handle */

    if (mutex->handle != NULL)
    {
        OCI_CALL0Q
        (
            pOCILib, res, mutex->err, 
            
            OCIThreadMutexDestroy(pOCILib->env, mutex->err, &mutex->handle),

	    xsink
        )
    }

    /* close error handle */

    if (mutex->err != NULL)
    {
       OCI_HandleFree2(pOCILib, mutex->err, OCI_HTYPE_ERROR);
    }

    /* free mutex structure */

    OCI_FREE(mutex);

    OCI_RESULT(pOCILib, res);

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_MutexAcquire
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_MutexAcquire(OCI_Library * pOCILib, OCI_Mutex *mutex, ExceptionSink* xsink)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MUTEX, mutex, FALSE);

    OCI_CALL3Q
    (
        pOCILib, res, mutex->err, 
        
        OCIThreadMutexAcquire(pOCILib->env, mutex->err, mutex->handle),

	xsink
    )
    
    OCI_RESULT(pOCILib, res);

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_MutexRelease
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_MutexRelease(OCI_Library * pOCILib, OCI_Mutex *mutex, ExceptionSink* xsink)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_MUTEX, mutex, FALSE);

    OCI_CALL3Q
    (
        pOCILib, res, mutex->err, 
        
        OCIThreadMutexRelease(pOCILib->env, mutex->err, mutex->handle),

	xsink
    )
  
    OCI_RESULT(pOCILib, res);

    return TRUE;
}
