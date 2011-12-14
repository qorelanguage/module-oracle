/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  OraResultSet.cpp

  Qore Programming Language

  Copyright (C) 2003 - 2011 David Nichols

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "oracle.h"

static int get_char_width(const QoreEncoding *enc, int num) {
#if QORE_VERSION_CODE >= 7001
   return num * enc->getMaxCharWidth();
#else
   // qore < 0.7.1 did not have the QoreEncoding::getMaxCharWidth() call, so we assume character width = 1
   // for all character encodings except UTF8
   return num * (enc == QCS_UTF8 ? 4 : 1);
#endif
}

OraResultSet::OraResultSet(QoreOracleStatement &n_stmt, const char *str, ExceptionSink *xsink) : stmt(n_stmt), defined(false) {
   QORE_TRACE("OraResultSet::OraResultSet()");

   QoreOracleConnection *conn = stmt.getData();

   // retrieve results, if any
   OCIParam *parmp;
   //void *parmp;

   // get columns in output
   while (!stmt.paramGet(parmp, clist.size() + 1)) {
      ub2 dtype;
      text *col_name;
      ub4 col_name_len;
      ub2 col_max_size;
      
      // get column type
      if (stmt.attrGet(parmp, &dtype, OCI_ATTR_DATA_TYPE, xsink))
         return;

      // get column name
      if (stmt.attrGet(parmp, &col_name, col_name_len, OCI_ATTR_NAME, xsink))
         return;

      ub2 col_char_len;
      if (stmt.attrGet(parmp, &col_char_len, OCI_ATTR_CHAR_SIZE, xsink))
         return;

      if (stmt.attrGet(parmp, &col_max_size, OCI_ATTR_DATA_SIZE, xsink))
         return;

      //printd(0, "OraResultSet::OraResultSet() column %s: type=%d char_len=%d size=%d (SQLT_STR=%d)\n", col_name, dtype, col_char_len, col_max_size, SQLT_STR);
      if (dtype == SQLT_NTY) {
          char *tname; // type name
          char *sname; // schema name

          if (stmt.attrGet(parmp, &sname, OCI_ATTR_SCHEMA_NAME, xsink))
             return;

          if (stmt.attrGet(parmp, &tname, OCI_ATTR_TYPE_NAME, xsink))
             return;

          // printd(0, "OraResultSet::OraResultSet() SQLT_NTY type=%s.%s\n", sname, tname);
          QoreString s(stmt.getEncoding());
          s.concat(sname);
          s.concat(".");
          s.concat(tname);

          OCI_TypeInfo * info = OCI_TypeInfoGet2(&conn->ocilib, conn->ocilib_cn, s.getBuffer(), OCI_TIF_TYPE);

          //printd(0, "OraResultSet::OraResultSet() ccode %d\n", info->ccode);
          // This is some kind of black magic - I'm not sure if it's sufficient
          // object/collection resolution method.
          int dsubtype = info->ccode ? SQLT_NTY_COLLECTION : SQLT_NTY_OBJECT;
          add((char *)col_name, col_name_len, col_max_size, dtype, col_char_len, dsubtype, s);
          continue;
      }
      if (dtype == SQLT_NCO) {
          // WTF is the NAMED COLLECTION in this case?! What's different from SQLT_NTY?
          printd(0, "OraResultSet::OraResultSet() SQLT_NCO - something is wrong. But what is SQLT_NCO???\n");
          assert(0);
      }

      add((char *)col_name, col_name_len, col_max_size, dtype, col_char_len);
   }
}

int OraResultSet::define(const char *str, ExceptionSink *xsink) {
   //QORE_TRACE("OraResultSet::define()");
   //    printd(0, "OraResultSet::define()\n");

   if (defined)
      return 0;

   defined = true;

   QoreOracleConnection *conn = stmt.getData();

   // iterate column list
   for (unsigned i = 0; i < clist.size(); ++i) {
      OraColumnBuffer *w = clist[i];
      //printd(5, "OraResultSet::define() this=%p %s: w->dtype=%d\n", this, w->name.getBuffer(), w->dtype);
      switch (w->dtype) {
	 case SQLT_INT:
	 case SQLT_UIN:
	    w->buf.i8 = 0;
            stmt.defineByPos(w->defp, i + 1, &w->buf.i8, sizeof(int64), SQLT_INT, &w->ind, xsink);
	    break;

	 case SQLT_FLT:
#ifdef SQLT_BFLOAT
	 case SQLT_BFLOAT:
#endif
#ifdef SQLT_BDOUBLE
	 case SQLT_BDOUBLE:
#endif
#ifdef SQLT_IBFLOAT
	 case SQLT_IBFLOAT:
#endif
#ifdef SQLT_IBDOUBLE
	 case SQLT_IBDOUBLE:
#endif
#if defined(SQLT_BDOUBLE) && defined(USE_NEW_NUMERIC_TYPES)
	    stmt.defineByPos(w->defp, i + 1, &w->buf.f8, sizeof(double), SQLT_BDOUBLE, &w->ind, xsink);
#else
	    stmt.defineByPos(w->defp, i + 1, &w->buf.f8, sizeof(double), SQLT_FLT, &w->ind, xsink);
#endif
	    break;

	 case SQLT_DAT:
	    stmt.defineByPos(w->defp, i + 1, w->buf.date, 7, SQLT_DAT, &w->ind, xsink);
	    break;

	 case SQLT_TIMESTAMP:
	 case SQLT_TIMESTAMP_TZ:
	 case SQLT_TIMESTAMP_LTZ:
	 case SQLT_DATE:
	    w->buf.odt = NULL;
            if (conn->descriptorAlloc((dvoid **)&w->buf.odt, QORE_DTYPE_TIMESTAMP, str, xsink))
               return -1;
	    //printd(5, "OraResultSet::define() this=%p got TIMESTAMP handle: %p size: %d\n", this, w->buf.odt, sizeof(w->buf.odt));
	    stmt.defineByPos(w->defp, i + 1, &w->buf.odt, sizeof(w->buf.odt), QORE_SQLT_TIMESTAMP, &w->ind, xsink);
	    break;

	 case SQLT_INTERVAL_YM:
	    w->buf.oi = NULL;
            if (conn->descriptorAlloc((dvoid **)&w->buf.oi, OCI_DTYPE_INTERVAL_YM, str, xsink))
               return -1;
	    //printd(5, "OraResultSet::define() got INTERVAL_YM handle %p\n", w->buf.oi);
	    stmt.defineByPos(w->defp, i + 1, &w->buf.oi, sizeof(w->buf.oi), SQLT_INTERVAL_YM, &w->ind, xsink);
	    break;

	 case SQLT_INTERVAL_DS:
	    w->buf.oi = NULL;
            if (conn->descriptorAlloc((dvoid **)&w->buf.oi, OCI_DTYPE_INTERVAL_DS, str, xsink))
               return -1;
	    //printd(5, "OraResultSet::define() got INTERVAL_DS handle %p\n", w->buf.oi);
	    stmt.defineByPos(w->defp, i + 1, &w->buf.oi, sizeof(w->buf.oi), SQLT_INTERVAL_DS, &w->ind, xsink);
	    break;

	 // handle raw data
	 case SQLT_BIN:
	 case SQLT_LBI: {
	    int size = w->maxsize;
	    if (!size)
	       size = ORA_RAW_SIZE;

	    w->buf.ptr = 0;
            if (conn->rawResize((OCIRaw**)&w->buf.ptr, size, xsink))
	       return -1;

	    stmt.defineByPos(w->defp, i + 1, w->buf.ptr, size + sizeof(int), SQLT_LVB, &w->ind, xsink);
	    //printd(5, "OraResultSet::define() w=%p SQLT_LVB size=%d ptr=%p\n", w, size, w->buf.ptr);
	    break;
	 }

	 case SQLT_BLOB:
	 case SQLT_CLOB:
	    w->buf.ptr = 0;
            if (conn->descriptorAlloc(&w->buf.ptr, OCI_DTYPE_LOB, str, xsink))
               return -1;
	    //printd(5, "OraResultSet::define() got LOB locator handle %p\n", w->buf.ptr);
	    stmt.defineByPos(w->defp, i + 1, &w->buf.ptr, 0, w->dtype, &w->ind, xsink);
	    break;

	 case SQLT_RSET:
	    w->buf.ptr = 0;
	    // allocate statement handle for result list
	    if (conn->handleAlloc(&w->buf.ptr, OCI_HTYPE_STMT, str, xsink))
               return -1;
	    stmt.defineByPos(w->defp, i + 1, &w->buf.ptr, 0, w->dtype, &w->ind, xsink);
	    break;

         case SQLT_NTY: {
            // w->ind is not affected in the OCIDefineByPos for SQLT_NTY. But set it to 0.
            // Real NULL is handled in getValue() for NTY.
            w->ind = 0;
            if (w->subdtype == SQLT_NTY_OBJECT) {
                w->buf.oraObj = objPlaceholderQore(conn, w->subdtypename.getBuffer(), xsink);
		if (*xsink)
		   return -1;

                stmt.defineByPos(w->defp, i + 1, 0, 0, w->dtype, &w->ind, xsink);
                conn->checkerr(OCIDefineObject((OCIDefine *) w->defp,
                                                conn->errhp,
                                                w->buf.oraObj->typinf->tdo,
                                                (void **) &w->buf.oraObj->handle,
                                                (ub4   *) NULL,
                                                (void **) &w->buf.oraObj->tab_ind,
                                                (ub4   *) NULL),
                                str, xsink);
            } else if (w->subdtype == SQLT_NTY_COLLECTION) {
                w->buf.oraColl = collPlaceholderQore(conn, w->subdtypename.getBuffer(), xsink);
                stmt.defineByPos(w->defp, i + 1, 0, 0, w->dtype, &w->ind, xsink);
                conn->checkerr(OCIDefineObject((OCIDefine *) w->defp,
                                                conn->errhp,
                                                w->buf.oraColl->typinf->tdo,
                                                (void **) &w->buf.oraColl->handle,
                                                (ub4   *) NULL,
                                                (void **) &w->buf.oraColl->tab_ind,
                                                (ub4   *) NULL),
                                str, xsink);
            } else {
                xsink->raiseException("DEFINE-NTY-ERROR", "An attempt to define unknown NTY type");
                return -1;
            }
            break;
         } // SQLT_NTY
            
#ifdef SQLT_RDD
         case SQLT_RDD:
	    w->buf.ptr = 0;
            if (conn->descriptorAlloc(&w->buf.ptr, OCI_DTYPE_ROWID, str, xsink))
               return -1;
	    //printd(5, "OraResultSet::define() got ROWID descriptor %p\n", w->buf.ptr);
	    stmt.defineByPos(w->defp, i + 1, &w->buf.ptr, 0, w->dtype, &w->ind, xsink);
	    break;
#endif

         case SQLT_NUM:
            w->maxsize = ORACLE_NUMBER_STR_LEN;
	    w->buf.ptr = malloc(sizeof(char) * (w->maxsize + 1));
	    stmt.defineByPos(w->defp, i + 1, w->buf.ptr, w->maxsize + 1, SQLT_STR, &w->ind, xsink);
            break;

         default: // treated as a string
	    if (w->charlen)
               w->maxsize = get_char_width(stmt.getEncoding(), w->charlen); 
	    w->buf.ptr = malloc(sizeof(char) * (w->maxsize + 1));
            //printd(0, "OraResultSet::define() i=%d, buf=%p, maxsize=%d\n", i + 1, w->buf.ptr, w->maxsize);
	    stmt.defineByPos(w->defp, i + 1, w->buf.ptr, w->maxsize + 1, SQLT_STR, &w->ind, xsink);
            break;
      }
      if (*xsink) return -1;
   }

   return 0;
}

