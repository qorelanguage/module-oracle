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
 * $Id: element.c, v 3.7.1 2010-07-30 11:57 Vincent Rogier $
 * ------------------------------------------------------------------------ */

#include "ocilib.h"
#include "ocilib_internal.h"

const char* OCI_GetColumnTypeName(int cdt) {
   switch (cdt) {
      case OCI_CDT_NUMERIC:
	 return "number";
      case OCI_CDT_DATETIME:
	 return "datetime";
      case OCI_CDT_TEXT:
	 return "text";
      case OCI_CDT_LONG:
	 return "long";
      case OCI_CDT_CURSOR:
	 return "cursor";
      case OCI_CDT_LOB:
	 return "LOB";
      case OCI_CDT_FILE:
	 return "file";
      case OCI_CDT_TIMESTAMP:
	 return "timestamp";
      case OCI_CDT_INTERVAL:
	 return "interval";
      case OCI_CDT_RAW:
	 return "raw";
      case OCI_CDT_OBJECT:
	 return "object";
      case OCI_CDT_COLLECTION:
	 return "collection";
      case OCI_CDT_REF:
	 return "ref";
   }
   return "unknown";
}

/* ************************************************************************ *
 *                             PRIVATE FUNCTIONS
 * ************************************************************************ */

/* ------------------------------------------------------------------------ *
 * OCI_ElemInit
 * ------------------------------------------------------------------------ */

OCI_Elem * OCI_ElemInit2(OCI_Library *pOCILib, OCI_Connection *con, OCI_Elem **pelem, void *handle, 
			 OCIInd *pind, OCI_TypeInfo *typinf, ExceptionSink* xsink)
{
    OCI_Elem *elem = NULL;
    boolean res    = TRUE;

    OCI_CHECK(pelem == NULL, NULL);

    if (*pelem == NULL)
        *pelem = (OCI_Elem *) OCI_MemAlloc2(pOCILib, OCI_IPC_ELEMENT, sizeof(*elem),
                                           (size_t) 1, TRUE);

    if (*pelem != NULL)
    {
        elem = *pelem;

        elem->con     = con;
        elem->ind     = OCI_IND_NULL;
        elem->typinf  = typinf;
        elem->handle = handle;

        if (handle == NULL)
            elem->hstate = OCI_OBJECT_ALLOCATED;
        else
            elem->hstate = OCI_OBJECT_FETCHED_CLEAN;

        switch (elem->typinf->cols[0].type)
        {
            case OCI_CDT_NUMERIC:

                if (elem->handle == NULL)
                {
		   elem->handle = (OCINumber *) OCI_MemAlloc2(pOCILib, OCI_IPC_VOID, 
                                                              sizeof(OCINumber),
                                                              1, TRUE);
                }

                break;

            case OCI_CDT_TEXT:
            case OCI_CDT_TIMESTAMP:
            case OCI_CDT_INTERVAL:
            case OCI_CDT_RAW:
            case OCI_CDT_LOB:
            case OCI_CDT_FILE:
            case OCI_CDT_REF:

                if (elem->handle != NULL)
                    elem->handle = * (void **) handle;
                break;
        }

        if (pind != NULL)
        {
            elem->pind = pind;
            elem->ind  = *elem->pind;
        }
        else
        {
            elem->pind = &elem->ind;
        }
    }
    else
        res = FALSE;

    /* check for failure */

    if (res == FALSE)
    {
       OCI_ElemFree2(pOCILib, elem, xsink);
       elem = NULL;
    }

    return elem;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemSetNullIndicator
 * ------------------------------------------------------------------------ */

boolean OCI_ElemSetNullIndicator(OCI_Elem *elem, OCIInd value)
{
    boolean res = TRUE;

    if (elem->typinf->cols[0].type == OCI_CDT_OBJECT)
    {
        OCI_Object *obj = (OCI_Object *) elem->obj;

        if (obj != NULL)
        {
            elem->pind = obj->tab_ind;
        }
    }
    else
    {
        if (elem->pind != NULL)
            *elem->pind  = value;
    }

    return res;
}

boolean OCI_ElemSetNumberFromString(OCI_Library* pOCILib, OCI_Elem* elem, const char* str, int size, ExceptionSink* xsink) {
    boolean res = FALSE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, FALSE);
    if (elem->typinf->cols[0].type != OCI_CDT_NUMERIC) {
       QoreStringNode* desc = new QoreStringNode("cannot bind a numeric value to element '");
       if (elem->typinf->schema) {
	  desc->concat(elem->typinf->schema);
	  desc->concat('.');
       }
       desc->sprintf("%s' of type '%s'", elem->typinf->name, OCI_GetColumnTypeName(elem->typinf->cols[0].type));
       return FALSE;
    }

    res = OCI_NumberConvertStr2(pOCILib, elem->con, (OCINumber*)elem->handle, str, size, NUM_FMT, NUM_FMT_LEN, xsink);

    if (res)
       OCI_ElemSetNullIndicator(elem, OCI_IND_NOTNULL);

    OCI_RESULT(pOCILib, res);

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemSetNumber
 * ------------------------------------------------------------------------ */
/*
boolean OCI_ElemSetNumber(OCI_Elem  *elem, void *value, uword size, uword flag)
{
   return OCI_ElemSetNumber2(&OCILib, elem, value, size, flag);
}
*/

boolean OCI_ElemSetNumber2(OCI_Library *pOCILib, OCI_Elem  *elem, void *value, uword size, uword flag, ExceptionSink* xsink)
{
    boolean res = FALSE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, FALSE);
    if (elem->typinf->cols[0].type != OCI_CDT_NUMERIC) {
       QoreStringNode* desc = new QoreStringNode("cannot bind a numeric value to element '");
       if (elem->typinf->schema) {
	  desc->concat(elem->typinf->schema);
	  desc->concat('.');
       }
       desc->sprintf("%s' of type '%s'", elem->typinf->name, OCI_GetColumnTypeName(elem->typinf->cols[0].type));
       return FALSE;
    }

    res = OCI_NumberSet2(pOCILib, elem->con, (OCINumber *) elem->handle, value, size, flag, xsink);

    if (res)
       OCI_ElemSetNullIndicator(elem, OCI_IND_NOTNULL);

    OCI_RESULT(pOCILib, res);

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemGetNumber
 * ------------------------------------------------------------------------ */

boolean OCI_ElemGetNumber2(OCI_Library *pOCILib, OCI_Elem *elem, void *value, uword size, uword flag, ExceptionSink* xsink)
{
    boolean res = FALSE;
    
    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, FALSE);
    
    if (elem->typinf->cols[0].type == OCI_CDT_NUMERIC)
    {
       //printd(5, "OCI_ElemGetNumber2() OCI_CDT_NUMERIC\n");
        OCINumber *num  = (OCINumber *) elem->handle;

        res = OCI_NumberGet2(pOCILib, elem->con, num, value, size, flag, xsink);
    }
    else if (elem->typinf->cols[0].type == OCI_CDT_TEXT)
    {
       //printd(5, "OCI_ElemGetNumber2() OCI_CDT_TEXT\n");

       const mtext *fmt = OCI_GetDefaultFormatNumeric(pOCILib, elem->con);
        ub4 fmt_size     = (ub4) mtslen(fmt);
        dtext *data      = (dtext *) OCI_ElemGetString2(pOCILib, elem);

        res = OCI_NumberGetFromStr2(pOCILib, elem->con, value, size, flag, data, 
				    (int) dtslen(data),  fmt, fmt_size, xsink);
    }
    else
    {
       OCI_ExceptionTypeNotCompatible2(pOCILib, elem->con, xsink);
    }

    OCI_RESULT(pOCILib, res);

    //printd(5, "OCI_ElemGetNumber2() size: %d res: %d\n", size, res);

    return res;
}

/* ************************************************************************ *
 *                            PUBLIC FUNCTIONS
 * ************************************************************************ */

/* ------------------------------------------------------------------------ *
 * OCI_ElemCreate
 * ------------------------------------------------------------------------ */

OCI_Elem * OCI_API OCI_ElemCreate2(OCI_Library *pOCILib, OCI_TypeInfo *typinf, ExceptionSink* xsink)
{
    OCI_Elem *elem = NULL;

    OCI_CHECK_INITIALIZED2(pOCILib, NULL);
    OCI_CHECK_PTR(pOCILib, OCI_IPC_TYPE_INFO, typinf, NULL);

    elem = OCI_ElemInit2(pOCILib, typinf->con, &elem, NULL, (OCIInd *) NULL, typinf, xsink);
                        
    OCI_RESULT(pOCILib, elem != NULL);

    return elem;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemFree
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_ElemFree2(OCI_Library *pOCILib, OCI_Elem *elem, ExceptionSink* xsink)
{
   assert(elem);
   //OCI_CHECK_PTRQ(pOCILib, OCI_IPC_ELEMENT, elem, FALSE, xsink);

    OCI_CHECK_OBJECT_FETCHED(elem, FALSE);

    /* if the element has sub-objects that have been fetched, we need to free
       these objects */

    if (elem->obj != NULL)
    {
        OCI_Datatype * data = (OCI_Datatype *) elem->obj;

        if (data->hstate == OCI_OBJECT_FETCHED_CLEAN)
            data->hstate = OCI_OBJECT_FETCHED_DIRTY;

        switch (elem->typinf->cols[0].type)
        {
            case OCI_CDT_DATETIME:

                OCI_DateFree(pOCILib, (OCI_Date *) elem->obj);
                break;

            case OCI_CDT_LOB:

	       OCI_LobFree(pOCILib, (OCI_Lob *) elem->obj, xsink);
                break;

            case OCI_CDT_FILE:
	       assert(false);
	       //OCI_FileFree(pOCILib, (OCI_File *) elem->obj);
                break;

            case OCI_CDT_OBJECT:

	       OCI_ObjectFree2(pOCILib, (OCI_Object *) elem->obj, xsink);
                break;

            case OCI_CDT_COLLECTION:

	       OCI_CollFree2(pOCILib, (OCI_Coll *) elem->obj, xsink);
	       break;

            case OCI_CDT_TIMESTAMP:

	       OCI_TimestampFree2(pOCILib, (OCI_Timestamp *) elem->obj);
                break;

            case OCI_CDT_INTERVAL:

                OCI_IntervalFree2(pOCILib, (OCI_Interval *) elem->obj);
                break;
            
            case OCI_CDT_REF:
	       assert(false);
	       //OCI_RefFree(pOCILib, (OCI_Ref *) elem->obj);
                break;       
        }
    }
    
    if ((elem->hstate == OCI_OBJECT_ALLOCATED) && 
        (elem->typinf->cols[0].type == OCI_CDT_NUMERIC))
    {
        OCI_FREE(elem->handle);
    }

    OCI_FREE(elem->buf);
    OCI_FREE(elem);

    OCI_RESULT(pOCILib, TRUE);

    return TRUE;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemGetShort
 * ------------------------------------------------------------------------ */

/*
short OCI_API OCI_ElemGetShort2(OCI_Library *pOCILib, OCI_Elem *elem)
{
    short value = 0;
    
    OCI_ElemGetNumber2(pOCILib, elem, (void *) &value, (uword) sizeof(short), 
                      (uword) OCI_NUM_SHORT);
  
    return value;
}
*/

/* ------------------------------------------------------------------------ *
 * OCI_ElemGetUnsignedShort
 * ------------------------------------------------------------------------ */

/*
unsigned short OCI_API OCI_ElemGetUnsignedShort2(OCI_Library *pOCILib, OCI_Elem *elem)
{
    unsigned short value = 0;
    
    OCI_ElemGetNumber2(pOCILib, elem, (void *) &value, (uword) sizeof(unsigned short),
                      (uword) OCI_NUM_USHORT);
 
    return value;
}
*/

/* ------------------------------------------------------------------------ *
 * OCI_ElemGetInt
 * ------------------------------------------------------------------------ */

/*
int OCI_API OCI_ElemGetInt2(OCI_Library *pOCILib, OCI_Elem *elem)
{
    int value = 0;
    
    OCI_ElemGetNumber2(pOCILib, elem, (void *) &value, (uword) sizeof(value),
                      (uword) OCI_NUM_INT);
 
    return value;
}
*/

/* ------------------------------------------------------------------------ *
 * OCI_ElemGetUnsignedInt
 * ------------------------------------------------------------------------ */

/*
unsigned int OCI_API OCI_ElemGetUnsignedInt2(OCI_Library *pOCILib, OCI_Elem *elem)
{
    unsigned int value = 0;
    
    OCI_ElemGetNumber2(pOCILib, elem, (void *) &value, (uword) sizeof(value),
                      (uword) OCI_NUM_UINT);
 
    return value;
}
*/

/* ------------------------------------------------------------------------ *
 * OCI_ElemGetBigInt
 * ------------------------------------------------------------------------ */

int64 OCI_API OCI_ElemGetBigInt2(OCI_Library *pOCILib, OCI_Elem *elem, ExceptionSink* xsink) {
   int64 value = 0;

   OCI_ElemGetNumber2(pOCILib, elem, (void *) &value, (uword) sizeof(int64), (uword) OCI_NUM_BIGINT, xsink);
   return value;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemGetUnsignedBigInt
 * ------------------------------------------------------------------------ */

/*
big_uint OCI_API OCI_ElemGetUnsignedBigInt2(OCI_Library *pOCILib, OCI_Elem *elem)
{   
    big_uint value = 0;

    OCI_ElemGetNumber2(pOCILib, elem, (void *) &value,  (uword) sizeof(big_uint),
                      (uword) OCI_NUM_BIGUINT);
 
    return value;
}
*/

/* ------------------------------------------------------------------------ *
 * OCI_ElemGetDouble
 * ------------------------------------------------------------------------ */

double OCI_API OCI_ElemGetDouble2(OCI_Library *pOCILib, OCI_Elem *elem, ExceptionSink* xsink)
{
    double value = 0.0;
    
    OCI_ElemGetNumber2(pOCILib, elem, (void *) &value, (uword) sizeof(double),
		       (uword) OCI_NUM_DOUBLE, xsink);

    return value;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemGetString
 * ------------------------------------------------------------------------ */
/*
const dtext * OCI_API OCI_ElemGetString(OCI_Elem *elem)
{
    return OCI_ElemGetString2(&OCILib, elem);
}
*/

const dtext * OCI_API OCI_ElemGetString2(OCI_Library *pOCILib, OCI_Elem *elem)
{
    const dtext *str  = NULL;
    boolean res       = FALSE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, NULL);

    if (elem->typinf->cols[0].type != OCI_CDT_TEXT) {
       QoreStringNode* desc = new QoreStringNode("cannot retrieve a text value for element '");
       if (elem->typinf->schema) {
	  desc->concat(elem->typinf->schema);
	  desc->concat('.');
       }
       desc->sprintf("%s' of type '%s'", elem->typinf->name, OCI_GetColumnTypeName(elem->typinf->cols[0].type));
       return FALSE;
    }
    
    if (elem->handle != NULL)
    {
        res = TRUE;

        str = (dtext *) OCI_StringFromStringPtr(pOCILib, (OCIString *) elem->handle,
                                                &elem->buf, &elem->buflen);
    }

    OCI_RESULT(pOCILib, res);

    return str;    
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemGetRaw
 * ------------------------------------------------------------------------ */

/*
unsigned int OCI_API OCI_ElemGetRaw(OCI_Elem *elem, void *value, unsigned int len)
{
    return OCI_ElemGetRaw2(&OCILib, elem, value, len);
}
*/
/*
unsigned int OCI_API OCI_ElemGetRaw2(OCI_Library *pOCILib, OCI_Elem *elem, void *value, unsigned int len)
{
    boolean res = FALSE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, 0);
    OCI_CHECK_COMPAT(pOCILib, elem->con, elem->typinf->cols[0].type == OCI_CDT_RAW, 0);

    if (elem->handle != NULL)
    {
        OCIRaw *raw = *(OCIRaw **) elem->handle;
        ub4 raw_len = 0;
    
        raw_len = OCIRawSize(pOCILib->env, raw);

        if (len > raw_len)
            len = raw_len;

        memcpy(value, OCIRawPtr(pOCILib->env, raw), (size_t) len);
    }

    OCI_RESULT(pOCILib, res);

    return len; 
}
*/
/* ------------------------------------------------------------------------ *
 * OCI_ElemGetDate
 * ------------------------------------------------------------------------ */
/*
OCI_Date * OCI_API  OCI_ElemGetDate(OCI_Elem *elem)
{
   return OCI_ElemGetDate2(&OCILib, elem);
}
*/

OCI_Date * OCI_API  OCI_ElemGetDate2(OCI_Library *pOCILib, OCI_Elem *elem)
{
    boolean res    = TRUE;
    OCI_Date *date = NULL;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, NULL);

    if (elem->typinf->cols[0].type != OCI_CDT_DATETIME) {
       QoreStringNode* desc = new QoreStringNode("cannot retrieve a datetime value for element '");
       if (elem->typinf->schema) {
	  desc->concat(elem->typinf->schema);
	  desc->concat('.');
       }
       desc->sprintf("%s' of type '%s'", elem->typinf->name, OCI_GetColumnTypeName(elem->typinf->cols[0].type));
       return FALSE;
    }
    
    if (elem->ind != OCI_IND_NULL)
    {
        if (elem->init == FALSE)
        {
            date = OCI_DateInit2(pOCILib, elem->con, (OCI_Date **) &elem->obj, 
                                (OCIDate *) elem->handle, FALSE, FALSE);

            elem->init = (date != NULL);
        }
        else
            date = (OCI_Date *) elem->obj;

        res = elem->init;
    }

    OCI_RESULT(pOCILib, res);

    return date;
}
/* ------------------------------------------------------------------------ *
 * OCI_ElemGetTimestamp
 * ------------------------------------------------------------------------ */
/*
OCI_Timestamp * OCI_API  OCI_ElemGetTimestamp(OCI_Elem *elem)
{
    return OCI_ElemGetTimestamp2(&OCILib, elem);
}
*/

OCI_Timestamp * OCI_API  OCI_ElemGetTimestamp2(OCI_Library *pOCILib, OCI_Elem *elem)
{
    boolean res = TRUE;
    OCI_Timestamp *tmsp = NULL;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, NULL);

    if (elem->typinf->cols[0].type != OCI_CDT_TIMESTAMP) {
       QoreStringNode* desc = new QoreStringNode("cannot retrieve a timestamp value for element '");
       if (elem->typinf->schema) {
	  desc->concat(elem->typinf->schema);
	  desc->concat('.');
       }
       desc->sprintf("%s' of type '%s'", elem->typinf->name, OCI_GetColumnTypeName(elem->typinf->cols[0].type));
       return FALSE;
    }

    if (elem->ind != OCI_IND_NULL)
    {
        if (elem->init == FALSE)
        {
            tmsp = OCI_TimestampInit(pOCILib, elem->con, (OCI_Timestamp **) &elem->obj, 
                                     (OCIDateTime *) elem->handle, 
                                     elem->typinf->cols[0].subtype);

            elem->init = (tmsp != NULL);
        }
        else
            tmsp = (OCI_Timestamp *) elem->obj;

        res = elem->init;
    }

    OCI_RESULT(pOCILib, res);

    return tmsp;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemGetInterval
 * ------------------------------------------------------------------------ */
/*
OCI_Interval * OCI_API OCI_ElemGetInterval(OCI_Elem *elem)
{
    return OCI_ElemGetInterval2(&OCILib, elem);
}
*/

OCI_Interval * OCI_API OCI_ElemGetInterval2(OCI_Library *pOCILib, OCI_Elem *elem)
{
    boolean res = TRUE;
    OCI_Interval *itv = NULL;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, NULL);

    if (elem->typinf->cols[0].type != OCI_CDT_INTERVAL) {
       QoreStringNode* desc = new QoreStringNode("cannot retrieve an interval value for element '");
       if (elem->typinf->schema) {
	  desc->concat(elem->typinf->schema);
	  desc->concat('.');
       }
       desc->sprintf("%s' of type '%s'", elem->typinf->name, OCI_GetColumnTypeName(elem->typinf->cols[0].type));
       return FALSE;
    }

    if (elem->ind != OCI_IND_NULL)
    {
        if (elem->init == FALSE)
        {
            itv = OCI_IntervalInit(pOCILib, elem->con, (OCI_Interval **) &elem->obj, 
                                    (OCIInterval *) elem->handle,
                                    elem->typinf->cols[0].subtype);

            elem->init = (itv != NULL);
        }
        else
            itv = (OCI_Interval *) elem->obj;

        res = elem->init;
    }

    OCI_RESULT(pOCILib, res);

    return itv;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemGetLob
 * ------------------------------------------------------------------------ */
/*
OCI_Lob * OCI_API OCI_ElemGetLob(OCI_Elem *elem)
{
    return OCI_ElemGetLob2(&OCILib, elem);
}
*/

OCI_Lob * OCI_API OCI_ElemGetLob2(OCI_Library *pOCILib, OCI_Elem *elem, ExceptionSink* xsink)
{
    boolean res  = TRUE;
    OCI_Lob *lob = NULL;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, NULL);
    if (elem->typinf->cols[0].type != OCI_CDT_LOB) {
       QoreStringNode* desc = new QoreStringNode("cannot retrieve a LOB value from element '");
       if (elem->typinf->schema) {
	  desc->concat(elem->typinf->schema);
	  desc->concat('.');
       }
       desc->sprintf("%s' of type '%s'", elem->typinf->name, OCI_GetColumnTypeName(elem->typinf->cols[0].type));
       return FALSE;
    }

    if (elem->ind != OCI_IND_NULL)
    {      
        if (elem->init == FALSE)
        {
            lob = OCI_LobInit(pOCILib, elem->con, (OCI_Lob **) &elem->obj, 
                              (OCILobLocator *) elem->handle,
                              elem->typinf->cols[0].subtype, xsink);

            elem->init = (lob != NULL);
        }
        else
            lob = (OCI_Lob *) elem->obj;

        res = elem->init;
    }

    OCI_RESULT(pOCILib, res);

    return lob;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemGetObject
 * ------------------------------------------------------------------------ */
/*
OCI_Object * OCI_API OCI_ElemGetObject(OCI_Elem *elem)
{
    return OCI_ElemGetObject2(&OCILib, elem);
}
*/

OCI_Object * OCI_API OCI_ElemGetObject2(OCI_Library * pOCILib, OCI_Elem *elem, ExceptionSink* xsink)
{
    boolean res = TRUE;
    OCI_Object *obj = NULL;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, NULL);

    if (elem->typinf->cols[0].type != OCI_CDT_OBJECT) {
       QoreStringNode* desc = new QoreStringNode("cannot retrieve an object value from element '");
       if (elem->typinf->schema) {
	  desc->concat(elem->typinf->schema);
	  desc->concat('.');
       }
       desc->sprintf("%s' of type '%s'", elem->typinf->name, OCI_GetColumnTypeName(elem->typinf->cols[0].type));
       return FALSE;
    }

    if (elem->ind != OCI_IND_NULL)
    {
        if (elem->init == FALSE)
        {
            obj = OCI_ObjectInit(pOCILib, elem->con, (OCI_Object **) &elem->obj,
                                 elem->handle, 
                                 elem->typinf->cols[0].typinf,
                                 NULL, -1, TRUE, xsink);

            elem->init = (obj != NULL);
        }
        else
            obj = (OCI_Object *) elem->obj;

        res = elem->init;
    }

    OCI_RESULT(pOCILib, res);

    return obj;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemGetColl
 * ------------------------------------------------------------------------ */
/*
OCI_Coll * OCI_API OCI_ElemGetColl(OCI_Elem *elem)
{
    return OCI_ElemGetColl2(&OCILib, elem);
}
*/

OCI_Coll * OCI_API OCI_ElemGetColl2(OCI_Library *pOCILib, OCI_Elem *elem, ExceptionSink* xsink)
{
    boolean res    = TRUE;
    OCI_Coll *coll = NULL;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, NULL);

    if (elem->typinf->cols[0].type != OCI_CDT_COLLECTION) {
       QoreStringNode* desc = new QoreStringNode("cannot retrieve a collection from element '");
       if (elem->typinf->schema) {
	  desc->concat(elem->typinf->schema);
	  desc->concat('.');
       }
       desc->sprintf("%s' of type '%s'", elem->typinf->name, OCI_GetColumnTypeName(elem->typinf->cols[0].type));
       return FALSE;
    }

    if (elem->ind != OCI_IND_NULL)
    {
        if (elem->init == FALSE)
        {
            coll = OCI_CollInit(pOCILib, elem->con, (OCI_Coll **) &elem->obj, 
                                (OCIColl *) elem->handle, 
                                elem->typinf->cols[0].typinf, xsink);

            elem->init = (coll != NULL);
        }
        else
            coll = (OCI_Coll *) elem->obj;

        res = elem->init;
    }

    OCI_RESULT(pOCILib, res);

    return coll;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemSetShort
 * ------------------------------------------------------------------------ */
/*
boolean OCI_API OCI_ElemSetShort(OCI_Library *pOCILib, OCI_Elem *elem, short value)
{
    return OCI_ElemSetNumber2(pOCILib, elem, (void *) &value, (uword) sizeof(value), 
                             (uword) OCI_NUM_SHORT);
}
*/
/* ------------------------------------------------------------------------ *
 * OCI_ElemSetUnsignedShort
 * ------------------------------------------------------------------------ */
/*
boolean OCI_API OCI_ElemSetUnsignedShort(OCI_Library *pOCILib, OCI_Elem *elem, unsigned short value)
{
    return OCI_ElemSetNumber2(pOCILib, elem, (void *) &value, (uword) sizeof(value), 
                             (uword) OCI_NUM_USHORT);
}
*/
/* ------------------------------------------------------------------------ *
 * OCI_ElemSetInt
 * ------------------------------------------------------------------------ */
/*
boolean OCI_API OCI_ElemSetInt(OCI_Library *pOCILib, OCI_Elem *elem, int value)
{
    return OCI_ElemSetNumber2(pOCILib, elem, (void *) &value, (uword) sizeof(value), 
                             (uword) OCI_NUM_INT);
}
*/
/* ------------------------------------------------------------------------ *
 * OCI_ElemSetUnsignedInt
 * ------------------------------------------------------------------------ */
/*
boolean OCI_API OCI_ElemSetUnsignedInt(OCI_Library *pOCILib, OCI_Elem *elem, unsigned int value)
{
    return OCI_ElemSetNumber2(pOCILib, elem, (void *) &value, (uword) sizeof(value), 
                             (uword) OCI_NUM_UINT);
}
*/
/* ------------------------------------------------------------------------ *
 * OCI_ElemSetBigInt
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_ElemSetBigInt(OCI_Library *pOCILib, OCI_Elem *elem, big_int value, ExceptionSink* xsink)
{
    return OCI_ElemSetNumber2(pOCILib, elem, (void *) &value, (uword) sizeof(value), 
			      (uword) OCI_NUM_BIGINT, xsink);
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemSetUnsignedBigInt
 * ------------------------------------------------------------------------ */
/*
boolean OCI_API OCI_ElemSetUnsignedBigInt(OCI_Library *pOCILib, OCI_Elem *elem, big_uint value)
{
    return OCI_ElemSetNumber2(pOCILib, elem, (void *) &value, (uword) sizeof(value), 
                             (uword) OCI_NUM_BIGUINT);
}
*/
/* ------------------------------------------------------------------------ *
 * OCI_ElemSetDouble
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_ElemSetDouble(OCI_Library *pOCILib, OCI_Elem *elem, double value, ExceptionSink* xsink)
{
    return OCI_ElemSetNumber2(pOCILib, elem, (void *) &value, (uword) sizeof(value), 
			      (uword) OCI_NUM_DOUBLE, xsink);
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemSetString
 * ------------------------------------------------------------------------ */
/*
boolean OCI_API OCI_ElemSetString(OCI_Elem *elem, const dtext *value)
{
    return OCI_ElemSetString2(&OCILib, elem, value);
}
*/

boolean OCI_API OCI_ElemSetString2(OCI_Library *pOCILib, OCI_Elem *elem, const dtext *value, ExceptionSink* xsink)
{
    boolean res  = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, FALSE);
    if (elem->typinf->cols[0].type != OCI_CDT_TEXT) {
       QoreStringNode* desc = new QoreStringNode("cannot bind a string value to element '");
       if (elem->typinf->schema) {
	  desc->concat(elem->typinf->schema);
	  desc->concat('.');
       }
       desc->sprintf("%s' of type '%s'", elem->typinf->name, OCI_GetColumnTypeName(elem->typinf->cols[0].type));
       return FALSE;
    }

    if (value == NULL)
    {
        res = OCI_ElemSetNull2(pOCILib, elem);
    }
    else
    {
        res = OCI_StringToStringPtr(pOCILib, (OCIString **) &elem->handle, 
                                    elem->con->err, (void *) value, 
                                    &elem->buf, &elem->buflen, xsink);

        OCI_ElemSetNullIndicator(elem, OCI_IND_NOTNULL);
    }

    OCI_RESULT(pOCILib, res);

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemSetRaw
 * ------------------------------------------------------------------------ */
/*
boolean OCI_API OCI_ElemSetRaw(OCI_Elem *elem, void* value, unsigned int len)
{
    return OCI_ElemSetRaw2(&OCILib, elem, value, len);
}
*/

/*
boolean OCI_API OCI_ElemSetRaw2(OCI_Library *pOCILib, OCI_Elem *elem, void* value, unsigned int len, ExceptionSink* xsink)
{
    boolean res  = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, FALSE);

    if (elem->typinf->cols[0].type != OCI_CDT_RAW) {
       QoreStringNode* desc = new QoreStringNode("cannot bind a raw value to element '");
       if (elem->typinf->schema) {
	  desc->concat(elem->typinf->schema);
	  desc->concat('.');
       }
       desc->sprintf("%s' of type '%s'", elem->typinf->name, OCI_GetColumnTypeName(elem->typinf->cols[0].type));
       return FALSE;
    }
 
    if (value == NULL)
    {
        res = OCI_ElemSetNull2(pOCILib, elem);
    }
    else
    {
       OCI_CHECK_MIN(pOCILib, elem->con, NULL, len, 1, FALSE);                                  

        OCI_CALL2Q
        (
	   pOCILib, res, elem->con, 
            
            OCIRawAssignBytes(pOCILib->env, elem->con->err, (ub1*) value,
                              len, (OCIRaw **) &elem->handle),

	   xsink
        )

        OCI_ElemSetNullIndicator(elem, OCI_IND_NOTNULL);
    }

    OCI_RESULT(pOCILib, res);

    return res;
}
*/

/* ------------------------------------------------------------------------ *
 * OCI_ElemSetDate
 * ------------------------------------------------------------------------ */
/*
boolean OCI_API OCI_ElemSetDate(OCI_Elem *elem, OCI_Date *value)
{
   return OCI_ElemSetDate2(&OCILib, elem, value);
}
*/

boolean OCI_API OCI_ElemSetDate2(OCI_Library *pOCILib, OCI_Elem *elem, OCI_Date *value, ExceptionSink* xsink)
{
    boolean res  = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, FALSE);

    if (elem->typinf->cols[0].type != OCI_CDT_DATETIME) {
       QoreStringNode* desc = new QoreStringNode("cannot bind a datetime value to element '");
       if (elem->typinf->schema) {
	  desc->concat(elem->typinf->schema);
	  desc->concat('.');
       }
       desc->sprintf("%s' of type '%s'", elem->typinf->name, OCI_GetColumnTypeName(elem->typinf->cols[0].type));
       return FALSE;
    }
  
    if (value == NULL)
    {
        res = OCI_ElemSetNull2(pOCILib, elem);
    }
    else
    {
        if (elem->obj == NULL)
        {
            OCI_DateInit2(pOCILib, elem->con, (OCI_Date **) &elem->obj, 
                         (OCIDate *) elem->handle, TRUE, FALSE);
        }

        if (elem->obj != NULL)
        {
	   res = OCI_DateAssign(pOCILib, (OCI_Date *) elem->obj, value, xsink);

            if (res == TRUE)
            {
                OCI_ElemSetNullIndicator(elem, OCI_IND_NOTNULL);

                elem->handle = ((OCI_Date *) elem->obj)->handle;
            }
        }
    }

    OCI_RESULT(pOCILib, res);

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemSetTimestamp
 * ------------------------------------------------------------------------ */
/*
boolean OCI_API OCI_ElemSetTimestamp(OCI_Elem *elem, OCI_Timestamp *value)
{
    return OCI_ElemSetTimestamp2(&OCILib, elem, value);
}
*/

boolean OCI_API OCI_ElemSetTimestamp2(OCI_Library *pOCILib, OCI_Elem *elem, OCI_Timestamp *value, ExceptionSink* xsink)
{
    boolean res  = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, FALSE);

    if (elem->typinf->cols[0].type != OCI_CDT_TIMESTAMP) {
       QoreStringNode* desc = new QoreStringNode("cannot bind a timestamp value to element '");
       if (elem->typinf->schema) {
	  desc->concat(elem->typinf->schema);
	  desc->concat('.');
       }
       desc->sprintf("%s' of type '%s'", elem->typinf->name, OCI_GetColumnTypeName(elem->typinf->cols[0].type));
       return FALSE;
    }
  
    if (value == NULL)
    {
        res = OCI_ElemSetNull2(pOCILib, elem);
    }
    else
    {
        if (elem->obj == NULL)
        {
            OCI_TimestampInit(pOCILib, elem->con, (OCI_Timestamp **) &elem->obj,
                              (OCIDateTime   *) elem->handle,
                              elem->typinf->cols[0].subtype);
        }

        if (elem->obj != NULL)
        {
	   res = OCI_TimestampAssign2(pOCILib, (OCI_Timestamp *) elem->obj, value, xsink);

            if (res == TRUE)
            {
                OCI_ElemSetNullIndicator(elem, OCI_IND_NOTNULL);

                elem->handle = ((OCI_Timestamp *) elem->obj)->handle;
            }
        }
    }

    OCI_RESULT(pOCILib, res);

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemSetInterval
 * ------------------------------------------------------------------------ */
/*
boolean OCI_API OCI_ElemSetInterval(OCI_Elem *elem, OCI_Interval *value)
{
    return OCI_ElemSetInterval2(&OCILib, elem, value);
}
*/

boolean OCI_API OCI_ElemSetInterval2(OCI_Library *pOCILib, OCI_Elem *elem, OCI_Interval *value, ExceptionSink* xsink)
{
    boolean res  = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, FALSE);

    if (elem->typinf->cols[0].type != OCI_CDT_INTERVAL) {
       QoreStringNode* desc = new QoreStringNode("cannot bind an interval value to element '");
       if (elem->typinf->schema) {
	  desc->concat(elem->typinf->schema);
	  desc->concat('.');
       }
       desc->sprintf("%s' of type '%s'", elem->typinf->name, OCI_GetColumnTypeName(elem->typinf->cols[0].type));
       return FALSE;
    }

    if (value == NULL)
    {
        res = OCI_ElemSetNull2(pOCILib, elem);
    }
    else
    {
        if (elem->obj == NULL)
        {
            OCI_IntervalInit(pOCILib, elem->con, (OCI_Interval **) &elem->obj,
                             (OCIInterval  *) elem->handle,
                             elem->typinf->cols[0].subtype);
        }

        if (elem->obj != NULL)
        {
	   res = OCI_IntervalAssign2(pOCILib, (OCI_Interval *) elem->obj, value, xsink);

            if (res == TRUE)
            {
                OCI_ElemSetNullIndicator(elem, OCI_IND_NOTNULL);

                elem->handle = ((OCI_Interval *) elem->obj)->handle;
            }
        }
    }

    OCI_RESULT(pOCILib, res);

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemSetColl
 * ------------------------------------------------------------------------ */
/*
boolean OCI_API OCI_ElemSetColl(OCI_Elem *elem, OCI_Coll *value)
{
    return OCI_ElemSetColl2(&OCILib, elem, value);
}
*/

boolean OCI_API OCI_ElemSetColl2(OCI_Library *pOCILib, OCI_Elem *elem, OCI_Coll *value, ExceptionSink* xsink)
{
    boolean res  = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, FALSE);

    if (elem->typinf->cols[0].type != OCI_CDT_COLLECTION) {
       QoreStringNode* desc = new QoreStringNode("cannot bind a collection value to element '");
       if (elem->typinf->schema) {
	  desc->concat(elem->typinf->schema);
	  desc->concat('.');
       }
       desc->sprintf("%s' of type '%s'", elem->typinf->name, OCI_GetColumnTypeName(elem->typinf->cols[0].type));
       return FALSE;
    }

    if (value == NULL)
    {
        res = OCI_ElemSetNull2(pOCILib, elem);
    }
    else
    {
        if (elem->obj == NULL)
        {
            OCI_CollInit(pOCILib, elem->con, (OCI_Coll **) &elem->obj,
                         (OCIColl  *) elem->handle,
                         elem->typinf->cols[0].typinf, xsink);
        }

        if (elem->obj != NULL)
        {
	   res = OCI_CollAssign(pOCILib, (OCI_Coll *) elem->obj, value, xsink);
            
            if (res == TRUE)
            {
                OCI_ElemSetNullIndicator(elem, OCI_IND_NOTNULL);

                elem->handle = ((OCI_Coll *) elem->obj)->handle;
            }
        }
    }

    OCI_RESULT(pOCILib, res);

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemSetObject
 * ------------------------------------------------------------------------ */
/*
boolean OCI_API OCI_ElemSetObject(OCI_Elem *elem, OCI_Object *value)
{
    return OCI_ElemSetObject2(&OCILib, elem, value);
}
*/

boolean OCI_API OCI_ElemSetObject2(OCI_Library *pOCILib, OCI_Elem *elem, OCI_Object *value, ExceptionSink* xsink)
{
    boolean res  = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, FALSE);

    if (elem->typinf->cols[0].type != OCI_CDT_OBJECT) {
       QoreStringNode* desc = new QoreStringNode("cannot bind an object value to element '");
       if (elem->typinf->schema) {
	  desc->concat(elem->typinf->schema);
	  desc->concat('.');
       }
       desc->sprintf("%s' of type '%s'", elem->typinf->name, OCI_GetColumnTypeName(elem->typinf->cols[0].type));
       return FALSE;
    }
  
    if (value == NULL)
    {
        res = OCI_ElemSetNull2(pOCILib, elem);
    }
    else
    {
        if (elem->obj == NULL)
        {
            OCI_ObjectInit(pOCILib, elem->con, (OCI_Object **) &elem->obj,
                           elem->handle, elem->typinf->cols[0].typinf, 
                           NULL, -1, TRUE, xsink);
        }

        if (elem->obj != NULL)
        {
	   res = OCI_ObjectAssign2(pOCILib, (OCI_Object *) elem->obj, value, xsink);

            if (res == TRUE)
            {
                OCI_ElemSetNullIndicator(elem, OCI_IND_NOTNULL);

                elem->handle = ((OCI_Object *) elem->obj)->handle;
            }
        }
    }

    OCI_RESULT(pOCILib, res);

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemSetLob
 * ------------------------------------------------------------------------ */
boolean OCI_API OCI_ElemSetLob2(OCI_Library *pOCILib, OCI_Elem *elem, OCI_Lob *value, ExceptionSink* xsink)
{
    boolean res  = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, FALSE);

    if (elem->typinf->cols[0].type != OCI_CDT_LOB) {
       QoreStringNode* desc = new QoreStringNode("cannot bind a lob value to element '");
       if (elem->typinf->schema) {
	  desc->concat(elem->typinf->schema);
	  desc->concat('.');
       }
       desc->sprintf("%s' of type '%s'", elem->typinf->name, OCI_GetColumnTypeName(elem->typinf->cols[0].type));
       return FALSE;
    }
  
    if (value == NULL)
    {
        res = OCI_ElemSetNull2(pOCILib, elem);
    }
    else
    {
        if (elem->obj == NULL)
        {
            OCI_LobInit(pOCILib, elem->con, (OCI_Lob **) &elem->obj,
                        (OCILobLocator *) elem->handle,
                        elem->typinf->cols[0].subtype, xsink);
        }

        if (elem->obj != NULL)
        {
	   res = OCI_LobAssign(pOCILib, (OCI_Lob *) elem->obj, value, xsink);

            if (res == TRUE)
            {
                OCI_ElemSetNullIndicator(elem, OCI_IND_NOTNULL);

                elem->handle = ((OCI_Lob *) elem->obj)->handle;
            }
        }
    }

    OCI_RESULT(pOCILib, res);

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemSetFile
 * ------------------------------------------------------------------------ */
/*
boolean OCI_API OCI_ElemSetFile(OCI_Elem *elem, OCI_File *value)
{
   return OCI_ElemSetFile2(&OCILib, elem, value);
}

boolean OCI_API OCI_ElemSetFile2(OCI_Library *pOCILib, OCI_Elem *elem, OCI_File *value)
{
    boolean res  = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, FALSE);
    OCI_CHECK_COMPAT(pOCILib, elem->con, elem->typinf->cols[0].type == OCI_CDT_FILE, FALSE);
  
    if (value == NULL)
    {
        res = OCI_ElemSetNull2(pOCILib, elem);
    }
    else
    {
        if (elem->obj == NULL)
        {
            OCI_FileInit(elem->con, (OCI_File **) &elem->obj,
                         (OCILobLocator *) elem->handle, 
                         elem->typinf->cols[0].subtype);
        }

        if (elem->obj != NULL)
        {
            res = OCI_FileAssign((OCI_File *) elem->obj, value);

            if (res == TRUE)
            {
                OCI_ElemSetNullIndicator(elem, OCI_IND_NOTNULL);

                elem->handle = ((OCI_Object *) elem->obj)->handle;
            }
        }
    }

    OCI_RESULT(pOCILib, res);

    return res;
}
*/


/* ------------------------------------------------------------------------ *
 * OCI_ElemSetRef
 * ------------------------------------------------------------------------ */
/*
boolean OCI_API OCI_ElemSetRef(OCI_Elem *elem, OCI_Ref *value)
{
    return OCI_ElemSetRef2(&OCILib, elem, value);
}

boolean OCI_API OCI_ElemSetRef2(OCI_Library *pOCILib, OCI_Elem *elem, OCI_Ref *value)
{
    boolean res  = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, FALSE);
    OCI_CHECK_COMPAT(pOCILib, elem->con, elem->typinf->cols[0].type == OCI_CDT_REF, FALSE);
  
    if (value == NULL)
    {
        res = OCI_ElemSetNull2(pOCILib, elem);
    }
    else
    {
        if (elem->obj == NULL)
        {
            OCI_RefInit(elem->con,elem->typinf->cols[0].typinf, 
                        (OCI_Ref **) &elem->obj, (OCIRef *) elem->handle);
        }

        if (elem->obj != NULL)
        {
            res = OCI_RefAssign((OCI_Ref *) elem->obj, value);

            if (res == TRUE)
            {
                OCI_ElemSetNullIndicator(elem, OCI_IND_NOTNULL);

                elem->handle = ((OCI_Ref *) elem->obj)->handle;
            }
        }
    }

    OCI_RESULT(pOCILib, res);

    return res;
}
*/

/* ------------------------------------------------------------------------ *
 * OCI_ElemIsNull
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_ElemIsNull2(OCI_Library *pOCILib, OCI_Elem *elem)
{
    boolean ret = FALSE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, FALSE);

    if (elem->pind != NULL)
    {
        ret = (*elem->pind == OCI_IND_NULL);
    }

    OCI_RESULT(pOCILib, TRUE);

    return ret;
}

/* ------------------------------------------------------------------------ *
 * OCI_ElemSetNull
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_ElemSetNull2(OCI_Library *pOCILib, OCI_Elem *elem)
{
    boolean res = FALSE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_ELEMENT, elem, FALSE);

    res = OCI_ElemSetNullIndicator(elem, OCI_IND_NULL);

    OCI_RESULT(pOCILib, res);
   
    return res;
}
