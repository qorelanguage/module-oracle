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
 * $Id: collection.c, v 3.7.0 2010-07-20 17:45 Vincent Rogier $
 * ------------------------------------------------------------------------ */

#include "ocilib_internal.h"

/* ************************************************************************ *
 *                             PRIVATE FUNCTIONS
 * ************************************************************************ */

/* ------------------------------------------------------------------------ *
 * OCI_CollInit
 * ------------------------------------------------------------------------ */

OCI_Coll * OCI_CollInit(OCI_Library *pOCILib, OCI_Connection *con, OCI_Coll **pcoll, void *handle,
                        OCI_TypeInfo *typinf)
{
    OCI_Coll *coll = NULL;
    boolean res    = TRUE;

    OCI_CHECK(pcoll == NULL, NULL);

    if (*pcoll == NULL)
        *pcoll = (OCI_Coll *) OCI_MemAlloc2(pOCILib, OCI_IPC_COLLECTION, sizeof(*coll),
                                           (size_t) 1, TRUE);

    if (*pcoll != NULL)
    {
        coll = *pcoll;

        coll->con    = con;
        coll->handle = handle;
        coll->typinf = typinf;

        if ((coll->handle == NULL) || (coll->hstate == OCI_OBJECT_ALLOCATED_ARRAY))
        {
            /* allocates handle for non fetched collection */

            if (coll->hstate != OCI_OBJECT_ALLOCATED_ARRAY)
            {
                coll->hstate = OCI_OBJECT_ALLOCATED;
            }

            OCI_CALL2
            (
                pOCILib, res, con,

                OCI_ObjectNew2(pOCILib, pOCILib->env, con->err, con->cxt, typinf->ccode,
                              typinf->tdo, (void *) NULL, OCI_DURATION_SESSION,
                              TRUE, (dvoid **) &coll->handle)
            )
            
            OCI_CALL2
            (
                pOCILib, res, con,

                OCIObjectGetInd(pOCILib->env, con->err,
                                (dvoid *) coll->handle,
                                (dvoid **) &coll->tab_ind)
            )

        }
        else
            coll->hstate = OCI_OBJECT_FETCHED_CLEAN;
    }
    else
        res = FALSE;

    /* check for failure */

    if (res == FALSE)
    {
        OCI_CollFree2(pOCILib, coll);
        coll = NULL;
    }

    return coll;
}

/* ************************************************************************ *
 *                             PUBLIC FUNCTIONS
 * ************************************************************************ */

/* ------------------------------------------------------------------------ *
 * OCI_CollCreate
 * ------------------------------------------------------------------------ */
/*
OCI_Coll * OCI_API OCI_CollCreate(OCI_TypeInfo *typinf)
{
    return OCI_CollCreate2(&OCILib, typinf);
}
*/

OCI_Coll * OCI_API OCI_CollCreate2(OCI_Library *pOCILib, OCI_TypeInfo *typinf)
{
    OCI_Coll *coll = NULL;

    OCI_CHECK_INITIALIZED2(pOCILib, NULL);

    OCI_CHECK_PTR(pOCILib, OCI_IPC_TYPE_INFO, typinf, NULL);
    OCI_CHECK(typinf->ccode == OCI_UNKNOWN, NULL)

    coll = OCI_CollInit(pOCILib, typinf->con, &coll, (OCIColl *) NULL, typinf);

    OCI_RESULT(pOCILib, coll != NULL);

    return coll;
}

/* ------------------------------------------------------------------------ *
 * OCI_CollFree
 * ------------------------------------------------------------------------ */
/*
boolean OCI_API OCI_CollFree(OCI_Coll *coll)
{
    return OCI_CollFree2(&OCILib, coll);
}
*/

boolean OCI_API OCI_CollFree2(OCI_Library *pOCILib, OCI_Coll *coll)
{
    OCI_CHECK_PTR(pOCILib, OCI_IPC_COLLECTION, coll, FALSE);
    OCI_CHECK_OBJECT_FETCHED(coll, FALSE);

    /* free data element accessor */

    if (coll->elem != NULL)
    {
        coll->elem->hstate = OCI_OBJECT_FETCHED_DIRTY;
        OCI_ElemFree2(pOCILib, coll->elem);
        coll->elem = NULL;
    }

    /* free collection for local object */

    if ((coll->hstate == OCI_OBJECT_ALLOCATED      ) ||
        (coll->hstate == OCI_OBJECT_ALLOCATED_ARRAY))
    {
       OCI_OCIObjectFree2(pOCILib, pOCILib->env, coll->typinf->con->err,
                          coll->handle, OCI_OBJECTFREE_NONULL);
    }

    if (coll->hstate != OCI_OBJECT_ALLOCATED_ARRAY)
    {
        OCI_FREE(coll);
    }

    OCI_RESULT(pOCILib, TRUE);

    return TRUE;
}

/* ------------------------------------------------------------------------ *
 * OCI_CollGetSize
 * ------------------------------------------------------------------------ */
/*
unsigned int OCI_API OCI_CollGetSize(OCI_Coll *coll)
{
    return OCI_CollGetSize2(&OCILib, coll);
}
*/

unsigned int OCI_API OCI_CollGetSize2(OCI_Library *pOCILib, OCI_Coll *coll)
{
    boolean res = TRUE;
    sb4 size    = 0;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_COLLECTION, coll, 0);

    OCI_CALL2
    (
        pOCILib, res, coll->con,

        OCICollSize(pOCILib->env, coll->con->err, coll->handle, &size)
    )

    OCI_RESULT(pOCILib, res);

    return (unsigned int) size;
}

/* ------------------------------------------------------------------------ *
 * OCI_CollGetAt
 * ------------------------------------------------------------------------ */
/*
OCI_Elem * OCI_API OCI_CollGetAt(OCI_Coll *coll, unsigned int index)
{
    return OCI_CollGetAt2(&OCILib, coll, index);
}
*/

OCI_Elem * OCI_API OCI_CollGetAt2(OCI_Library *pOCILib, OCI_Coll *coll, unsigned int index)
{
    boolean res    = TRUE;
    boolean exists = FALSE;
    void *data     = NULL;
    OCIInd *p_ind  = NULL;
    OCI_Elem *elem = NULL;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_COLLECTION, coll, NULL);

    OCI_CALL2
    (
        pOCILib, res, coll->con,

        OCICollGetElem(pOCILib->env, coll->con->err, coll->handle, (sb4) index-1,
                       &exists, &data, (dvoid **) (dvoid *) &p_ind)
    )

    if (res == TRUE && exists == TRUE && data != NULL)
    {
        elem = coll->elem = OCI_ElemInit2(pOCILib, coll->con, &coll->elem,
                                         data, p_ind, coll->typinf);
    }

    OCI_RESULT(pOCILib, res);

    return elem;
}


/* ------------------------------------------------------------------------ *
 * OCI_CollAppend
 * ------------------------------------------------------------------------ */
/*
boolean OCI_API OCI_CollAppend(OCI_Coll *coll, OCI_Elem *elem)
{
    return OCI_CollAppend2(&OCILib, coll, elem);
}
*/

boolean OCI_API OCI_CollAppend2(OCI_Library *pOCILib, OCI_Coll *coll, OCI_Elem *elem)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_COLLECTION, coll, FALSE);
    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, FALSE);

    OCI_CHECK_COMPAT(pOCILib, coll->con, elem->typinf->cols[0].type == coll->typinf->cols[0].type, FALSE);

    OCI_CALL2
    (
       pOCILib, res, coll->con,

        OCICollAppend(pOCILib->env, coll->con->err, elem->handle, elem->pind,
                      coll->handle)
    )

    OCI_RESULT(pOCILib, res);

    return res;
}
