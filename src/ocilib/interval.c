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
 * $Id: interval.c, v 3.7.0 2010-07-20 17:45 Vincent Rogier $
 * ------------------------------------------------------------------------ */

#include "ocilib_internal.h"

/* ************************************************************************ *
 *                             PRIVATE FUNCTIONS
 * ************************************************************************ */

/* ------------------------------------------------------------------------ *
 *                         OCI_Interval functions
 * ------------------------------------------------------------------------ */

OCI_Interval * OCI_IntervalInit(OCI_Library *pOCILib, OCI_Connection *con, OCI_Interval **pitv,
                                OCIInterval *buffer, ub4 type)
{
    OCI_Interval *itv = NULL;

#if OCI_VERSION_COMPILE >= OCI_9_0

    boolean res       = TRUE;

    OCI_CHECK(pitv == NULL, NULL);

    if (*pitv == NULL)
        *pitv = (OCI_Interval *) OCI_MemAlloc(OCI_IPC_INTERVAL, sizeof(*itv),
                                              (size_t) 1, TRUE);

    if (*pitv != NULL)
    {
        itv = *pitv;

        itv->con    = con;
        itv->handle = buffer;
        itv->type   = type;

        /* get the right error handle */

        if (con != NULL)
            itv->err = con->err;
        else
            itv->err = OCILib.err;

        /* allocate buffer if needed */

        if ((itv->handle == NULL) || (itv->hstate == OCI_OBJECT_ALLOCATED_ARRAY))
        {
            ub4 htype = 0;

            if (itv->type == OCI_INTERVAL_YM)
                htype = OCI_DTYPE_INTERVAL_YM;
            else if (itv->type == OCI_INTERVAL_DS)
                htype = OCI_DTYPE_INTERVAL_DS;

            if (itv->hstate != OCI_OBJECT_ALLOCATED_ARRAY)
            {
                res = (OCI_SUCCESS == OCI_DescriptorAlloc((dvoid  *) pOCILib->env,
                                                          (dvoid **) (void *) &itv->handle,
                                                          (ub4     ) htype, (size_t) 0,
                                                          (dvoid **) NULL));

                itv->hstate = OCI_OBJECT_ALLOCATED;
            }
        }
        else
            itv->hstate = OCI_OBJECT_FETCHED_CLEAN;
    }
    else
        res = FALSE;

    /* check for failure */

    if (res == FALSE)
    {
        OCI_IntervalFree(itv);
        itv = NULL;
    }

#else

    OCI_NOT_USED(con);
    OCI_NOT_USED(pitv);
    OCI_NOT_USED(type);
    OCI_NOT_USED(buffer);

#endif

    return itv;
}

/* ************************************************************************ *
 *                            PUBLIC FUNCTIONS
 * ************************************************************************ */

/* ------------------------------------------------------------------------ *
 * OCI_IntervalCreate
 * ------------------------------------------------------------------------ */

OCI_Interval * OCI_API OCI_IntervalCreate(OCI_Connection *con, unsigned int type)
{
    return OCI_IntervalCreate2(&OCILib, con, type);
}

OCI_Interval * OCI_API OCI_IntervalCreate2(OCI_Library *pOCILib, OCI_Connection *con, unsigned int type)
{
    OCI_Interval *itv = NULL;

    OCI_CHECK_INITIALIZED2(NULL);

    OCI_CHECK_INTERVAL_ENABLED(con, NULL);

#if OCI_VERSION_COMPILE >= OCI_9_0

    itv = OCI_IntervalInit(pOCILib, con, &itv, NULL, type);

#else

    OCI_NOT_USED(type);

#endif

    OCI_RESULT(itv != NULL);

    return itv;
}

/* ------------------------------------------------------------------------ *
 * OCI_IntervalFree
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_IntervalFree(OCI_Interval *itv)
{
    OCI_CHECK_PTR(OCI_IPC_INTERVAL, itv, FALSE);

    OCI_CHECK_INTERVAL_ENABLED(itv->con, FALSE);

#if OCI_VERSION_COMPILE >= OCI_9_0

    OCI_CHECK_OBJECT_FETCHED(itv, FALSE);

    if (itv->hstate == OCI_OBJECT_ALLOCATED)
    {
        ub4 htype = 0;

        if (itv->type == OCI_INTERVAL_YM)
            htype = OCI_DTYPE_INTERVAL_YM;
        else if (itv->type == OCI_INTERVAL_DS)
            htype = OCI_DTYPE_INTERVAL_DS;

        OCI_DescriptorFree((dvoid *) itv->handle, htype);
    }

    if (itv->hstate != OCI_OBJECT_ALLOCATED_ARRAY)
    {
        OCI_FREE(itv);
    }

#endif

   OCI_RESULT(TRUE);

   return TRUE;
}
/* ------------------------------------------------------------------------ *
 * OCI_IntervalAssign
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_IntervalAssign(OCI_Interval *itv,  OCI_Interval *itv_src)
{
    return OCI_IntervalAssign2(&OCILib, itv, itv_src);
}

boolean OCI_API OCI_IntervalAssign2(OCI_Library *pOCILib, OCI_Interval *itv,  OCI_Interval *itv_src)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(OCI_IPC_INTERVAL, itv,     FALSE);
    OCI_CHECK_PTR(OCI_IPC_INTERVAL, itv_src, FALSE);

    OCI_CHECK_INTERVAL_ENABLED(itv->con, FALSE);

#if OCI_VERSION_COMPILE >= OCI_9_0

    OCI_CALL4
    (
        res, itv->err, itv->con,

        OCIIntervalAssign((dvoid *) pOCILib->env, itv->err,
                          itv_src->handle, itv->handle)
    )

#endif

    OCI_RESULT(res);

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_IntervalGetDaySecond
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_IntervalGetDaySecond(OCI_Interval *itv, int *day, int *hour,
                                         int *min, int *sec, int *fsec)
{
    return OCI_IntervalGetDaySecond2(&OCILib, itv, day, hour, min, sec, fsec);
}

boolean OCI_API OCI_IntervalGetDaySecond2(OCI_Library *pOCILib, OCI_Interval *itv, int *day, int *hour,
                                         int *min, int *sec, int *fsec)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(OCI_IPC_INTERVAL, itv, FALSE);
    OCI_CHECK_PTR(OCI_IPC_INT, hour, FALSE);
    OCI_CHECK_PTR(OCI_IPC_INT, min , FALSE);
    OCI_CHECK_PTR(OCI_IPC_INT, sec,  FALSE);
    OCI_CHECK_PTR(OCI_IPC_INT, fsec, FALSE);

    OCI_CHECK_INTERVAL_ENABLED(itv->con, FALSE);

    *day  = 0;
    *hour = 0;
    *min  = 0;
    *sec  = 0;
    *fsec = 0;

#if OCI_VERSION_COMPILE >= OCI_9_0

    OCI_CALL4
    (
        res, itv->err, itv->con,

        OCIIntervalGetDaySecond((dvoid *) pOCILib->env, itv->err,
                                (sb4 *) day, (sb4 *) hour, (sb4 *) min,
                                (sb4 *) sec, (sb4 *) fsec, itv->handle)
    )

#else

    OCI_NOT_USED(day);
    OCI_NOT_USED(hour);
    OCI_NOT_USED(min);
    OCI_NOT_USED(sec);
    OCI_NOT_USED(fsec);

#endif

    OCI_RESULT(res);

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_IntervalGetYearMonth
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_IntervalGetYearMonth(OCI_Interval *itv, int *year, int *month)
{
    return OCI_IntervalGetYearMonth2(&OCILib, itv, year, month);
}

boolean OCI_API OCI_IntervalGetYearMonth2(OCI_Library *pOCILib, OCI_Interval *itv, int *year, int *month)
{
    boolean res = FALSE;

    OCI_CHECK_PTR(OCI_IPC_INTERVAL, itv, FALSE);
    OCI_CHECK_PTR(OCI_IPC_INT, year,  FALSE);
    OCI_CHECK_PTR(OCI_IPC_INT, month, FALSE);

    OCI_CHECK_INTERVAL_ENABLED(itv->con, FALSE);

    *year  = 0;
    *month = 0;

#if OCI_VERSION_COMPILE >= OCI_9_0

    OCI_CALL4
    (
        res, itv->err, itv->con,

        OCIIntervalGetYearMonth((dvoid *) pOCILib->env, itv->err,
                                (sb4 *) year, (sb4 *) month, itv->handle)
    )

#else

    OCI_NOT_USED(year);
    OCI_NOT_USED(month);

#endif

    OCI_RESULT(res);

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_IntervalSetDaySecond
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_IntervalSetDaySecond(OCI_Interval *itv, int day,int hour,
                                         int min, int sec, int fsec)
{
    return OCI_IntervalSetDaySecond2(&OCILib, itv, day, hour,
                                         min, sec, fsec);
}

boolean OCI_API OCI_IntervalSetDaySecond2(OCI_Library *pOCILib, OCI_Interval *itv, int day,int hour,
                                         int min, int sec, int fsec)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(OCI_IPC_INTERVAL, itv, FALSE);

    OCI_CHECK_INTERVAL_ENABLED(itv->con, FALSE);

#if OCI_VERSION_COMPILE >= OCI_9_0

    OCI_CALL4
    (
        res, itv->err, itv->con,

        OCIIntervalSetDaySecond((dvoid *) pOCILib->env, itv->err,
                                (sb4) day, (sb4) hour, (sb4) min,
                                (sb4) sec, (sb4) fsec, itv->handle)
    )

#else

    OCI_NOT_USED(day);
    OCI_NOT_USED(hour);
    OCI_NOT_USED(min);
    OCI_NOT_USED(sec);
    OCI_NOT_USED(fsec);

#endif

    OCI_RESULT(res);

    return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_IntervalSetYearMonth
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_IntervalSetYearMonth(OCI_Interval *itv, int year, int month)
{
    return OCI_IntervalSetYearMonth2(&OCILib, itv, year, month);
}

boolean OCI_API OCI_IntervalSetYearMonth2(OCI_Library *pOCILib, OCI_Interval *itv, int year, int month)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(OCI_IPC_INTERVAL, itv, FALSE);

    OCI_CHECK_INTERVAL_ENABLED(itv->con, FALSE);

#if OCI_VERSION_COMPILE >= OCI_9_0

    OCI_CALL4
    (
        res, itv->err, itv->con,

        OCIIntervalSetYearMonth((dvoid *) pOCILib->env, itv->err,
                                (sb4) year, (sb4) month, itv->handle)
    )

#else

    OCI_NOT_USED(year);
    OCI_NOT_USED(month);

#endif

    OCI_RESULT(res);

    return res;
}
