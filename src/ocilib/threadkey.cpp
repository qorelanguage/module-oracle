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
 * $Id: threadkey.c, v 3.7.0 2010-07-26 21:10 Vincent Rogier $
 * ------------------------------------------------------------------------ */

#include "ocilib_internal.h"

/* ************************************************************************ *
 *                             PRIVATE FUNCTIONS
 * ************************************************************************ */

/* ------------------------------------------------------------------------ *
 * OCI_ThreadKeyCreateInternal
 * ------------------------------------------------------------------------ */

/*
OCI_ThreadKey * OCI_ThreadKeyCreateInternal(OCI_Library *pOCILib, POCI_THREADKEYDEST destfunc)
{
    boolean  res       = TRUE;
    OCI_ThreadKey *key = NULL;

    // allocate key structure

    key = (OCI_ThreadKey *) OCI_MemAlloc2(pOCILib, OCI_IPC_THREADKEY, sizeof(*key), 
                                         (size_t) 1, TRUE);

    if (key != NULL)
    {
        // allocate error handle

       res = (OCI_SUCCESS == OCI_HandleAlloc2(pOCILib, pOCILib->env, 
                                              (dvoid **) (void *) &key->err,
                                              OCI_HTYPE_ERROR, (size_t) 0,
                                              (dvoid **) NULL));

        // key initialization

        OCI_CALL3
        (
            pOCILib, res, key->err,
            
            OCIThreadKeyInit(pOCILib->env, key->err, &key->handle, destfunc)
        )
    }
    else
        res = FALSE;

    // check errors

    if (res == FALSE)
    {
       OCI_ThreadKeyFree(pOCILib, key);
        key = NULL;
    }

    return key;
}
*/

/* ------------------------------------------------------------------------ *
 * OCI_ThreadKeyFree
 * ------------------------------------------------------------------------ */

boolean OCI_ThreadKeyFree(OCI_Library *pOCILib, OCI_ThreadKey *key)
{
    boolean res = TRUE;

    OCI_CHECK(key == NULL, FALSE);

    /* close key handle */

    if (key->handle != NULL)
    {
        OCI_CALL0
        (
            pOCILib, res, key->err, 
            
            OCIThreadKeyDestroy(pOCILib->env, key->err, &key->handle)
        )
    }

    /* close error handle */

    if (key->err != NULL)
    {
       OCI_HandleFree2(pOCILib, key->err, OCI_HTYPE_ERROR);
    }

    /* free key structure */

    OCI_FREE(key);

    OCI_RESULT(pOCILib, res);

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_ThreadKeySet
 * ------------------------------------------------------------------------ */

boolean OCI_ThreadKeySet(OCI_Library *pOCILib, OCI_ThreadKey *key, void *value)
{
    boolean res = TRUE;

    OCI_CHECK(key == NULL, FALSE);

    OCI_CALL3
    (
        pOCILib, res, key->err, 
        
        OCIThreadKeySet(pOCILib->env, key->err, key->handle, value)
     )

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_ThreadKeyGet
 * ------------------------------------------------------------------------ */

boolean OCI_ThreadKeyGet(OCI_Library *pOCILib, OCI_ThreadKey* key, void **value)
{
    boolean res  = TRUE;

    OCI_CHECK(key == NULL, FALSE);
 
    OCI_CALL3
    (
        pOCILib, res, key->err, 
        
        OCIThreadKeyGet(pOCILib->env, key->err, key->handle, value)
     )
  
    return res;
}

