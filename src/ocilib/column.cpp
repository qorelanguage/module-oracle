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
 * $Id: column.c, v 3.7.0 2010-07-20 17:45 Vincent Rogier $
 * ------------------------------------------------------------------------ */

#include "ocilib_internal.h"

/* ************************************************************************ *
 *                             PRIVATE FUNCTIONS
 * ************************************************************************ */

/* ------------------------------------------------------------------------ *
 * OCI_ColumnDescribe
 * ------------------------------------------------------------------------ */
/*
boolean OCI_ColumnDescribe(OCI_Column *col, OCI_Connection *con, 
                           OCI_Statement *stmt, void *handle, int index,
                           int ptype)
{
}
*/

boolean OCI_ColumnDescribe2(OCI_Library *pOCILib, OCI_Column *col, OCI_Connection *con, 
			    OCI_Statement *stmt, void *handle, int index,
			    int ptype, ExceptionSink* xsink)
{
    void       *ostr     = NULL;
    void       *param    = NULL;
    boolean     res      = TRUE;
    int         osize    = 0;
    ub4         htype    = 0;
    ub4         attrname = 0;

    /* get descriptor */

    if (ptype == OCI_DESC_COLLECTION)
    {
        OCI_CALL1Q
        (
            pOCILib, res, con, stmt,
            
            OCIAttrGet((dvoid *) handle, (ub4) OCI_DTYPE_PARAM, (dvoid *) &param, 
                       (ub4 *) NULL, (ub4) OCI_ATTR_COLLECTION_ELEMENT, con->err),

	    xsink
        )

        attrname = OCI_ATTR_TYPE_NAME;
    }
    else
    {
        if (ptype == OCI_DESC_RESULTSET)
            htype = OCI_HTYPE_STMT;
        else
            htype = OCI_DTYPE_PARAM;

        OCI_CALL1Q
        (
            pOCILib, res, con, stmt,
            
            OCIParamGet((dvoid *) handle, htype,  con->err, (void**) &param, 
                        (ub4) index),

	    xsink
        )
    }

    /* sql code */

    OCI_CALL1Q
    (
        pOCILib, res, con, stmt,
        
        OCIAttrGet((dvoid *) param, (ub4) OCI_DTYPE_PARAM, (dvoid *) &col->ocode,
                   (ub4 *) NULL, (ub4) OCI_ATTR_DATA_TYPE, con->err),

	xsink
    )
 
   /* size */

    OCI_CALL1Q
    (
        pOCILib, res, con, stmt,
        
        OCIAttrGet((dvoid *) param, (ub4)  OCI_DTYPE_PARAM, (dvoid *) &col->size,
                   (ub4 *) NULL, (ub4) OCI_ATTR_DATA_SIZE, con->err),

	xsink
    )

    /* scale */

    OCI_CALL1Q
    (
        pOCILib, res, con, stmt,
        
        OCIAttrGet((dvoid *) param, (ub4) OCI_DTYPE_PARAM, (dvoid *) &col->scale,
                   (ub4 *) NULL, (ub4) OCI_ATTR_SCALE, con->err),

	xsink
    )

    /* precision */

    if (ptype == OCI_DESC_RESULTSET)
    {
        sb2 prec = 0;

        OCI_CALL1Q
        (
            pOCILib, res, con, stmt,
            
            OCIAttrGet((dvoid *) param, (ub4) OCI_DTYPE_PARAM, (dvoid *) &prec,
                       (ub4 *) NULL, (ub4) OCI_ATTR_PRECISION, con->err),

	    xsink
        )

        col->prec = (sb2) prec;
    }
    else
    {
        ub1 prec = 0;

        OCI_CALL1Q
        (
            pOCILib, res, con, stmt,
            
            OCIAttrGet((dvoid *) param, (ub4) OCI_DTYPE_PARAM, (dvoid *) &prec,
                       (ub4 *) NULL, (ub4) OCI_ATTR_PRECISION, con->err),

	    xsink
        )

        col->prec = (sb2) prec;
    }

    /* charset form */

    OCI_CALL1Q
    (
        pOCILib, res, con, stmt,
        
        OCIAttrGet((dvoid *) param, (ub4) OCI_DTYPE_PARAM, (dvoid *) &col->csfrm, 
                   (ub4 *) NULL, (ub4) OCI_ATTR_CHARSET_FORM, con->err),

	xsink
    )

    /* type of column length for string based column */

#if OCI_VERSION_COMPILE >= OCI_9_0 

    if ((pOCILib->version_runtime >= OCI_9_0) && (con->ver_num >= OCI_9_0))
    {
        /* char used - no error checking because on Oracle 9.0, querying
                       this param that is not char/varchar based will cause an 
                       error */

        OCI_CALL1Q
        (
            pOCILib, res, con, stmt,
            
            OCIAttrGet((dvoid *) param, (ub4) OCI_DTYPE_PARAM, 
                       (dvoid *) &col->charused, (ub4 *) NULL, 
                       (ub4) OCI_ATTR_CHAR_USED, con->err),
	    
	    xsink
        )
    }

    /* char size */
    
    if (col->charused == TRUE)
    {
        OCI_CALL1Q
        (
            pOCILib, res, con, stmt,
        
            OCIAttrGet((dvoid *) param, (ub4) OCI_DTYPE_PARAM, 
                       (dvoid *) &col->charsize, (ub4 *) NULL, 
                       (ub4) OCI_ATTR_CHAR_SIZE, con->err),

	    xsink
        )
    }

    if ((pOCILib->version_runtime >= OCI_9_0) && (con->ver_num >= OCI_9_0))
    {
        /* fractional time precision for timestamps */

         if (col->ocode == SQLT_TIMESTAMP    ||
             col->ocode == SQLT_TIMESTAMP_TZ ||
             col->ocode == SQLT_TIMESTAMP_LTZ)
        {
            OCI_CALL1Q
            (
                pOCILib, res, con, stmt,
        
                OCIAttrGet((dvoid *) param, (ub4) OCI_DTYPE_PARAM,
                           (dvoid *) &col->prec, (ub4 *) NULL, 
                           (ub4) OCI_ATTR_FSPRECISION, con->err),

		xsink
            )
        }

        /* leading and fractional precision for interval */

        if (col->ocode == SQLT_INTERVAL_DS ||
            col->ocode == SQLT_INTERVAL_YM)
        {
            OCI_CALL1Q
            (
                pOCILib, res, con, stmt,
        
                OCIAttrGet((dvoid *) param, (ub4) OCI_DTYPE_PARAM, 
                           (dvoid *) &col->prec, (ub4 *) NULL, 
                           (ub4) OCI_ATTR_LFPRECISION, con->err),

		xsink
            )

            OCI_CALL1Q
            (
                pOCILib, res, con, stmt,
        
                OCIAttrGet((dvoid *) param, (ub4) OCI_DTYPE_PARAM, 
                           (dvoid *) &col->prec2, (ub4 *) NULL, 
                           (ub4) OCI_ATTR_FSPRECISION, con->err),

		xsink
            )
        }
    }

#endif

    /* check nullable only for table based column */

    if (ptype == OCI_DESC_TABLE)
    {
        OCI_CALL1Q
        (
            pOCILib, res, con, stmt,
        
            OCIAttrGet((dvoid *) param, (ub4) OCI_DTYPE_PARAM, 
                       (dvoid *) &col->null, (ub4 *) NULL, 
                       (ub4) OCI_ATTR_IS_NULL, con->err),

	    xsink
        )
    }
    else
        col->null = TRUE;


    /* name */

    if (ptype == OCI_DESC_COLLECTION) 
        attrname = OCI_ATTR_TYPE_NAME;
    else
        attrname = OCI_ATTR_NAME;

    /* name */

    if (ptype == OCI_DESC_COLLECTION) 
        attrname = OCI_ATTR_TYPE_NAME;
    else
        attrname = OCI_ATTR_NAME;

    OCI_CALL1Q
    (
        pOCILib, res, con, stmt,
        
        OCIAttrGet((dvoid *) param, (ub4) OCI_DTYPE_PARAM, (dvoid *) &ostr,
                   (ub4 *) &osize, (ub4) attrname, con->err),

	xsink
    )

    if ((res == TRUE) && (ostr != NULL))
    {
        col->name = (mtext *) OCI_MemAlloc2(pOCILib, OCI_IPC_STRING, sizeof(mtext),
                                           (size_t) ((osize / (int) sizeof(omtext)) + 1),
                                           TRUE);
  
        if (col->name != NULL)
        {
            OCI_CopyString(ostr, col->name, &osize, 
                           sizeof(omtext), sizeof(mtext));
        }
        else
            res = FALSE;
    }

    /* user type descriptor */

    if (col->ocode == SQLT_NTY || col->ocode == SQLT_REF)
    {
        OCI_CALL1Q
        (
            pOCILib, res, con, stmt,
        
            OCIAttrGet((dvoid *) param, (ub4) OCI_DTYPE_PARAM, 
                       (dvoid *) &ostr, (ub4 *) &osize, 
                       (ub4) OCI_ATTR_TYPE_NAME, con->err),

	    xsink
        )
      
        if ((res == TRUE) && (ostr != NULL))
        {   
            mtext *tmp = (mtext *) OCI_MemAlloc2(pOCILib, OCI_IPC_STRING, sizeof(mtext),
                                                (size_t)((osize / (int) sizeof(omtext)) + 1),
                                                TRUE);
        
            if (tmp != NULL)
            {
               OCI_CopyString(ostr, tmp, &osize, sizeof(omtext), sizeof(mtext));
               col->typinf = OCI_TypeInfoGet2(pOCILib, con, tmp, OCI_SCHEMA_TYPE, xsink);
            }

            res = (col->typinf != NULL);
            
            OCI_FREE(tmp);
        }
    }

    if (param != NULL)
    {
        res = (OCI_SUCCESS == OCIDescriptorFree(param, OCI_DTYPE_PARAM));
    }

    return res;
}
    
/* ------------------------------------------------------------------------ *
 * OCI_ColumnMap
 * ------------------------------------------------------------------------ */
/*
boolean OCI_ColumnMap(OCI_Column *col, OCI_Statement *stmt)
{
    return OCI_ColumnMap2(&OCILib, col, stmt);
}
*/

boolean OCI_ColumnMap2(OCI_Library *pOCILib, OCI_Column *col, OCI_Statement *stmt)
{
    boolean res = TRUE;
    
    OCI_CHECK(col == NULL, FALSE);

    /* map Oracle SQL code to OCILIB types and setup of internal buffer size */

    col->icode = col->ocode;

    switch (col->icode)
    {
        case SQLT_INT:

            col->type = OCI_CDT_NUMERIC;
            
            /* set bufsize only if it's not a "returning into" placeholder */
         
            if (col->bufsize == 0) 
            {
                col->subtype = OCI_NUM_INT;
                col->bufsize = sizeof(int);
            }

            break;

       case SQLT_UIN:

            col->type = OCI_CDT_NUMERIC;
           
            /* set bufsize only if it's not a "returning into" placeholder */

            if (col->bufsize == 0) 
            {
                col->subtype = OCI_NUM_UINT;
                col->bufsize = sizeof(unsigned int);
            }

            break;

        case SQLT_FLT:
        case SQLT_VNU:
        case SQLT_PDN:
        case SQLT_NUM:

#if OCI_VERSION_COMPILE >= OCI_10_1

        case SQLT_BFLOAT:
        case SQLT_BDOUBLE:
        case SQLT_IBFLOAT:
        case SQLT_IBDOUBLE:

#endif  
            col->type    = OCI_CDT_NUMERIC;
            col->subtype = OCI_NUM_NUMBER;
            col->icode   = SQLT_VNU;
            col->bufsize = sizeof(OCINumber);

            break;

        case SQLT_DAT:
        case SQLT_ODT:

            col->type    = OCI_CDT_DATETIME;

            /* We map to SQLT_ODT only it the column is not part of a 
               "returning into" clause (workaround for Oracle 
               known bug #3269146 
            */

            if (col->bufsize == 0)
            {
                col->icode   = SQLT_ODT;
                col->bufsize = sizeof(OCIDate);
            }

            break;

        case SQLT_CUR:
        case SQLT_RSET:

            col->type     = OCI_CDT_CURSOR;
            col->bufsize  = sizeof(OCIStmt *);
            col->dtype    = OCI_HTYPE_STMT;
            break;

        case SQLT_RID:
        case SQLT_RDD:

            col->icode   = SQLT_STR;
            col->type    = OCI_CDT_TEXT;

            if ((col->ocode == SQLT_RDD) || (col->size > sizeof(OCIRowid *)))
            {
                /* For Oracle 7 ROWIDs and regular ROWID descriptors, the 
                   max size of the hex value is defined by the constant
                   OCI_SIZE_ROWID 
                */

                col->bufsize = (OCI_SIZE_ROWID + 1) * (ub4) sizeof(dtext);
            }
            else
            {
                /* For ROWID descriptor, if column size is bigger than the size
                    of the descriptor, it means that an UROWID column and then 
                    the column size is the maximum size needed for representing
                    its value as an hex string
                */

                col->bufsize = (ub4) ((col->size + 1) * (ub2) sizeof(dtext));
            }

            break;
 
        case SQLT_BIN:

            col->type    = OCI_CDT_RAW;
            col->bufsize = (ub4) (col->size + (ub2) sizeof(dtext)); /* for string conversion */
            break;

        case SQLT_BLOB:

            col->type    = OCI_CDT_LOB;
            col->subtype = OCI_BLOB;
            col->dtype   = OCI_DTYPE_LOB;
            col->bufsize = (ub4) sizeof(OCILobLocator *);
            break;

        case SQLT_CLOB:

            col->type    = OCI_CDT_LOB;
            col->dtype   = OCI_DTYPE_LOB;
            col->bufsize = (ub4) sizeof(OCILobLocator *);
           
            if (col->csfrm == SQLCS_NCHAR)
                col->subtype = OCI_NCLOB;
            else
                col->subtype = OCI_CLOB;

            break;

        case SQLT_BFILE:

            col->type    = OCI_CDT_FILE;
            col->subtype = OCI_BFILE;
            col->dtype   = OCI_DTYPE_LOB;
            col->bufsize = (ub4) sizeof(OCILobLocator *);
            break;

        case SQLT_CFILE:

            col->type    = OCI_CDT_FILE;
            col->subtype = OCI_CFILE;
            col->bufsize = (ub4) sizeof(OCILobLocator *);
            col->dtype   = OCI_DTYPE_LOB;
            break;

        case SQLT_LNG:
        case SQLT_LVC:
        case SQLT_LBI:
        case SQLT_LVB:
        case SQLT_VBI:

            if ((col->icode == SQLT_LNG || col->icode == SQLT_LVC) &&
                (stmt != NULL && stmt->long_mode == OCI_LONG_IMPLICIT))
            {
                col->type     = OCI_CDT_TEXT;
                col->bufsize  = (OCI_SIZE_LONG+1) * ((ub2) sizeof(dtext));
                col->subtype  = OCI_CLONG;

                if (pOCILib->nls_utf8 == TRUE)
                {
                    col->bufsize *= UTF8_BYTES_PER_CHAR;
                }
            }
            else
            {
                col->type    = OCI_CDT_LONG;
                col->bufsize = INT_MAX;
                
                if (col->icode == SQLT_LBI ||
                    col->icode == SQLT_LVB ||
                    col->icode == SQLT_VBI)
                {
                    col->subtype = OCI_BLONG;
                }
                else
                {
                    col->subtype = OCI_CLONG;
                }

            }

            break;

#if OCI_VERSION_COMPILE >= OCI_9_0

        case SQLT_TIMESTAMP:

            col->type    = OCI_CDT_TIMESTAMP;
            col->subtype = OCI_TIMESTAMP;
            col->dtype   = OCI_DTYPE_TIMESTAMP;
            col->bufsize = (ub4) sizeof(OCIDateTime *);
            break;

        case SQLT_TIMESTAMP_TZ:

            col->type    = OCI_CDT_TIMESTAMP;
            col->subtype = OCI_TIMESTAMP_TZ;
            col->dtype   = OCI_DTYPE_TIMESTAMP_TZ;
            col->bufsize = (ub4) sizeof(OCIDateTime *);
            break;

        case SQLT_TIMESTAMP_LTZ:

            col->type    = OCI_CDT_TIMESTAMP;
            col->subtype = OCI_TIMESTAMP_LTZ;
            col->dtype   = OCI_DTYPE_TIMESTAMP_LTZ;
            col->bufsize = (ub4) sizeof(OCIDateTime *);
            break;

        case SQLT_INTERVAL_YM:

            col->type    = OCI_CDT_INTERVAL;
            col->subtype = OCI_INTERVAL_YM;
            col->dtype   = OCI_DTYPE_INTERVAL_YM;
            col->bufsize = (ub4) sizeof(OCIInterval *);
            break;

        case SQLT_INTERVAL_DS:

            col->type    = OCI_CDT_INTERVAL;
            col->subtype = OCI_INTERVAL_DS;
            col->dtype   = OCI_DTYPE_INTERVAL_DS;
            col->bufsize = (ub4) sizeof(OCIInterval *);
            break;

#endif

#if OCI_VERSION_COMPILE >= OCI_9_0

        case SQLT_PNTY:

#endif

        case SQLT_NTY:

            col->icode   = SQLT_NTY;
            col->bufsize = (ub4) sizeof(void *);

            if (col->typinf->tcode == SQLT_NCO)
                col->type = OCI_CDT_COLLECTION;                    
            else
                col->type = OCI_CDT_OBJECT;                    

            break;

        case SQLT_REF:

            col->icode   = SQLT_REF;
            col->bufsize = (ub4) sizeof(OCIRef *);
            col->type    = OCI_CDT_REF;                    

            break;

        case SQLT_CHR:
        case SQLT_STR:
        case SQLT_VCS:
        case SQLT_AFC:
        case SQLT_AVC:
        case SQLT_VST:
        case SQLT_LAB:
        case SQLT_OSL:
        case SQLT_SLS:
        default:

            col->icode    = SQLT_STR;
            col->type     = OCI_CDT_TEXT;
            col->bufsize  = (ub4) ((col->size + 1) * (ub2) sizeof(dtext));

            if (pOCILib->nls_utf8 == TRUE)
            {
                col->bufsize *= UTF8_BYTES_PER_CHAR;
            }

            break;
    }

    return res;
}

/* ************************************************************************ *
 *                            PUBLIC FUNCTIONS
 * ************************************************************************ */

/* ------------------------------------------------------------------------ *
 * OCI_ColumnGetName
 * ------------------------------------------------------------------------ */

const mtext * OCI_API OCI_ColumnGetName2(OCI_Library *pOCILib, OCI_Column *col, ExceptionSink* xsink)
{
   OCI_CHECK_PTRQ(pOCILib, OCI_IPC_COLUMN, col, NULL, xsink);

   return col->name;
}

/* ------------------------------------------------------------------------ *
 * OCI_ColumnGetType
 * ------------------------------------------------------------------------ */

unsigned int OCI_API OCI_ColumnGetType2(OCI_Library *pOCILib, OCI_Column *col)
{
    OCI_CHECK_PTR(pOCILib, OCI_IPC_COLUMN, col, OCI_UNKNOWN);

    OCI_RESULT(pOCILib, TRUE);

    return col->type;
}

/* ------------------------------------------------------------------------ *
 * OCI_ColumnGetCharsetForm
 * ------------------------------------------------------------------------ */

unsigned int OCI_API OCI_ColumnGetCharsetForm2(OCI_Library *pOCILib, OCI_Column *col)
{
    OCI_CHECK_PTR(pOCILib, OCI_IPC_COLUMN, col, OCI_CSF_NONE);

    OCI_RESULT(pOCILib, TRUE);

    if (col->csfrm == SQLCS_NCHAR)
        return OCI_CSF_NATIONAL;
    else if (col->csfrm == SQLCS_IMPLICIT)
        return OCI_CSF_CHARSET;
    else
        return OCI_CSF_NONE;
}

/* ------------------------------------------------------------------------ *
 * OCI_ColumnGetSize
 * ------------------------------------------------------------------------ */

unsigned int OCI_API OCI_ColumnGetSize2(OCI_Library *pOCILib, OCI_Column *col)
{
    OCI_CHECK_PTR(pOCILib, OCI_IPC_COLUMN, col, 0);

    OCI_RESULT(pOCILib, TRUE);

    /* Oracle 9i introduced CHAR attribute on string columns to indicate the 
       size of the column is not in bytes (default) but in chars
       OCI_ColumnDescribe() already managed the Oracle compatibly
       version, so if col->charsize is zero it means :
       - the column is not a string column 
       - the size is not in char
       - client does not support the OCI_ATTR_CHAR_SIZE attribute */
       
    if (col->charused == TRUE && col->charsize > 0)
        return col->charsize;
    else
        return col->size;
}

/* ------------------------------------------------------------------------ *
 * OCI_ColumnGetScale
 * ------------------------------------------------------------------------ */

int OCI_API OCI_ColumnGetScale2(OCI_Library *pOCILib, OCI_Column *col)
{
    OCI_CHECK_PTR(pOCILib, OCI_IPC_COLUMN, col, 0);

    OCI_RESULT(pOCILib, TRUE);

    return (int) col->scale;
}

/* ------------------------------------------------------------------------ *
 * OCI_ColumnGetPrecision
 * ------------------------------------------------------------------------ */

int OCI_API OCI_ColumnGetPrecision2(OCI_Library *pOCILib, OCI_Column *col)
{
    OCI_CHECK_PTR(pOCILib, OCI_IPC_COLUMN, col, 0);

    OCI_RESULT(pOCILib, TRUE);

    if (col->type == OCI_CDT_NUMERIC)
        return (int) col->prec;
    else
        return 0;
}

/* ------------------------------------------------------------------------ *
 * OCI_ColumnGetFractionalPrecision
 * ------------------------------------------------------------------------ */

int OCI_API OCI_ColumnGetFractionalPrecision2(OCI_Library *pOCILib, OCI_Column *col)
{
    OCI_CHECK_PTR(pOCILib, OCI_IPC_COLUMN, col, 0);

    OCI_RESULT(pOCILib, TRUE);

    if (col->type == OCI_CDT_TIMESTAMP)
        return (int) col->prec;
    else if (col->type == OCI_CDT_INTERVAL)
        return (int) col->prec2;
    else
        return 0;
}

/* ------------------------------------------------------------------------ *
 * OCI_ColumnGetLeadingPrecision
 * ------------------------------------------------------------------------ */

int OCI_API OCI_ColumnGetLeadingPrecision2(OCI_Library *pOCILib, OCI_Column *col)
{
    OCI_CHECK_PTR(pOCILib, OCI_IPC_COLUMN, col, 0);

    OCI_RESULT(pOCILib, TRUE);

    if (col->type == OCI_CDT_INTERVAL)
        return (int) col->prec;
    else
        return 0;
}

/* ------------------------------------------------------------------------ *
 * OCI_ColumnGetNullable
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_ColumnGetNullable2(OCI_Library *pOCILib, OCI_Column *col)
{
    OCI_CHECK_PTR(pOCILib, OCI_IPC_COLUMN, col, FALSE);

    OCI_RESULT(pOCILib, TRUE);

    return (col->null == TRUE);
}

/* ------------------------------------------------------------------------ *
 * OCI_ColumnGetCharUsed
 * ------------------------------------------------------------------------ */

boolean OCI_API OCI_ColumnGetCharUsed2(OCI_Library *pOCILib, OCI_Column *col)
{
    OCI_CHECK_PTR(pOCILib, OCI_IPC_COLUMN, col, FALSE);

    OCI_RESULT(pOCILib, TRUE);

    return (boolean) col->charused;
}

/* ------------------------------------------------------------------------ *
 * OCI_ColumnGetSQLType
 * ------------------------------------------------------------------------ */

const mtext * OCI_API OCI_ColumnGetSQLType2(OCI_Library *pOCILib, OCI_Column *col)
{
    OCI_CHECK_PTR(pOCILib, OCI_IPC_COLUMN, col, NULL);

    /* VARCHAR type will not be returned because Oracle does not make any 
       difference with VARCHAR2. If a column is created with VARCHAR, it is
       internally created as VARCHAR2
    */       

    OCI_RESULT(pOCILib, TRUE);

    switch(col->ocode)
    {
        case SQLT_AFC:

            if (col->csfrm == SQLCS_NCHAR)
                return MT("NCHAR");
            else
                return MT("CHAR");

        case SQLT_AVC:
        case SQLT_STR:
        case SQLT_CHR:

            if (col->csfrm == SQLCS_NCHAR)
                return MT("NVARCHAR2");
            else
                return MT("VARCHAR2");

        case SQLT_NUM:

            if (col->scale == -127 && col->prec > 0)
                 return MT("FLOAT");
             else
                 return MT("NUMBER");

        case SQLT_INT:

             return MT("INTEGER");

        case SQLT_FLT:

             return MT("FLOAT");

#if OCI_VERSION_COMPILE >= OCI_10_1

        case SQLT_BFLOAT:
        case SQLT_IBFLOAT:

             return MT("BINARY FLOAT");

        case SQLT_BDOUBLE:
        case SQLT_IBDOUBLE:

             return MT("BINARY DOUBLE");

 #endif

        case SQLT_LNG:

             return MT("LONG");

        case SQLT_DAT:
        case SQLT_ODT:
        case SQLT_DATE:

            return MT("DATE");

        case SQLT_RDD:
        case SQLT_RID:

            return MT("ROWID");

        case SQLT_BIN:

            return MT("RAW");

        case SQLT_LBI:

            return MT("LONG RAW");

        case SQLT_RSET:

            return MT("RESULTSET");

        case SQLT_CUR:

            return MT("CURSOR");

        case SQLT_CLOB:

            if (col->subtype == OCI_NCLOB)
                return MT("NCLOB");
            else
                return MT("CLOB");

        case SQLT_BLOB:

            return MT("BLOB");

        case SQLT_BFILE:

            return MT("BINARY FILE LOB");

        case SQLT_CFILE:

            return MT("CFILE");

#if OCI_VERSION_COMPILE >= OCI_9_0

        case SQLT_TIMESTAMP:

            return MT("TIMESTAMP");

        case SQLT_TIMESTAMP_TZ:

            return MT("TIMESTAMP WITH TIME ZONE");

        case SQLT_TIMESTAMP_LTZ:

            return MT("TIMESTAMP WITH LOCAL TIME ZONE");

        case SQLT_INTERVAL_YM:

            return MT("INTERVAL YEAR TO MONTH");

        case SQLT_INTERVAL_DS:

            return MT("INTERVAL DAY TO SECOND");

#endif

        case SQLT_REF:

            return MT("REF");

#if OCI_VERSION_COMPILE >= OCI_9_0

        case SQLT_PNTY:
#endif

        case SQLT_NTY:

            if (col->typinf != NULL)
                return col->typinf->name;
            else
                return MT("NAMED TYPE");

       default:

          /* unknown datatype ? Should not happen because all
               datatypes are supported */

            return MT("?");
    }
}


/* ------------------------------------------------------------------------ *
 * OCI_ColumnGetFullSQLType
 * ------------------------------------------------------------------------ */

unsigned int OCI_API OCI_ColumnGetFullSQLType2(OCI_Library *pOCILib, OCI_Column *col, mtext *buffer, 
                                              unsigned int len)
{
    OCI_CHECK_PTR(pOCILib, OCI_IPC_COLUMN, col, 0);
    OCI_CHECK_PTR(pOCILib, OCI_IPC_STRING, buffer, 0);

    OCI_RESULT(pOCILib, TRUE);

    buffer[0] = 0;

    /* ISO C functions are supposed to be "standard", but we still see specific
       implementations that make some usage not portable and worse not compatible.
       MS Windows is implementing string conversion characters (%s/%ls) of the 
       printf/wprintf family differently from unixes !
    */

    /* This function returns the same strings as Sql*Plus DESC command */

    switch(col->ocode)
    {
        case SQLT_AFC:

#if defined(OCI_METADATA_WIDE) && !defined(_WINDOWS)
            len = mtsprintf(buffer, len, MT("%lsCHAR(%i%ls)"), 
#else
            len = mtsprintf(buffer, len, MT("%sCHAR(%i%s)"), 
#endif
                            col->csfrm    == SQLCS_NCHAR ? MT("N") : MT(""),
                            (int) (col->charused == TRUE ? col->charsize : col->size),
                            col->charused == TRUE &&
                            col->csfrm    != SQLCS_NCHAR ? MT(" CHAR") : MT(""));
            break;
 
        case SQLT_AVC:
        case SQLT_STR:
        case SQLT_CHR:
 
#if defined(OCI_METADATA_WIDE) && !defined(_WINDOWS)
            len = mtsprintf(buffer, len, MT("%lsVARCHAR(%i%ls)"), 
#else
            len = mtsprintf(buffer, len, MT("%sVARCHAR(%i%s)"), 
#endif
                            col->csfrm    == SQLCS_NCHAR ? MT("N") : MT(""),
                            (int) (col->charused == TRUE ? col->charsize : col->size),
                            col->charused == TRUE &&
                            col->csfrm    != SQLCS_NCHAR ? MT(" CHAR") : MT(""));
            break;

        case SQLT_NUM:
 
            if (col->scale == -127 && col->prec > 0)
                len = mtsprintf(buffer, len,  MT("FLOAT(%i)"), col->prec);
            else if (col->scale > 0 && col->prec > 0)
                len = mtsprintf(buffer, len,  MT("NUMBER(%i,%i)"), 
                                (int) col->prec, (int) col->scale);
            else if (col->prec > 0)
                len = mtsprintf(buffer, len,  MT("NUMBER(%i)"), (int) col->prec);
            else
                len = mtsprintf(buffer, len,  MT("NUMBER"));
        
            break;
 
        case SQLT_INT:

            len = mtsprintf(buffer, len,  MT("NUMBER"));
            break;

        case SQLT_FLT:

            len = mtsprintf(buffer, len,  MT("FLOAT(%i)"), (int) col->prec);
            break;

#if OCI_VERSION_COMPILE >= OCI_10_1

        case SQLT_BFLOAT:
        case SQLT_IBFLOAT:

            len = mtsprintf(buffer, len,  MT("BINARY FLOAT"));
            break;

        case SQLT_BDOUBLE:
        case SQLT_IBDOUBLE:

             len = mtsprintf(buffer, len,  MT("BINARY DOUBLE"));
            break;

#endif

        case SQLT_LNG:

            len = mtsprintf(buffer, len, MT("LONG"));
            break;

        case SQLT_DAT:
        case SQLT_ODT:
        case SQLT_DATE:

            len = mtsprintf(buffer, len, MT("DATE"));
            break;

        case SQLT_RDD:
        case SQLT_RID:

            len = mtsprintf(buffer, len,  MT("ROWID"));
            break;

        case SQLT_BIN:
            len = mtsprintf(buffer, len, MT("RAW(%i)"), (int) col->size);
            break;

        case SQLT_LBI:

            len = mtsprintf(buffer, len, MT("LONG RAW(%i)"), (int) col->size);
            break;

        case SQLT_RSET:

            len = mtsprintf(buffer, len,  MT("RESULTSET"));
            break;

        case SQLT_CUR:

            len = mtsprintf(buffer, len,  MT("CURSOR"));
            break;

        case SQLT_CLOB:

            if (col->subtype == OCI_NCLOB)
                len = mtsprintf(buffer, len,  MT("NCLOB"));
            else
                len = mtsprintf(buffer, len,  MT("CLOB"));
            break;

        case SQLT_BLOB:

            len = mtsprintf(buffer, len,  MT("BLOB"));
            break;

        case SQLT_BFILE:

            len = mtsprintf(buffer, len,  MT("BINARY FILE LOB"));
            break;
 
        case SQLT_CFILE:
 
            len = mtsprintf(buffer, len,  MT("CFILE"));
            break;
 
#if OCI_VERSION_COMPILE >= OCI_9_0

        case SQLT_TIMESTAMP:

            len = mtsprintf(buffer, len,  MT("TIMESTAMP(%i)"), (int) col->prec);
            break;
 
        case SQLT_TIMESTAMP_TZ:
 
            len = mtsprintf(buffer, len,  MT("TIMESTAMP(%i) WITH TIME ZONE"),
                            (int) col->prec);
            break;
 
        case SQLT_TIMESTAMP_LTZ:
 
            len = mtsprintf(buffer, len,  MT("TIMESTAMP(%i) WITH LOCAL TIME ZONE"),
                            (int) col->prec);
            break;
 
        case SQLT_INTERVAL_YM:
 
           len = mtsprintf(buffer, len,  MT("INTERVAL(%i) YEAR TO MONTH(%i)"),
                            (int) col->prec, (int) col->prec2);
            break;
 
       case SQLT_INTERVAL_DS:
 
            len = mtsprintf(buffer, len,  MT("INTERVAL(%i) DAY TO SECOND(%i)"),
                            (int) col->prec, (int) col->prec2);
            break;

#endif
       
       case SQLT_REF:

            len = mtsprintf(buffer, len,  MT("REF"));
            break;

#if OCI_VERSION_COMPILE >= OCI_9_0

        case SQLT_PNTY:
#endif

        case SQLT_NTY:
 
            if (col->typinf != NULL)
                len = mtsprintf(buffer, len, col->typinf->name);
            else
                len = mtsprintf(buffer, len, MT("NAMED TYPE"));
            break;

        default:

            mtsncat(buffer, MT("?"), (size_t) len);
    }

    return len;
}
    
/* ------------------------------------------------------------------------ *
 * OCI_ColumnGetTypeInfo
 * ------------------------------------------------------------------------ */

OCI_TypeInfo * OCI_API OCI_ColumnGetTypeInfo2(OCI_Library *pOCILib, OCI_Column *col)
{
    OCI_CHECK_PTR(pOCILib, OCI_IPC_COLUMN, col, FALSE);

    OCI_RESULT(pOCILib, TRUE);

    return col->typinf;
}

/* ------------------------------------------------------------------------ *
 * OCI_ColumnGetSubType
 * ------------------------------------------------------------------------ */

unsigned int OCI_API OCI_ColumnGetSubType2(OCI_Library *pOCILib, OCI_Column *col)
{
    unsigned int type = OCI_UNKNOWN;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_COLUMN, col, OCI_UNKNOWN);

    OCI_RESULT(pOCILib, TRUE);

    if (col->type == OCI_CDT_LONG      || 
        col->type == OCI_CDT_LOB       ||
        col->type == OCI_CDT_FILE      ||
        col->type == OCI_CDT_TIMESTAMP ||
        col->type == OCI_CDT_INTERVAL)
    {
        type = col->subtype;
    }

    return type;
}
