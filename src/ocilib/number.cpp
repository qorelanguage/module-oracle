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
 * $Id: number.c, v 3.7.0 2010-07-20 17:45 Vincent Rogier $
 * ------------------------------------------------------------------------ */

#include "ocilib_internal.h"

/* ************************************************************************ *
 *                             PRIVATE FUNCTIONS
 * ************************************************************************ */

/* ------------------------------------------------------------------------ *
 * OCI_NumberGet
 * ------------------------------------------------------------------------ */

boolean OCI_NumberGet2(OCI_Library *pOCILib, OCI_Connection *con,  OCINumber *data, void *value,
		       uword size, uword flag, ExceptionSink* xsink)
{
    boolean res = TRUE;

    OCI_CHECK(con   == NULL, FALSE);
    OCI_CHECK(value == NULL, FALSE);
    OCI_CHECK(data  == NULL, FALSE);
        
    if (flag & OCI_NUM_DOUBLE)
    {
        OCI_CALL2Q
        (
            pOCILib, res, con, 
            
            OCINumberToReal(con->err, data, size, value),

	    xsink
        )
    }
    else
    {
        uword sign = OCI_NUMBER_SIGNED;

        if (flag & OCI_NUM_UNSIGNED)
            sign = OCI_NUMBER_UNSIGNED;

        OCI_CALL2Q
        (
            pOCILib, res, con, 
            
            OCINumberToInt(con->err, data, size, sign, value),

	    xsink
        )
    }
            
   return res;
}

/* ------------------------------------------------------------------------ *
 * OCI_NumberSet
 * ------------------------------------------------------------------------ */

boolean OCI_NumberSet2(OCI_Library *pOCILib, OCI_Connection *con,  OCINumber *data, void *value, 
                      uword size, uword flag, ExceptionSink* xsink)
{
    boolean res = TRUE;

    OCI_CHECK(con   == NULL, FALSE);
    OCI_CHECK(value == NULL, FALSE);
    OCI_CHECK(data  == NULL, FALSE);
        
    if (flag & OCI_NUM_DOUBLE)
    {
        OCI_CALL2Q
        (
	   pOCILib, res, con, 
            
	   OCINumberFromReal(con->err, value, size, (OCINumber *) data),

	   xsink
        )
    }
    else
    {
        uword sign = OCI_NUMBER_SIGNED;

        if (flag & OCI_NUM_UNSIGNED)
            sign = OCI_NUMBER_UNSIGNED;

        OCI_CALL2Q
        (
            pOCILib, res, con, 
            
            OCINumberFromInt(con->err, value, size, sign, (OCINumber *) data),

	    xsink
        )
    }
            
    return res;
}


/* ------------------------------------------------------------------------ *
 * OCI_NumberConvertStr
 * ------------------------------------------------------------------------ */

boolean OCI_NumberConvertStr2(OCI_Library *pOCILib, OCI_Connection *con,  OCINumber *num, 
                            const dtext *str, int str_size, 
                            const mtext* fmt, ub4 fmt_size, ExceptionSink* xsink)
{
    boolean res = TRUE;
    void *ostr1 = NULL;
    int  osize1 = str_size;
    void *ostr2 = NULL;
    int  osize2 = fmt_size;

#ifdef OCI_CHARSET_MIXED

    mtext temp[OCI_SIZE_BUFFER + 1];
  
#endif

    OCI_CHECK(con   == NULL, FALSE);
    OCI_CHECK(str   == NULL, FALSE);
    OCI_CHECK(fmt   == NULL, FALSE);

#ifdef OCI_CHARSET_MIXED

    temp[0] = 0;

    ostr1  = temp;
    osize1 = (int) wcstombs(temp, str, OCI_SIZE_BUFFER + OCI_CVT_CHAR);

#else

    ostr1 = OCI_GetInputDataString(pOCILib, str, &osize1);

#endif

    ostr2 = OCI_GetInputMetaString(pOCILib, fmt, &osize2);


    memset(num, 0, sizeof(*num));

    OCI_CALL2Q
    (
       pOCILib, res, con, 
        
       OCINumberFromText(con->err, (oratext *) ostr1, (ub4) osize1,
			  (oratext *) ostr2, (ub4) osize2, 
			  (oratext *) NULL,  (ub4) 0, num),

       xsink
    )

#ifndef OCI_CHARSET_MIXED

    OCI_ReleaseDataString(ostr1);

#endif

    OCI_ReleaseMetaString(ostr2);      

    return res;
 }


/* ------------------------------------------------------------------------ *
 * OCI_NumberGetFromStr
 * ------------------------------------------------------------------------ */

boolean OCI_NumberGetFromStr2(OCI_Library *pOCILib, OCI_Connection *con,  void *value, uword size,
                             uword type, const dtext *str, int str_size, 
                             const mtext* fmt, ub4 fmt_size, ExceptionSink* xsink)
{
    OCINumber num;
  
    OCI_CHECK(value == NULL, FALSE);
  
    return (OCI_NumberConvertStr2(pOCILib, con, &num, str, str_size, fmt, fmt_size, xsink) &&
            OCI_NumberGet2(pOCILib, con, &num, value, size, type, xsink));
 }
