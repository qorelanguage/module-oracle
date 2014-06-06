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
 * $Id: timestamp.c, v 3.7.1 2010-07-27 18:35 Vincent Rogier $
 * ------------------------------------------------------------------------ */

#include "ocilib_internal.h"

/* ************************************************************************ *
 *                             PRIVATE FUNCTIONS
 * ************************************************************************ */

/* ------------------------------------------------------------------------ *
 * OCI_TimestampInit
 * ------------------------------------------------------------------------ */

OCI_Timestamp * OCI_TimestampInit(OCI_Library *pOCILib, OCI_Connection *con, OCI_Timestamp **ptmsp,
                                  OCIDateTime *buffer, ub4 type)
{
    OCI_Timestamp *tmsp = NULL;

#if OCI_VERSION_COMPILE >= OCI_9_0

    boolean res = TRUE;

    OCI_CHECK(ptmsp == NULL, NULL);

    if (*ptmsp == NULL)
        *ptmsp = (OCI_Timestamp *) OCI_MemAlloc2(pOCILib, OCI_IPC_TIMESTAMP, sizeof(*tmsp),
                                                (size_t) 1, TRUE);

    if (*ptmsp != NULL)
    {
        tmsp = *ptmsp;

        tmsp->con    = con;
        tmsp->handle = buffer;
        tmsp->type   = type;

        /* get the right error handle */

        if (con != NULL)
            tmsp->err = con->err;
        else
            tmsp->err = pOCILib->err;

        /* allocate buffer if needed */

        if ((tmsp->handle == NULL) || (tmsp->hstate == OCI_OBJECT_ALLOCATED_ARRAY))
        {
            ub4 htype = 0;

            if (tmsp->type == OCI_TIMESTAMP)
                htype = OCI_DTYPE_TIMESTAMP;
            else if (tmsp->type == OCI_TIMESTAMP_TZ)
                htype = OCI_DTYPE_TIMESTAMP_TZ;
            else if (tmsp->type == OCI_TIMESTAMP_LTZ)
                htype = OCI_DTYPE_TIMESTAMP_LTZ;

            if (tmsp->hstate != OCI_OBJECT_ALLOCATED_ARRAY)
            {
                res = (OCI_SUCCESS == OCI_DescriptorAlloc2(pOCILib, (dvoid  *) pOCILib->env,
							   (dvoid **) (void *) &tmsp->handle,
							   (ub4     ) htype, (size_t) 0,
							   (dvoid **) NULL));
                tmsp->hstate = OCI_OBJECT_ALLOCATED;
            }

        }
        else
            tmsp->hstate = OCI_OBJECT_FETCHED_CLEAN;
    }
    else
        res = FALSE;

    /* check for failure */

    if (res == FALSE)
    {
        OCI_TimestampFree2(pOCILib, tmsp);
        tmsp = NULL;
    }

#else

    OCI_NOT_USED(con);
    OCI_NOT_USED(type);
    OCI_NOT_USED(buffer);
    OCI_NOT_USED(ptmsp);

#endif

    return tmsp;
}

/* ************************************************************************ *
 *                            PUBLIC FUNCTIONS
 * ************************************************************************ */

/* ------------------------------------------------------------------------ *
 * OCI_TimestampCreate
 * ------------------------------------------------------------------------ */
/*
OCI_Timestamp * OCI_API OCI_TimestampCreate(OCI_Connection *con,
                                            unsigned int type)
{
    return OCI_TimestampCreate2(&OCILib, con, type);
}
*/

OCI_Timestamp * OCI_API OCI_TimestampCreate2(OCI_Library *pOCILib, OCI_Connection *con,
					     unsigned int type)
{
    OCI_Timestamp *tmsp = NULL;

    OCI_CHECK_INITIALIZED2(pOCILib, NULL);

    OCI_CHECK_TIMESTAMP_ENABLED(pOCILib, con, NULL);

#if OCI_VERSION_COMPILE >= OCI_9_0

    tmsp = OCI_TimestampInit(pOCILib, con, &tmsp, NULL, type);

#else

    OCI_NOT_USED(type);

#endif

    OCI_RESULT(pOCILib, tmsp != NULL);

    return tmsp;
}

/* ------------------------------------------------------------------------ *
 * OCI_TimestampFree
 * ------------------------------------------------------------------------ */
/*
boolean OCI_API OCI_TimestampFree(OCI_Timestamp *tmsp)
{
   return OCI_TimestampFree2(&OCILib, tmsp);
}
*/

boolean OCI_API OCI_TimestampFree2(OCI_Library *pOCILib, OCI_Timestamp *tmsp)
{
   assert(tmsp);
   //OCI_CHECK_PTRQ(pOCILib, OCI_IPC_TIMESTAMP, tmsp, FALSE, xsink);

    OCI_CHECK_TIMESTAMP_ENABLED(pOCILib, tmsp->con, FALSE);

#if OCI_VERSION_COMPILE >= OCI_9_0

    OCI_CHECK_OBJECT_FETCHED(tmsp, FALSE);

    if (tmsp->hstate == OCI_OBJECT_ALLOCATED)
    {
        ub4 htype  = 0;

        if (tmsp->type == OCI_TIMESTAMP)
            htype = OCI_DTYPE_TIMESTAMP;
        else if (tmsp->type == OCI_TIMESTAMP_TZ)
            htype = OCI_DTYPE_TIMESTAMP_TZ;
        else if (tmsp->type == OCI_TIMESTAMP_LTZ)
            htype = OCI_DTYPE_TIMESTAMP_LTZ;

       OCI_DescriptorFree2(pOCILib, (dvoid *) tmsp->handle, htype);
    }

    if (tmsp->hstate != OCI_OBJECT_ALLOCATED_ARRAY)
    {
        OCI_FREE(tmsp);
    }

#endif

   OCI_RESULT(pOCILib, TRUE);

   return TRUE;
}

/* ------------------------------------------------------------------------ *
 * OCI_DateZoneToZone
 * ------------------------------------------------------------------------ */
/*
boolean OCI_API OCI_TimestampAssign(OCI_Timestamp *tmsp, OCI_Timestamp *tmsp_src)
{
    return OCI_TimestampAssign2(&OCILib, tmsp, tmsp_src);
}
*/
boolean OCI_API OCI_TimestampAssign2(OCI_Library *pOCILib, OCI_Timestamp *tmsp, OCI_Timestamp *tmsp_src, ExceptionSink* xsink)
{
    boolean res = TRUE;
    
    assert(tmsp);
    assert(tmsp_src);
    //OCI_CHECK_PTR(pOCILib, OCI_IPC_TIMESTAMP, tmsp,     FALSE);
    //OCI_CHECK_PTR(pOCILib, OCI_IPC_TIMESTAMP, tmsp_src, FALSE);

    OCI_CHECK_TIMESTAMP_ENABLED(pOCILib, tmsp->con, FALSE);

#if OCI_VERSION_COMPILE >= OCI_9_0

    OCI_CALL4Q
    (
        pOCILib, res, tmsp->err, tmsp->con,

        OCIDateTimeAssign((dvoid *) pOCILib->env, tmsp->err,
                          tmsp_src->handle, tmsp->handle),

	xsink
    )

#endif

   OCI_RESULT(pOCILib, res);

   return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_TimestampConstruct
 * ------------------------------------------------------------------------ */
/*
boolean OCI_API OCI_TimestampConstruct(OCI_Timestamp *tmsp, int year,int month,
                                      int day, int hour,  int min, int sec,
                                      int fsec, const mtext *timezone)
{
    return OCI_TimestampConstruct2(&OCILib, tmsp, year, month, day, hour, min, sec, fsec, timezone);
}
*/

boolean OCI_API OCI_TimestampConstruct2(OCI_Library *pOCILib, OCI_Timestamp *tmsp, int year,int month,
                                      int day, int hour,  int min, int sec,
					int fsec, const mtext *timezone, ExceptionSink* xsink)
{
    boolean res = TRUE;

    assert(tmsp);
    //OCI_CHECK_PTR(pOCILib, OCI_IPC_TIMESTAMP, tmsp, FALSE);

    OCI_CHECK_TIMESTAMP_ENABLED(pOCILib, tmsp->con, FALSE);

#if OCI_VERSION_COMPILE >= OCI_9_0

    OCI_CALL4Q
    (
        pOCILib, res, tmsp->err, tmsp->con,

        OCIDateTimeConstruct((dvoid *) pOCILib->env, tmsp->err,
                                         tmsp->handle,
                                         (sb2) year, (ub1) month, (ub1) day,
                                         (ub1) hour, (ub1) min,(ub1) sec,
                                         (ub4) fsec, (OraText *) timezone,
			     (size_t) (timezone ? mtextsize(timezone) : 0)),

	xsink
    )

#else

    OCI_NOT_USED(year);
    OCI_NOT_USED(month);
    OCI_NOT_USED(day);
    OCI_NOT_USED(hour);
    OCI_NOT_USED(min);
    OCI_NOT_USED(sec);
    OCI_NOT_USED(fsec);
    OCI_NOT_USED(timezone);

#endif

    OCI_RESULT(pOCILib, res);

   return res;
}
