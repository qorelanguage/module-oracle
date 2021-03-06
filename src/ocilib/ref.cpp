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
 * $Id: ref.c, v 3.7.0 2010-07-26 21:10 Vincent Rogier $
 * ------------------------------------------------------------------------ */

#include "ocilib_internal.h"

/* ************************************************************************ *
 *                             PRIVATE FUNCTIONS
 * ************************************************************************ */

/* ------------------------------------------------------------------------ *
 * OCI_RefInit
 * ------------------------------------------------------------------------ */

OCI_Ref * OCI_RefInit(OCI_Library *pOCILib, OCI_Connection *con, OCI_TypeInfo *typinf, OCI_Ref **pref,
                      void *handle)
{
    boolean res   = TRUE;
    OCI_Ref * ref = NULL;

    OCI_CHECK(pref == NULL, NULL);

    if (*pref == NULL)
       *pref = (OCI_Ref *) OCI_MemAlloc2(pOCILib, OCI_IPC_REF, sizeof(*ref), (size_t) 1, TRUE);

    if (*pref != NULL)
    {
        ref = *pref;

        ref->handle = (OCIRef*)handle;
        ref->con    = con;
        ref->typinf = typinf;

        if ((ref->handle == NULL) || (ref->hstate == OCI_OBJECT_ALLOCATED_ARRAY))
        {
            /* allocates handle for non fetched object */

            if (ref->hstate != OCI_OBJECT_ALLOCATED_ARRAY)
            {
                ref->hstate = OCI_OBJECT_ALLOCATED;
            }

            OCI_CALL2
            (
	       pOCILib, res, ref->con,

	       OCI_ObjectNew2(pOCILib, pOCILib->env,  con->err, con->cxt,
                              (OCITypeCode) SQLT_REF,
                              (OCIType*) NULL,
                              (dvoid *) NULL,
                              (OCIDuration) OCI_DURATION_SESSION,
                              (boolean) FALSE,
                              (dvoid **) &ref->handle)
           )
        }
        else
        {
            ref->hstate = OCI_OBJECT_FETCHED_CLEAN;

            OCI_RefUnpin(pOCILib, ref);
        }
    }
    else
        res = FALSE;

    /* check for failure */

    if (res == FALSE)
    {
       OCI_RefFree(pOCILib, ref);
        ref = NULL;
    }

    return ref;
}

/* ------------------------------------------------------------------------ *
 * OCI_RefPin
 * ------------------------------------------------------------------------ */

boolean OCI_RefPin(OCI_Library *pOCILib, OCI_Ref *ref, ExceptionSink* xsink)
{
    boolean res      = TRUE;
    void *obj_handle = NULL;

    OCI_CHECK(ref == NULL, FALSE);

    OCI_RefUnpin(pOCILib, ref);

    OCI_CALL2Q
    (
        pOCILib, res, ref->con,

        OCIObjectPin(pOCILib->env, ref->con->err, ref->handle,
                     (OCIComplexObject *) 0, OCI_PIN_ANY, OCI_DURATION_SESSION,
                     OCI_LOCK_NONE, &obj_handle),

	xsink
    )

    if (res == TRUE)
    {
        OCI_Object *obj = NULL;

        if (res == TRUE)
        {
            obj =  OCI_ObjectInit(pOCILib, ref->con, (OCI_Object **) &ref->obj,
                                  obj_handle, ref->typinf, NULL, -1, TRUE, xsink);
        }

        if (obj != NULL)
            ref->pinned = TRUE;
        else
            res = FALSE;
    }

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_RefUnpin
 * ------------------------------------------------------------------------ */

boolean OCI_RefUnpin(OCI_Library *pOCILib, OCI_Ref *ref)
{
    boolean res = TRUE;

    OCI_CHECK(ref == NULL, FALSE);

    if (ref->pinned == TRUE)
    {
        OCI_CALL2
        (
	   pOCILib, res, ref->con,

	   OCIObjectUnpin(pOCILib->env, ref->con->err, ref->obj->handle)
        )

        ref->pinned = FALSE;
    }

    if (ref->obj != NULL)
    {
        ref->obj->hstate = OCI_OBJECT_FETCHED_DIRTY;
        OCI_ObjectFree2(pOCILib, ref->obj);
        ref->obj = NULL;
    }

    return res;
}

/* ************************************************************************ *
 *                            PUBLIC FUNCTIONS
 * ************************************************************************ */

/* ------------------------------------------------------------------------ *
 * OCI_RefCreate
 * ------------------------------------------------------------------------ */

OCI_Ref * OCI_API OCI_RefCreate(OCI_Library *pOCILib, OCI_Connection *con, OCI_TypeInfo *typinf)
{
    OCI_Ref *ref = NULL;

    OCI_CHECK_INITIALIZED2(pOCILib, NULL);

    OCI_CHECK_PTR(pOCILib, OCI_IPC_CONNECTION, con, NULL);
    OCI_CHECK_PTR(pOCILib, OCI_IPC_TYPE_INFO, typinf, NULL);

    ref = OCI_RefInit(pOCILib, con, typinf, &ref, NULL);

    OCI_RESULT(pOCILib, ref != NULL);

    return ref;
}

/* ------------------------------------------------------------------------ *
 * OCI_RefFree
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_RefFree(OCI_Library *pOCILib, OCI_Ref *ref)
{
    OCI_CHECK_PTR(pOCILib, OCI_IPC_REF, ref, FALSE);

    OCI_CHECK_OBJECT_FETCHED(ref, FALSE);

    OCI_RefUnpin(pOCILib, ref);

    if ((ref->hstate == OCI_OBJECT_ALLOCATED      ) ||
        (ref->hstate == OCI_OBJECT_ALLOCATED_ARRAY))
    {
       OCI_OCIObjectFree2(pOCILib, pOCILib->env, ref->con->err,  ref->handle,
                          OCI_OBJECTFREE_NONULL);
    }

    if (ref->hstate != OCI_OBJECT_ALLOCATED_ARRAY)
    {
        OCI_FREE(ref);
    }

    OCI_RESULT(pOCILib, TRUE);

    return TRUE;
}

/* ------------------------------------------------------------------------ *
 * OCI_RefArrayCreate
 * ------------------------------------------------------------------------ */
/*
OCI_Ref ** OCI_API OCI_RefArrayCreate(OCI_Connection *con,
                                      OCI_TypeInfo *typinf,
                                      unsigned int nbelem)
{
    OCI_Array   *arr  = NULL;
    OCI_Ref    **refs = NULL;

    arr = OCI_ArrayCreate(con, nbelem, OCI_CDT_REF, 0, 
                          sizeof(OCIRef *), sizeof(OCI_Ref), 
                          0, typinf);

    if (arr != NULL)
    {
        refs = (OCI_Ref **) arr->tab_obj;
    }

    return refs;
}
*/

/* ------------------------------------------------------------------------ *
 * OCI_RefArrayFree
 * ------------------------------------------------------------------------ */
 /*
boolean OCI_API OCI_RefArrayFree(OCI_Ref **refs)
{
    return OCI_ArrayFreeFromHandles((void **) refs);
}
 */
/* ------------------------------------------------------------------------ *
 * OCI_RefGetObject
 * ------------------------------------------------------------------------ */

OCI_Object * OCI_API OCI_RefGetObject(OCI_Library *pOCILib, OCI_Ref *ref, ExceptionSink* xsink)
{
    OCI_Object *obj = NULL;

    if (OCI_RefIsNull(pOCILib, ref) == FALSE)
    {
        boolean res = TRUE;

        res = OCI_RefPin(pOCILib, ref, xsink);

        OCI_RESULT(pOCILib, res);

        obj = ref->obj;
    }

	return obj;
}

/* ------------------------------------------------------------------------ *
 * OCI_RefAssign
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_RefAssign(OCI_Library *pOCILib, OCI_Ref *ref, OCI_Ref *ref_src)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_REF, ref,     FALSE);
    OCI_CHECK_PTR(pOCILib, OCI_IPC_REF, ref_src, FALSE);

    OCI_CHECK_COMPAT(pOCILib, ref->con, ref->typinf == ref_src->typinf, FALSE);

    OCI_CALL2
    (
        pOCILib, res, ref->con,

        OCIRefAssign(pOCILib->env, ref->con->err, ref_src->handle, &ref->handle)
    )

    if (res == TRUE)
    {
        if (ref->obj != NULL)
        {
	   OCI_ObjectFree2(pOCILib, ref->obj);
            ref->obj = NULL;
        }

        ref->typinf = ref_src->typinf;
        ref->pinned = ref_src->pinned;
    }

    OCI_RESULT(pOCILib, res);

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_RefIsNull
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_RefIsNull(OCI_Library *pOCILib, OCI_Ref *ref)
{
    OCI_CHECK_PTR(pOCILib, OCI_IPC_REF, ref, TRUE);

    OCI_RESULT(pOCILib, TRUE);

    return (OCIRefIsNull(pOCILib->env, ref->handle) == TRUE);
}

/* ------------------------------------------------------------------------ *
 * OCI_RefSetNull
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_RefSetNull(OCI_Library *pOCILib, OCI_Ref *ref)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_REF, ref, FALSE);

    res = OCI_RefUnpin(pOCILib, ref);

    if (res == TRUE)
    {
        OCIRefClear(pOCILib->env, ref->handle);

        if (ref->obj != NULL)
        {
	   OCI_ObjectFree2(pOCILib, ref->obj);
            ref->obj = NULL;
        }
    }

    OCI_RESULT(pOCILib, res);

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_RefToText
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_RefToText(OCI_Library *pOCILib, OCI_Ref *ref, unsigned int size, mtext *str)
{
    boolean res = TRUE;
    void *ostr  = NULL;
    int osize   = (int) size * (int) sizeof(mtext);

    OCI_CHECK_PTR(pOCILib, OCI_IPC_REF, ref, FALSE);
    OCI_CHECK_PTR(pOCILib, OCI_IPC_STRING, str, FALSE);

    /* init output buffer in case of OCI failure */

    str[0] = 0;

    ostr = OCI_GetInputMetaString(pOCILib, str, &osize);

    OCI_CALL2
    (
       pOCILib, res, ref->con,

        OCIRefToHex(pOCILib->env, ref->con->err, ref->handle,
                    (OraText *) ostr, (ub4 *) &osize)
    )

    OCI_GetOutputMetaString(ostr, str, &osize);
    OCI_ReleaseMetaString(ostr);

    /* set null string terminator */

    str[osize/ (int) sizeof(mtext)] = 0;

    OCI_RESULT(pOCILib, res);

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_RefGetHexSize
 * ------------------------------------------------------------------------ */

unsigned int OCI_API OCI_RefGetHexSize(OCI_Library *pOCILib, OCI_Ref *ref)
{
    ub4 size = 0;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_REF, ref, 0);

    size = OCIRefHexSize(pOCILib->env, (const OCIRef *) ref->handle);

    size /= (ub4) sizeof(mtext);

    OCI_RESULT(pOCILib, TRUE);

    return (unsigned int) size;
}

/* ------------------------------------------------------------------------ *
 * OCI_CollRefGetTypeInfo
 * ------------------------------------------------------------------------ */

OCI_TypeInfo * OCI_API OCI_RefGetTypeInfo(OCI_Library *pOCILib, OCI_Ref *ref)
{
    OCI_CHECK_PTR(pOCILib, OCI_IPC_REF, ref, NULL);

    OCI_RESULT(pOCILib, TRUE);

    return ref->typinf;
}


