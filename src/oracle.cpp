/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  oracle.cpp

  Oracle OCI Interface to Qore DBI layer

  Qore Programming Language

  Copyright 2003 - 2009 David Nichols

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
#include "oracle-module.h"
#include "oracleobject.h"
#include "ocilib_checks.h"
#include "ocilib_defs.h"
#include "ocilib_internal.h"
#include "ocilib_types.h"
#include "oci_loader.h"
#include "oci_types.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <assert.h>

#include <memory>

DLLEXPORT char qore_module_name[] = "oracle";
DLLEXPORT char qore_module_version[] = PACKAGE_VERSION;
DLLEXPORT char qore_module_description[] = "Oracle database driver";
DLLEXPORT char qore_module_author[] = "David Nichols";
DLLEXPORT char qore_module_url[] = "http://qore.org";
DLLEXPORT int qore_module_api_major = QORE_MODULE_API_MAJOR;
DLLEXPORT int qore_module_api_minor = QORE_MODULE_API_MINOR;
DLLEXPORT qore_module_init_t qore_module_init = oracle_module_init;
DLLEXPORT qore_module_ns_init_t qore_module_ns_init = oracle_module_ns_init;
DLLEXPORT qore_module_delete_t qore_module_delete = oracle_module_delete;
DLLEXPORT qore_license_t qore_module_license = QL_LGPL;

DBIDriver *DBID_ORACLE = 0;

static int oracle_commit(Datasource *ds, ExceptionSink *xsink) {
   OracleData *d_ora = (OracleData *)ds->getPrivateData();
   return d_ora->commit(xsink);
}

static int oracle_rollback(Datasource *ds, ExceptionSink *xsink) {
   OracleData *d_ora = (OracleData *)ds->getPrivateData();
   return d_ora->rollback(xsink);
}

static int get_char_width(const QoreEncoding *enc, int num) {
#if QORE_VERSION_CODE >= 7001
   return num * enc->getMaxCharWidth();
#else
   // qore < 0.7.1 did not have the QoreEncoding::getMaxCharWidth() call, so we assume character width = 1
   // for all character encodings except UTF8
   return num * (enc == QCS_UTF8 ? 4 : 1);
#endif
}

OraColumns::OraColumns(QoreOracleStatement &n_stmt, const char *str, ExceptionSink *xsink) : stmt(n_stmt) {
   QORE_TRACE("OraColumns::OraColumns()");

   OracleData *d_ora = stmt.getData();

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
      if (stmt.attrGet(parmp, (void*)&col_name, col_name_len, OCI_ATTR_NAME, xsink))
         return;

      ub2 col_char_len;
      if (d_ora->checkerr(OCIAttrGet(parmp, OCI_DTYPE_PARAM, &col_char_len, 0, OCI_ATTR_CHAR_SIZE, d_ora->errhp), str, xsink))
         return;

      if (d_ora->checkerr(OCIAttrGet(parmp, OCI_DTYPE_PARAM, &col_max_size, 0, OCI_ATTR_DATA_SIZE, d_ora->errhp), str, xsink))
         return;

      //       printd(5, "OraColumns::OraColumns() column %s: type=%d char_len=%d size=%d (SQLT_STR=%d)\n", col_name, dtype, col_char_len, col_max_size, SQLT_STR);
      //       printd(0, "OraColumns::OraColumns() column %s: type=%d char_len=%d size=%d (SQLT_NTY=%d)\n", col_name, dtype, col_char_len, col_max_size, SQLT_NTY);
      if (dtype == SQLT_NTY) {
          char * tname; // type name
          char * sname; // schema name

          if (d_ora->checkerr(OCIAttrGet(parmp, OCI_DTYPE_PARAM, &sname, 0, OCI_ATTR_SCHEMA_NAME, d_ora->errhp), str, xsink))
             return;

          if (d_ora->checkerr(OCIAttrGet(parmp, OCI_DTYPE_PARAM, &tname, 0, OCI_ATTR_TYPE_NAME, d_ora->errhp), str, xsink))
             return;

          // printd(0, "OraColumns::OraColumns() SQLT_NTY type=%s.%s\n", sname, tname);
          QoreString s(stmt.getEncoding());
          s.concat(sname);
          s.concat(".");
          s.concat(tname);

          OCI_TypeInfo * info = OCI_TypeInfoGet2(&d_ora->ocilib, d_ora->ocilib_cn, s.getBuffer(), OCI_TIF_TYPE);

          //           printd(0, "OraColumns::OraColumns() ccode %d\n", info->ccode);
          // This is some kind of black magic - I'm not sure if it's sufficient
          // object/collection resolution method.
          int dsubtype = info->ccode ? SQLT_NTY_COLLECTION : SQLT_NTY_OBJECT;
          add((char *)col_name, col_name_len, col_max_size, dtype, col_char_len, dsubtype, s);
          continue;
      }
      if (dtype == SQLT_NCO) {
          // WTF is the NAMED COLLECTION in this case?! What's different from SQLT_NTY?
          printd(0, "OraColumns::OraColumns() SQLT_NCO - something is wrong. But what is SQLT_NCO???\n");
          assert(0);
      }

      add((char *)col_name, col_name_len, col_max_size, dtype, col_char_len);
   }
}

int OraColumns::define(const char *str, ExceptionSink *xsink) {
   //QORE_TRACE("OraColumns::define()");
   //    printd(0, "OraColumns::define()\n");

   OracleData *d_ora = stmt.getData();

   // iterate column list
   for (unsigned i = 0; i < clist.size(); ++i) {
      OraColumn *w = clist[i];
      //printd(5, "OraColumns::define() %s: w->dtype=%d\n", w->name, w->dtype);
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
            if (d_ora->descriptorAlloc((dvoid **)&w->buf.odt, QORE_DTYPE_TIMESTAMP, str, xsink))
               return -1;
	    //printd(5, "OraColumns::define() got TIMESTAMP handle: %p size: %d\n", w->buf.odt, sizeof(w->buf.odt));
	    stmt.defineByPos(w->defp, i + 1, &w->buf.odt, sizeof(w->buf.odt), QORE_SQLT_TIMESTAMP, &w->ind, xsink);
	    break;

	 case SQLT_INTERVAL_YM:
	    w->buf.oi = NULL;
            if (d_ora->descriptorAlloc((dvoid **)&w->buf.oi, OCI_DTYPE_INTERVAL_YM, str, xsink))
               return -1;
	    //printd(5, "OraColumns::define() got TIMESTAMP handle %p\n", w->buf.odt);
	    stmt.defineByPos(w->defp, i + 1, &w->buf.oi, sizeof(w->buf.oi), SQLT_INTERVAL_YM, &w->ind, xsink);
	    break;

	 case SQLT_INTERVAL_DS:
	    w->buf.oi = NULL;
            if (d_ora->descriptorAlloc((dvoid **)&w->buf.oi, OCI_DTYPE_INTERVAL_DS, str, xsink))
               return -1;
	    //printd(5, "OraColumns::define() got TIMESTAMP handle %p\n", w->buf.odt);
	    stmt.defineByPos(w->defp, i + 1, &w->buf.oi, sizeof(w->buf.oi), SQLT_INTERVAL_DS, &w->ind, xsink);
	    break;

	 // handle raw data
	 case SQLT_BIN:
	 case SQLT_LBI: {
	    //printd(5, "OraColumns::define() maxsize=%d\n", w->maxsize);
	    int size = w->maxsize;
	    if (!size)
	       size = ORA_RAW_SIZE;

	    w->buf.ptr = 0;
	    w->dtype = SQLT_LVB;
	    if (d_ora->checkerr(OCIRawResize(d_ora->envhp, d_ora->errhp, size, (OCIRaw**)&w->buf.ptr), "OraColumns::define() setup binary buffer", xsink))
	       return -1;

	    stmt.defineByPos(w->defp, i + 1, w->buf.ptr, size + sizeof(int), SQLT_LVB, &w->ind, xsink);
	    break;
	 }

	 case SQLT_BLOB:
	 case SQLT_CLOB:
	    w->buf.ptr = 0;
            if (d_ora->descriptorAlloc(&w->buf.ptr, OCI_DTYPE_LOB, str, xsink))
               return -1;
	    //printd(5, "OraColumns::define() got LOB locator handle %p\n", w->buf.ptr);
	    stmt.defineByPos(w->defp, i + 1, &w->buf.ptr, 0, w->dtype, &w->ind, xsink);
	    break;
	    
	 case SQLT_RSET:
	    w->buf.ptr = 0;
	    // allocate statement handle for result list
	    if (d_ora->handleAlloc(&w->buf.ptr, OCI_HTYPE_STMT, str, xsink))
               return -1;
	    stmt.defineByPos(w->defp, i + 1, &w->buf.ptr, 0, w->dtype, &w->ind, xsink);
	    break;

         case SQLT_NTY: {
            // w->ind is not affected in the OCIDefineByPos for SQLT_NTY. But set it to 0.
            // Real NULL is handled in getValue() for NTY.
            w->ind = 0;
            if (w->subdtype == SQLT_NTY_OBJECT) {
                w->buf.oraObj = objPlaceholderQore(d_ora, w->subdtypename.getBuffer(), xsink);
		if (*xsink)
		   return -1;

                stmt.defineByPos(w->defp, i + 1, 0, 0, w->dtype, &w->ind, xsink);
                d_ora->checkerr(OCIDefineObject((OCIDefine *) w->defp,
                                                d_ora->errhp,
                                                w->buf.oraObj->typinf->tdo,
                                                (void **) &w->buf.oraObj->handle,
                                                (ub4   *) NULL,
                                                (void **) &w->buf.oraObj->tab_ind,
                                                (ub4   *) NULL),
                                str, xsink);

            } else if (w->subdtype == SQLT_NTY_COLLECTION) {
                w->buf.oraColl = collPlaceholderQore(d_ora, w->subdtypename.getBuffer(), xsink);
                stmt.defineByPos(w->defp, i + 1, 0, 0, w->dtype, &w->ind, xsink);
                d_ora->checkerr(OCIDefineObject((OCIDefine *) w->defp,
                                                d_ora->errhp,
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
            
         default: // treated as a string
	    if (w->charlen) w->maxsize = get_char_width(stmt.getEncoding(), w->charlen); 
	    w->buf.ptr = malloc(sizeof(char) * (w->maxsize + 1));
            //printd(0, "OraColumns::define() i=%d, buf=%p, maxsize=%d\n", i + 1, w->buf.ptr, w->maxsize);
	    stmt.defineByPos(w->defp, i + 1, w->buf.ptr, w->maxsize + 1, SQLT_STR, &w->ind, xsink);
      }
      if (*xsink) return -1;
   }

   return 0;
}

static DateTimeNode *convert_date_time(unsigned char *str) {
   int year;
   if ((str[0] < 100) || (str[1] < 100))
      year = 9999; 
   else
      year = (str[0] - 100) * 100 + (str[1] - 100);

   //printd(5, "convert_date_time(): %d %d = %04d-%02d-%02d %02d:%02d:%02d\n", str[0], str[1], year, str[2], str[3], str[4] - 1, str[5] - 1, str[6] - 1);
   return new DateTimeNode(year, str[2], str[3], str[4] - 1, str[5] - 1, str[6] - 1);
}

extern "C" sb4 read_clob_callback(void *sp, CONST dvoid *bufp, ub4 len, ub1 piece) {
//    printd(5, "read_clob_callback(%p, %p, %d, %d)\n", sp, bufp, len, piece);
   (reinterpret_cast<QoreStringNode *>(sp))->concat((char *)bufp, len);
   return OCI_CONTINUE;
}

extern "C" sb4 read_blob_callback(void *bp, CONST dvoid *bufp, ub4 len, ub1 piece) {
//    printd(5, "read_blob_callback(%p, %p, %d, %d)\n", bp, bufp, len, piece);
   ((BinaryNode *)bp)->append((char *)bufp, len);
   return OCI_CONTINUE;
}

void OraValue::del(ExceptionSink *xsink) {
   switch (dtype) {
      case 0:
      case SQLT_INT:
      case SQLT_UIN:
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
      case SQLT_DAT:
         break;

      case SQLT_CLOB:
      case SQLT_BLOB:
         if (buf.ptr)
            OCIDescriptorFree(buf.ptr, OCI_DTYPE_LOB);
         break;

      case SQLT_RSET:
         if (buf.ptr)
            OCIHandleFree(buf.ptr, OCI_HTYPE_STMT);
         break;

      case SQLT_TIMESTAMP:
      case SQLT_TIMESTAMP_TZ:
      case SQLT_TIMESTAMP_LTZ:
      case SQLT_DATE:
         if (buf.odt)
            OCIDescriptorFree(buf.odt, OCI_DTYPE_TIMESTAMP);
         break;

      case SQLT_INTERVAL_YM:
         if (buf.oi)
            OCIDescriptorFree(buf.odt, OCI_DTYPE_INTERVAL_YM);
         break;

      case SQLT_INTERVAL_DS:
         if (buf.oi)
            OCIDescriptorFree(buf.odt, OCI_DTYPE_INTERVAL_DS);
         break;

      case SQLT_LVB: {
         OracleData *d_ora = stmt.getData();
         //printd(5, "freeing binary pointer for SQLT_LVB %p\n", buf.ptr);
         d_ora->checkerr(OCIRawResize(d_ora->envhp, d_ora->errhp, 0, (OCIRaw**)&buf.ptr), "OraValue::del() free binary buffer", xsink);
         break;
      }
	    
      case SQLT_NTY:
         freeObject();
         break;

      default:
         if (buf.ptr)
            free(buf.ptr);
         break;            
   }
}

AbstractQoreNode *OraValue::getValue(ExceptionSink *xsink, bool horizontal, bool destructive) {
   // SQL NULL returned
   if (ind == -1)
      return null();

   //OracleData *d_ora = stmt.getData();

   switch (dtype) {
      case SQLT_INT:
      case SQLT_UIN:
	 return new QoreBigIntNode(buf.i8);

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
	 return new QoreFloatNode(buf.f8);      

      case SQLT_DAT:
	 return convert_date_time(buf.date);

      case SQLT_TIMESTAMP:
      case SQLT_DATE:
         return stmt.getData()->getTimestamp(false, buf.odt, xsink);

      case SQLT_TIMESTAMP_TZ:
      case SQLT_TIMESTAMP_LTZ:
         return stmt.getData()->getTimestamp(true, buf.odt, xsink);

      case SQLT_INTERVAL_YM: {
	 // get oracle data
         OracleData *d_ora = stmt.getData();

	 //printd(5, "OraValue::getValue() using INTERVAL_YM handle %p\n", buf.oi);

	 sb4 year, month;
	 d_ora->checkerr(OCIIntervalGetYearMonth(d_ora->envhp, d_ora->errhp, &year, &month, buf.oi), "OCIIntervalGetYearMonth()", xsink);
	 
	 return (*xsink ? 0 : new DateTimeNode(year, month, 0, 0, 0, 0, 0, true));
      }

      case SQLT_INTERVAL_DS: {
	 // get oracle data
         OracleData *d_ora = stmt.getData();

	 //printd(5, "OraValue::getValue() using INTERVAL_DS handle %p\n", buf.oi);

	 sb4 day, hour, minute, second, nanosecond;
	 d_ora->checkerr(OCIIntervalGetDaySecond(d_ora->envhp, d_ora->errhp, &day, &hour, &minute, &second, &nanosecond, buf.oi),
		      "OCIIntervalGetDaySecond()", xsink);
#ifdef _QORE_HAS_TIME_ZONES
	 return *xsink 
	    ? NULL 
	    : DateTimeNode::makeRelative(0, 0, day, hour, minute, second, nanosecond / 1000);
#else
	 return *xsink
	    ? NULL :
	    new DateTimeNode(0, 0, day, hour, minute, second, nanosecond / 1000000, true);
#endif
      }

      case SQLT_LVB: {
	 // get oracle data
         OracleData *d_ora = stmt.getData();
	 BinaryNode *b = new BinaryNode;
	 b->append(OCIRawPtr(d_ora->envhp, (OCIRaw *)buf.ptr), OCIRawSize(d_ora->envhp, (OCIRaw *)buf.ptr));
	 return b;
      }

      case SQLT_CLOB:
      case SQLT_BLOB: {
	 // get oracle data
         OracleData *d_ora = stmt.getData();
	 
         //printd(5, "OraValue::getValue() using LOB locator handle %p\n", buf.ptr);
	 
	 // retrieve *LOB data
	 void *dbuf = malloc(LOB_BLOCK_SIZE);
         ON_BLOCK_EXIT(free, dbuf);
	 AbstractQoreNode *rv;
	 ub4 amt = 0;
	 if (dtype == SQLT_CLOB) {
	    QoreStringNodeHolder str(new QoreStringNode(stmt.getEncoding()));
	    // read LOB data in streaming callback mode
	    d_ora->checkerr(OCILobRead(d_ora->svchp, d_ora->errhp, (OCILobLocator *)buf.ptr, &amt, 1, dbuf, LOB_BLOCK_SIZE,
                                       *str, read_clob_callback, (ub2)d_ora->charsetid, (ub1)0), "oraReadCLOBCallback()", xsink);
	    rv = *xsink ? 0 : str.release();
	 }
	 else {
	    SimpleRefHolder<BinaryNode> b(new BinaryNode);
	    // read LOB data in streaming callback mode
	    d_ora->checkerr(OCILobRead(d_ora->svchp, d_ora->errhp, (OCILobLocator *)buf.ptr, &amt, 1, dbuf, LOB_BLOCK_SIZE,
                                       *b, read_blob_callback, (ub2)0, (ub1)0), "oraReadBLOBCallback()", xsink);
	    rv = *xsink ? 0 : b.release();
	 }
	 return rv;
      }

      case SQLT_RSET: {
         QoreOracleStatement tstmt(stmt.getDatasource(), (OCIStmt*)buf.takePtr());
         if (horizontal) 
            return tstmt.fetchRows(xsink);
         return tstmt.fetchColumns(xsink);
      }

      case SQLT_NTY: {
         // Real (atomic) null handling is quite tricky here.
         // pp_ind and pp_struct are unknown C structs taken from
         // the OCI NTY object where we know only the pp_ind's first
         // member "_atomic". It holds info about NULL value of the
         // whole object. The rest content of those structs is ignored.
         // (It took ages to get those info from the docs :/)
         OCIInd *pp_ind = 0; // obtain NULL info
         void *pp_struct = 0; // used only for call. No usage for its value
         OracleData *d_ora = stmt.getData();

         if (subdtype == SQLT_NTY_OBJECT) {
            OCI_ObjectGetStruct2(&d_ora->ocilib, buf.oraObj, (void**)&pp_struct, (void**)&pp_ind);
            if (*pp_ind == OCI_IND_NULL || *pp_ind == OCI_IND_BADNULL)
               return null();
            return objToQore(d_ora, buf.oraObj, stmt.getDatasource(), xsink);
         }
         else {
            assert(subdtype == SQLT_NTY_COLLECTION);
            OCI_CollGetStruct(&d_ora->ocilib, buf.oraColl, (void**)&pp_struct, (void**)&pp_ind);
            if (*pp_ind == OCI_IND_NULL || *pp_ind == OCI_IND_BADNULL)
               return null();
            return collToQore(d_ora, buf.oraColl, stmt.getDatasource(), xsink);
         }
         break;
      } // SQLT_NTY

      // treat as string
      default:
         //printd(5, "type=%d\n", dtype);
         // must be string data
         remove_trailing_blanks((char *)buf.ptr);
         if (!destructive)
            return new QoreStringNode((const char *)buf.ptr, stmt.getEncoding());
         int len = strlen((char *)buf.ptr);
         QoreStringNode *str = new QoreStringNode((char *)buf.ptr, len, len + 1, stmt.getEncoding());
         buf.ptr = 0;
         return str;
   }
   
   assert(false);
   return 0;
}

int OraBindGroup::prepare(const QoreString &sql, const QoreListNode *args, bool parse, ExceptionSink *xsink) {
   // create copy of string and convert encoding if necessary
   str = sql.convertEncoding(getEncoding(), xsink);
   if (*xsink)
      return -1;

   //printd(4, "OraBindGroup::setSQL() ds=%p, d_ora=%p, SQL='%s', args=%d\n", ds, d_ora, str->getBuffer(), args ? args->size() : 0);

   // process query string and setup bind value list
   if (parse) {
      parseQuery(args, xsink);
      if (*xsink)
	 return -1;
   }

   if (allocate(xsink))
      return -1;

   if (QoreOracleStatement::prepare(*str, xsink))
      return -1;

   if (!node_list.empty()) {
      if (bindOracle(xsink))
	 return -1;
   }

   return 0;
}

int OraBindGroup::bindOracle(ExceptionSink *xsink) {
   int pos = 1;

   for (node_list_t::iterator i = node_list.begin(), e = node_list.end(); i != e; ++i) {
      if ((*i)->isValue())
	 (*i)->bindValue(pos, xsink);
      else
	 (*i)->bindPlaceholder(pos, xsink);

      if (*xsink)
	 return -1;

      ++pos;
   }   
   //printd(0, "OraBindGroup::bindOracle() bound %d position(s), statement handle: %p\n", pos - 1, stmthp);

   //bound = true;
   return 0;
}

int OraBindGroup::bind(const QoreListNode *args, ExceptionSink *xsink) {
   //bound = false;

   for (unsigned i = 0, end = node_list.size(); i < end; ++i) {
      OraBindNode *w = node_list[i];

      // get bind argument
      const AbstractQoreNode *v = args ? args->retrieve_entry(i) : 0;

      if (w->set(v, xsink))
	 return -1;
   }

   if (bindOracle(xsink))
      return -1;

   return 0;
}

int OraBindGroup::bindPlaceholders(const QoreListNode *args, ExceptionSink *xsink) {
   //bound = false;
   
   unsigned arg_offset = 0;
   for (unsigned i = 0, end = node_list.size(); i < end; ++i) {
      OraBindNode *w = node_list[i];
      if (w->isValue())
	 continue;

      // get bind argument
      const AbstractQoreNode *v = args ? args->retrieve_entry(arg_offset++) : 0;

      if (w->set(v, xsink))
	 return -1;
   }

   if (bindOracle(xsink))
      return -1;

   return 0;
}

int OraBindGroup::bindValues(const QoreListNode *args, ExceptionSink *xsink) {
   //bound = false;

   unsigned arg_offset = 0;
   for (unsigned i = 0, end = node_list.size(); i < end; ++i) {
      OraBindNode *w = node_list[i];
      if (!w->isValue())
	 continue;

      // get bind argument
     const AbstractQoreNode *v = args ? args->retrieve_entry(arg_offset++) : 0;

     if (w->set(v, xsink))
	return -1;
   }

   if (bindOracle(xsink))
      return -1;

   return 0;
}

void OraBindGroup::parseQuery(const QoreListNode *args, ExceptionSink *xsink) {
   //printd(5, "parseQuery() args=%p str=%s\n", args, str->getBuffer());
 
   char quote = 0;

   const char *p = str->getBuffer();
   unsigned index = 0;
   QoreString tmp(getEncoding());
   while (*p) {
      if (!quote && (*p) == '%' && (p == str->getBuffer() || !isalnum(*(p-1)))) { // found value marker
	 const AbstractQoreNode *v = args ? args->retrieve_entry(index++) : NULL;

	 int offset = p - str->getBuffer();

	 ++p;
	 if ((*p) == 'd') {
	    DBI_concat_numeric(&tmp, v);
	    str->replace(offset, 2, &tmp);
	    p = str->getBuffer() + offset + tmp.strlen();
	    tmp.clear();
	    continue;
	 }
	 if ((*p) == 's') {
	    if (DBI_concat_string(&tmp, v, xsink))
	       break;
	    str->replace(offset, 2, &tmp);
	    p = str->getBuffer() + offset + tmp.strlen();
	    tmp.clear();
	    continue;
	 }
	 if ((*p) != 'v') {
	    xsink->raiseException("DBI-EXEC-PARSE-EXCEPTION", "invalid value specification (expecting '%v' or '%%d', got %%%c)", *p);
	    break;
	 }
	 ++p;
	 if (isalpha(*p)) {
	    xsink->raiseException("DBI-EXEC-PARSE-EXCEPTION", "invalid value specification (expecting '%v' or '%%d', got %%v%c*)", *p);
	    break;
	 }

	 // replace value marker with generated name
	 tmp.sprintf(":qdodvrs___%d", node_list.size());
	 str->replace(offset, 2, &tmp);
	 p = str->getBuffer() + offset + tmp.strlen();
	 tmp.clear();

	 // 	 printd(5, "OraBindGroup::parseQuery() newstr=%s\n", str->getBuffer());
	 // 	 printd(5, "OraBindGroup::parseQuery() adding value type=%s\n",v ? v->getTypeName() : "<NULL>");
	 add(v);
      }
      else if (!quote && (*p) == ':') { // found placeholder marker
	 ++p;
	 if (!isalpha(*p))
	    continue;

	 // get placeholder name
	 QoreString tstr;
	 while (isalnum(*p) || (*p) == '_')
	    tstr.concat(*(p++));

	 // add default placeholder
	 OraBindNode *n = add(tstr.giveBuffer(), -1);

	 const AbstractQoreNode *v = args ? args->retrieve_entry(index++) : NULL;

	 if (n->set(v, xsink))
	    break;
      }
      else if (((*p) == '\'') || ((*p) == '\"')) {
	 if (!quote)
	    quote = *p;
	 else if (quote == (*p))
	    quote = 0;
	 ++p;
      }
      // allow quoting of ':' and '%' characters
      else if (!quote && (*p) == '\\' && (*(p+1) == ':' || *(p+1) == '%')) {
	 str->splice(p - str->getBuffer(), 1, xsink);
	 p += 2;
      }
      else
	 ++p;
   }
}

void OraBindNode::resetPlaceholder(ExceptionSink *xsink, bool free_name) {
   if (free_name && data.ph.name) {
      free(data.ph.name);
      data.ph.name = 0;
   }

   if (data.ph.type) {
      free(data.ph.type);
      data.ph.type = 0;
   }

   // free buffer data if any
   del(xsink);

   dtype = 0;
}

int OraBindNode::set(const AbstractQoreNode *v, ExceptionSink *xsink) {
   if (bindtype == BN_VALUE) {
      resetValue(xsink);
      setValue(v, xsink);
      return *xsink ? -1 : 0;
   }

   return setPlaceholder(v, xsink);
}

void OraBindNode::reset(ExceptionSink *xsink, bool free_name) {
   if (value) {
      value->deref(xsink);
      value = 0;
   }

   // printd(0, "DLLLOCAL void OraBindNode::reset(ExceptionSink *xsink) %d\n", bindtype == BN_PLACEHOLDER);
   if (bindtype == BN_PLACEHOLDER)
      resetPlaceholder(xsink, free_name);
   else
      resetValue(xsink);
}

int OraBindNode::setupDateDescriptor(ExceptionSink *xsink) {
   OracleData *d_ora = (OracleData *)stmt.getData();

   dtype = QORE_SQLT_TIMESTAMP;
   buf.odt = NULL;

   if (d_ora->descriptorAlloc((dvoid **)&buf.odt, QORE_DTYPE_TIMESTAMP, "OraBindNode::bindValue() TIMESTAMP", xsink))
      return -1;
   return 0;
}

int OraBindNode::bindDate(int pos, ExceptionSink *xsink) {
   return stmt.bindByPos(bndp, pos, &buf.odt, 0, QORE_SQLT_TIMESTAMP, xsink);
}

int OraBindNode::setDateDescriptor(const DateTime &d, ExceptionSink *xsink) {
   OracleData *d_ora = (OracleData *)stmt.getData();

#ifdef _QORE_HAS_TIME_ZONES
   // get broken-down time information in the current time zone
   qore_tm info;
   d.getInfo(currentTZ(), info);

   // setup time zone string
   char tz[7];
   int se = info.utc_secs_east;

   if (se < 0) {
      tz[0] = '-';
      se = -se;
   }
   else
      tz[0] = '+';

   int hours = se / 3600;
   sprintf(&tz[1], "%02d:", hours);

   se %= 3600;
   sprintf(&tz[4], "%02d", se / 60);   

   //printd(5, "OraBindNode::setDateDescriptor(year=%d, month=%d, day=%d, hour=%d, minute=%d, second=%d, us=%d, tz=%s) %s\n", info.year, info.month, info.day, info.hour, info.minute, info.second, info.us, tz, info.regionName());
   if (d_ora->checkerr(OCIDateTimeConstruct(d_ora->envhp, d_ora->errhp, buf.odt, (sb2)info.year, (ub1)info.month, (ub1)info.day,
                                            (ub1)info.hour, (ub1)info.minute, (ub1)info.second, (ub4)(info.us * 1000), (OraText*)tz, 6), 
                       "OraBindNode::setDateDescriptor()", xsink))
      return -1;   
#else
   if (d_ora->checkerr(OCIDateTimeConstruct(d_ora->envhp, d_ora->errhp, buf.odt, (sb2)d.getYear(), (ub1)d.getMonth(), (ub1)d.getDay(),
                                            (ub1)d.getHour(), (ub1)d.getMinute(), (ub1)d.getSecond(),
                                            (ub4)(d.getMillisecond() * 1000000), NULL, 0), "OraBindNode::setDateDescriptor()", xsink))
      return -1;
#endif
   return 0;
}

void OraBindNode::bindValue(int pos, ExceptionSink *xsink) {
   OracleData *d_ora = (OracleData *)stmt.getData();

   ind = 0;

   //printd(5, "OraBindNode::bindValue() type=%s\n", value ? value->getTypeName() : "NOTHING");

   // bind a NULL
   if (is_nothing(value) || is_null(value)) {
      stmt.bindByPos(bndp, pos, 0, 0, SQLT_STR, xsink);
      return;
   }

   qore_type_t ntype = value->getType();
   
   if (ntype == NT_STRING) {
      const QoreStringNode *bstr = reinterpret_cast<const QoreStringNode *>(value);
      dtype = SQLT_STR;

      qore_size_t len;

      // convert to the db charset if necessary
      if (bstr->getEncoding() != stmt.getEncoding()) {
	 QoreString *nstr = bstr->QoreString::convertEncoding(stmt.getEncoding(), xsink);
	 if (*xsink)
	    return;
	 // save temporary string for later deleting
	 data.v.tstr = nstr;
	 buf.ptr = (char *)nstr->getBuffer();
	 len = nstr->strlen();
      }
      else { // bind value to buffer
	 buf.ptr = (char *)bstr->getBuffer();
	 len = bstr->strlen();
      }

      // bind it
      if (len > CLOB_THRESHOLD) {
	 //printd(5, "binding string %p len=%lld as CLOB\n", buf.ptr, len);
	 // bind as a CLOB

	 // allocate LOB descriptor
	 if (d_ora->descriptorAlloc((dvoid **)&strlob, OCI_DTYPE_LOB, "OraBindNode::bindValue() alloc LOB descriptor", xsink))
	    return;

	 // create temporary BLOB
	 if (d_ora->checkerr(OCILobCreateTemporary(d_ora->svchp, d_ora->errhp, strlob, OCI_DEFAULT, OCI_DEFAULT, OCI_TEMP_CLOB, FALSE, OCI_DURATION_SESSION),
                             "OraBindNode::bindValue() create temporary CLOB", xsink))
	    return;

	 clob_allocated = true;

	 // write the buffer data into the CLOB
	 ub4 amtp = len;
	 if (d_ora->checkerr(OCILobWrite (d_ora->svchp, d_ora->errhp, strlob, &amtp, 1, buf.ptr, len + 1, OCI_ONE_PIECE, 0, 0, d_ora->charsetid, SQLCS_IMPLICIT),
                             "OraBindNode::bindValue() write CLOB", xsink))
	    return;

         stmt.bindByPos(bndp, pos, &strlob, 0, SQLT_CLOB, xsink);
      }
      else {	 
	 // bind as a string
         stmt.bindByPos(bndp, pos, buf.ptr, len + 1, SQLT_STR, xsink);
      }

      return;
   }

   if (ntype == NT_DATE) {
      const DateTimeNode *d = reinterpret_cast<const DateTimeNode *>(value);

      if (setupDateDescriptor(xsink))
	 return;

      if (setDateDescriptor(*d, xsink))
	 return;

      bindDate(pos, xsink);
      return;
   }

   if (ntype == NT_BINARY) {
      const BinaryNode *b = reinterpret_cast<const BinaryNode *>(value);
      printd(5, "OraBindNode::bindValue() BLOB ptr=%p size=%d\n", b->getPtr(), b->size());

      stmt.bindByPos(bndp, pos, (void *)b->getPtr(), b->size(), SQLT_BIN, xsink);
      return;
   }

   if (ntype == NT_BOOLEAN) {
      buf.i4 = reinterpret_cast<const QoreBoolNode *>(value)->getValue();

      stmt.bindByPos(bndp, pos, &buf.i4, sizeof(int), SQLT_INT, xsink);
      return;
   }

   if (ntype == NT_INT) {
      const QoreBigIntNode *b = reinterpret_cast<const QoreBigIntNode *>(value);
      if (b->val <= MAXINT32 && b->val >= -MAXINT32) {
	 buf.i4 = b->val;

         stmt.bindByPos(bndp, pos, &buf.i4, sizeof(int), SQLT_INT, xsink);
      }
      else { // bind as a string value
	 dtype = SQLT_STR;

	 QoreString *tstr = new QoreString(stmt.getEncoding());
	 tstr->sprintf("%lld", b->val);
	 data.v.tstr = tstr;

	 //printd(5, "binding number '%s'\n", buf.ptr);
         stmt.bindByPos(bndp, pos, (char *)tstr->getBuffer(), tstr->strlen() + 1, SQLT_STR, xsink);
      }
      return;
   }

   if (ntype == NT_FLOAT) {
#if defined(SQLT_BDOUBLE) && defined(USE_NEW_NUMERIC_TYPES)
      stmt.bindByPos(bndp, pos, &(reinterpret_cast<QoreFloatNode *>(const_cast<AbstractQoreNode *>(value))->f), sizeof(double), SQLT_BDOUBLE, xsink);
#else
      stmt.bindByPos(bndp, pos, &(reinterpret_cast<QoreFloatNode *>(const_cast<AbstractQoreNode *>(value))->f), sizeof(double), SQLT_FLT, xsink);
#endif
      return;
   }

   if (ntype == NT_HASH) {
//        printd(0, "hash structure in the Oracle IN attribute\n");
       const QoreHashNode * h = reinterpret_cast<const QoreHashNode*>(value);
       if (!h->existsKey("type") || !h->existsKey("^oratype^") || !h->existsKey("^values^")) {
           xsink->raiseException("ORACLE-BIND-VALUE-ERROR",
                                 "Plain hash/list cannot be bound as an Oracle Object/Collection. Use bindOracleObject()/bindOracleCollection()");
           return;
       }
       
       const QoreStringNode * t = reinterpret_cast<const QoreStringNode*>(h->getKeyValue("type"));
       if (t->compare(ORACLE_OBJECT) == 0) {
//            printd(0, "binding hash as an oracle object\n");
           subdtype = SQLT_NTY_OBJECT;
           dtype = SQLT_NTY;
           buf.oraObj = objBindQore(d_ora, h, xsink);
           if (*xsink || !buf.oraObj)
               return;

           stmt.bindByPos(bndp, pos, 0, 0, SQLT_NTY, xsink);

           d_ora->checkerr(OCIBindObject(bndp, d_ora->errhp,
                                         buf.oraObj->typinf->tdo, (void**)&buf.oraObj->handle, 0,
                                         0, 0 // NULL struct
                              ),
                           "OraBindNode::bindValue() OCIBindObject", xsink);
           return;
       }
       else if (t->compare(ORACLE_COLLECTION) == 0) {
//            printd(0, "binding list as an oracle collection\n");

           subdtype = SQLT_NTY_COLLECTION;
           dtype = SQLT_NTY;
           buf.oraColl = collBindQore(d_ora, h, xsink);
           if (*xsink || !buf.oraColl)
               return;

           stmt.bindByPos(bndp, pos, 0, 0, SQLT_NTY, xsink);
      
           d_ora->checkerr(OCIBindObject(bndp, d_ora->errhp,
                                         buf.oraColl->typinf->tdo, (void**)&buf.oraColl->handle, 0,
                                         0, 0 // NULL struct
                              ),
                           "OraBindNode::bindValue() OCIBindObject", xsink);
           return;
       }
       else {
          xsink->raiseException("ORACLE-BIND-VALUE-ERROR",
                                 "Only Objects (hash) or collections (list) are allowed. Use bindOracleObject()/bindOracleCollection()");
          return;
       }
   }

   xsink->raiseException("ORACLE-BIND-VALUE-ERROR", "type '%s' is not supported for SQL binding", value->getTypeName());
}

void OraBindNode::bindPlaceholder(int pos, ExceptionSink *xsink) {
   OracleData *d_ora = (OracleData *)stmt.getData();

   //printd(5, "OraBindNode::bindPlaceholder(ds=%p, pos=%d) type=%s, size=%d)\n", ds, pos, data.ph.type, data.ph.maxsize);

   if (!strcmp(data.ph.type, "string")) {
      if (data.ph.maxsize < 0)
	 data.ph.maxsize = DBI_DEFAULT_STR_LEN;

      // simply malloc some space for sending to the new node
      dtype = SQLT_STR;
      buf.ptr = malloc(sizeof(char) * (data.ph.maxsize + 1));

      if (value) {
	 QoreStringValueHelper str(value);
	 
	 strncpy((char *)buf.ptr, str->getBuffer(), data.ph.maxsize);
	 ((char *)buf.ptr)[data.ph.maxsize] = '\0';	 
      }
      else
	 ((char *)buf.ptr)[0] = '\0';

      stmt.bindByPos(bndp, pos, buf.ptr, data.ph.maxsize + 1, SQLT_STR, xsink, &ind);
   }
   else if (!strcmp(data.ph.type, "date")) {
      //       printd(5, "oraBindNode::bindPlaceholder() this=%p, timestamp dtype=%d\n", this, QORE_SQLT_TIMESTAMP);

      dtype = QORE_SQLT_TIMESTAMP;
      buf.odt = NULL;
      d_ora->descriptorAlloc((dvoid **)&buf.odt, QORE_DTYPE_TIMESTAMP, "OraBindNode::bindPlaceholder() allocate timestamp descriptor", xsink);
      if (*xsink)
	 return;

      if (value) {
	 DateTimeNodeValueHelper d(value);

	 if (setDateDescriptor(*(*d), xsink))
	    return;

	 /*
	 if (d_ora->checkerr(
			  OCIDateTimeConstruct(d_ora->envhp, d_ora->errhp, buf.odt, (sb2)d->getYear(), (ub1)d->getMonth(), (ub1)d->getDay(),
					       (ub1)d->getHour(), (ub1)d->getMinute(), (ub1)d->getSecond(),
					       (ub4)(d->getMillisecond() * 1000), NULL, 0), "OraBindNode::bindPlaceholder() setup timestamp", xsink))
	    return;
	 */
      }

      //d_ora->checkerr(OCIBindByPos(stmthp, &bndp, d_ora->errhp, pos, &buf.odt, 0, QORE_SQLT_TIMESTAMP, &ind, (ub2 *)NULL, (ub2 *)NULL, (ub4)0, (ub4 *)NULL, OCI_DEFAULT), "OraBindNode::bindPlaceholder() timestamp", xsink);
      if (bindDate(pos, xsink))
	 return;
   }
   else if (!strcmp(data.ph.type, "binary")) {
      dtype = SQLT_LVB;
      buf.ptr = 0;

      if (value) {
	 if (value->getType() == NT_BINARY) {
	    const BinaryNode *bin = reinterpret_cast<const BinaryNode *>(value);
	    // if too big, raise an exception (not likely to happen)
	    if (bin->size() > 0x7fffffff) {
	       xsink->raiseException("BIND-ERROR", "value passed for binding is %lld bytes long, which is too big to bind as a long binary value, maximum size is %d bytes", bin->size(), 0x7fffffff);
	       return;
	    }
	    size_t size = bin->size() + sizeof(int);
	    if (ORA_RAW_SIZE > size)
	       size = ORA_RAW_SIZE;
	    
	    data.ph.maxsize = size;

	    if (d_ora->checkerr(OCIRawAssignBytes(d_ora->envhp, d_ora->errhp, (const ub1*)bin->getPtr(), bin->size(), (OCIRaw**)&buf.ptr), "OraBindNode::bindPlaceholder() bind binary value", xsink)) {
	       return;	    
	    }
	 }
	 else {
	    QoreStringValueHelper str(value);
	    // if too big, raise an exception (not likely to happen)
	    if (str->strlen() > 0x7fffffff) {
	       xsink->raiseException("BIND-ERROR", "value passed for binding is %lld bytes long, which is too big to bind as a long binary value, maximum size is %d bytes", str->strlen(), 0x7fffffff);
	       return;
	    }
	    size_t size = str->strlen() + sizeof(int);
	    if (ORA_RAW_SIZE > size)
	       size = ORA_RAW_SIZE;
	    
	    data.ph.maxsize = size;

	    if (d_ora->checkerr(OCIRawAssignBytes(d_ora->envhp, d_ora->errhp, (const ub1*)str->getBuffer(), str->strlen(), (OCIRaw**)&buf.ptr), "OraBindNode::bindPlaceholder() bind binary value from string", xsink))
	       return;
	 }
      }
      else {
	 data.ph.maxsize = ORA_RAW_SIZE;
	 buf.ptr = 0;

	 if (d_ora->checkerr(OCIRawResize(d_ora->envhp, d_ora->errhp, ORA_RAW_SIZE, (OCIRaw**)&buf.ptr), "OraBindNode::bindPlaceholder() setup binary placeholder", xsink))
	    return;
      }

      stmt.bindByPos(bndp, pos, buf.ptr, ORA_RAW_SIZE, SQLT_LVB, xsink, &ind);
   }
   else if (!strcmp(data.ph.type, "clob")) {
      dtype = SQLT_CLOB;
      buf.ptr = NULL;

      if (d_ora->descriptorAlloc(&buf.ptr, OCI_DTYPE_LOB, "OraBindNode::bindPlaceholder() allocate clob descriptor", xsink))
	 return;

      //       printd(5, "OraBindNode::bindPlaceholder() got LOB locator handle %p\n", buf.ptr);
      stmt.bindByPos(bndp, pos, &buf.ptr, 0, SQLT_CLOB, xsink, &ind);
   }
   else if (!strcmp(data.ph.type, "blob")) {
      dtype = SQLT_BLOB;
      buf.ptr = NULL;

      d_ora->descriptorAlloc(&buf.ptr, OCI_DTYPE_LOB, "OraBindNode::bindPlaceholder() allocate blob descriptor", xsink);

      if (*xsink) return;
//       printd(5, "bindPalceholder() got LOB locator handle %p\n", buf.ptr);

      stmt.bindByPos(bndp, pos, &buf.ptr, 0, SQLT_BLOB, xsink, &ind);
   }
   else if (!strcmp(data.ph.type, "integer")) {
      dtype = SQLT_INT;

      buf.i8 = value ? value->getAsBigInt() : 0;
      
      stmt.bindByPos(bndp, pos, &buf.i8, sizeof(int64), SQLT_INT, xsink, &ind);
   }
   else if (!strcmp(data.ph.type, "float")) {
      buf.f8 = value ? value->getAsFloat() : 0.0;

#if defined(SQLT_BDOUBLE) && defined(USE_NEW_NUMERIC_TYPES)
      dtype = SQLT_BDOUBLE;
      stmt.bindByPos(bndp, pos, &buf.f8, sizeof(double), SQLT_BDOUBLE, xsink, &ind);
#else
      dtype = SQLT_FLT;
      stmt.bindByPos(bndp, pos, &buf.f8, sizeof(double), SQLT_FLT, xsink, &ind);
#endif
   }
   else if (!strcmp(data.ph.type, "hash")) {
      dtype = SQLT_RSET;

      // allocate statement handle for result list
      d_ora->checkerr(OCIHandleAlloc(d_ora->envhp, (dvoid **)&buf.ptr, OCI_HTYPE_STMT, 0, 0), "OraBindNode::bindPlaceHolder() allocate statement handle", xsink);

      if (!*xsink)
         stmt.bindByPos(bndp, pos, &buf.ptr, 0, SQLT_RSET, xsink, &ind);
      else
	 buf.ptr = NULL;
   }
   else if (!strcmp(data.ph.type, ORACLE_OBJECT)) {
       subdtype = SQLT_NTY_OBJECT;
       dtype = SQLT_NTY;

       const QoreHashNode * h = reinterpret_cast<const QoreHashNode*>(value);
       if (h->existsKey("^values^"))
           buf.oraObj = objBindQore(d_ora, h, xsink); // IN/OUT
       else {
           const QoreStringNode * str = reinterpret_cast<const QoreStringNode*>(h->getKeyValue("value"));
           buf.oraObj = objPlaceholderQore(d_ora, str->getBuffer(), xsink); // IN
       }

       if (*xsink)
           return;

       stmt.bindByPos(bndp, pos,  0, 0, SQLT_NTY, xsink, &ind);

       d_ora->checkerr(OCIBindObject(bndp, d_ora->errhp,
                                     buf.oraObj->typinf->tdo, (void**)&buf.oraObj->handle, 0,
                                     0, 0 // NULL struct
                          ),
                       "OraBindNode::bindPlaceholder() OCIBindObject", xsink);

       //printd(0, "OraBindNode::bindValue() object '%s' tdo=0x%x handle=0x%x\n", h->getBuffer(), buf.oraObj->typinf->tdo, buf.oraObj->handle);
   }
   else if (!strcmp(data.ph.type, ORACLE_COLLECTION)) {
       subdtype = SQLT_NTY_COLLECTION;
       dtype = SQLT_NTY;

       const QoreHashNode * h = reinterpret_cast<const QoreHashNode*>(value);
       if (h->existsKey("^values^"))
           buf.oraColl = collBindQore(d_ora, h, xsink); // IN/OUT
       else {
           const QoreStringNode * str = reinterpret_cast<const QoreStringNode*>(h->getKeyValue("value"));
           buf.oraColl = collPlaceholderQore(d_ora, str->getBuffer(), xsink); // IN
       }

       if (*xsink)
           return;

       stmt.bindByPos(bndp, pos,  0, 0, SQLT_NTY, xsink, &ind);

       d_ora->checkerr(
                    OCIBindObject(bndp, d_ora->errhp,
                                 buf.oraColl->typinf->tdo, (void**)&buf.oraColl->handle, 0,
                                 0, 0 // NULL struct
                                 ),
                    "OraBindNode::bindPalceholder() OCIBindObject collection", xsink);
//         assert(0);
   }
   else {
      //printd(0, "OraBindNode::bindPlaceholder(ds=%p, pos=%d) type=%s, size=%d)\n", ds, pos, data.ph.type, data.ph.maxsize);
      xsink->raiseException("BIND-EXCEPTION", "type '%s' is not supported for SQL binding", data.ph.type);
   }
}

void OraBindNode::resetValue(ExceptionSink *xsink) {
   if (!dtype) {
      assert(!data.v.tstr);
      return;
   }

   if (data.v.tstr)
      delete data.v.tstr;

   if (strlob) {
      if (clob_allocated) {
         OracleData *d_ora = stmt.getData();
         //printd(5, "deallocating temporary clob\n");
         d_ora->checkerr(OCILobFreeTemporary(d_ora->svchp, d_ora->errhp, strlob), "OraBindNode::resetValue() free temporary CLOB", xsink);
      }
      //printd(5, "freeing clob descriptor\n");
      OCIDescriptorFree(strlob, OCI_DTYPE_LOB);
   }
   else if (dtype == SQLT_NTY)
      freeObject();
}

AbstractQoreNode *OraBindNode::getValue(bool horizontal, ExceptionSink *xsink) {
   return OraValue::getValue(xsink, horizontal, true);
}

QoreHashNode *OraBindGroup::getOutputHash(bool rows, ExceptionSink *xsink) {
   ReferenceHolder<QoreHashNode> h(new QoreHashNode, xsink);

   for (node_list_t::iterator i = node_list.begin(), e = node_list.end(); i != e; ++i) {
      if ((*i)->bindtype == BN_PLACEHOLDER)
	 h->setKeyValue((*i)->data.ph.name, (*i)->getValue(rows, xsink), xsink);
   }

   return *xsink ? 0 : h.release();
}

AbstractQoreNode *OraBindGroup::execWithPrologue(bool rows, ExceptionSink *xsink) {
   if (exec(xsink))
      return 0;

   ReferenceHolder<AbstractQoreNode> rv(xsink);

   // if there are output variables, then fix values if necessary and return
   if (hasOutput)
      rv = getOutputHash(rows, xsink);
   else {
      // get row count
      int rc = affectedRows(xsink);
      rv = *xsink ? 0 : new QoreBigIntNode(rc);
   }
   
   // commit transaction if autocommit set for datasource
   if (ds->getAutoCommit())
      getData()->commit(xsink);

   return *xsink ? 0 : rv.release();
}

int OraBindGroup::affectedRows(ExceptionSink *xsink) {
   int rc = 0;
   getData()->checkerr(OCIAttrGet(stmthp, OCI_HTYPE_STMT, &rc, 0, OCI_ATTR_ROW_COUNT, getData()->errhp), "OraBindGroup::affectedRows()", xsink);
   return rc;
}

AbstractQoreNode *OraBindGroup::select(ExceptionSink *xsink) {
   if (execute("OraBindGroup::select()", xsink))
      return 0;

   ReferenceHolder<QoreHashNode> h(QoreOracleStatement::fetchColumns(xsink), xsink);
   if (*xsink)
      return 0;

   // commit transaction if autocommit set for datasource
   if (ds->getAutoCommit() && getData()->commit(xsink))
      return 0;

   return h.release();
}

AbstractQoreNode *OraBindGroup::selectRows(ExceptionSink *xsink) {
   if (execute("OraBindGroup::selectRows()", xsink))
      return 0;

   ReferenceHolder<QoreListNode> l(QoreOracleStatement::fetchRows(xsink), xsink);
   if (*xsink)
      return 0;

   // commit transaction if autocommit set for datasource
   if (ds->getAutoCommit() && getData()->commit(xsink))
      return 0;

   return l.release();
}

QoreHashNode *OraBindGroup::fetchRow(ExceptionSink *xsink) {
   assert(columns);
/*
   if (!columns) {
      columns = new OraColumns(*this, "OraBindGroup::fetchRow", xsink);
      if (*xsink || fetch(xsink))
         return 0;
   }
*/
   return QoreOracleStatement::fetchRow(*columns, xsink);
}

QoreListNode *OraBindGroup::fetchRows(int rows, ExceptionSink *xsink) {
   assert(columns);
/*
   if (!columns) {
      columns = new OraColumns(*this, "OraBindGroup::fetchRows", xsink);
      if (*xsink)
         return 0;
   }
*/
   return QoreOracleStatement::fetchRows(*columns, rows, xsink);
}

QoreHashNode *OraBindGroup::fetchColumns(int rows, ExceptionSink *xsink) {
   assert(columns);
/*
   if (!columns) {
      columns = new OraColumns(*this, "OraBindGroup::fetchColumns", xsink);
      if (*xsink)
         return 0;
   }
*/
   return QoreOracleStatement::fetchColumns(*columns, rows, xsink);
}

#ifdef _QORE_HAS_PREPARED_STATMENT_API
static int oracle_stmt_prepare(SQLStatement *stmt, const QoreString &str, const QoreListNode *args, ExceptionSink *xsink) {
   assert(!stmt->getPrivateData());

   OraBindGroup *bg = new OraBindGroup(stmt->getDatasource());
   stmt->setPrivateData(bg);

   return bg->prepare(str, args, true, xsink);
}

static int oracle_stmt_prepare_raw(SQLStatement *stmt, const QoreString &str, ExceptionSink *xsink) {
   assert(!stmt->getPrivateData());

   OraBindGroup *bg = new OraBindGroup(stmt->getDatasource());
   stmt->setPrivateData(bg);

   return bg->prepare(str, 0, false, xsink);
}

static int oracle_stmt_bind(SQLStatement *stmt, const QoreListNode &l, ExceptionSink *xsink) {
   OraBindGroup *bg = (OraBindGroup *)stmt->getPrivateData();
   assert(bg);

   return bg->bind(&l, xsink);
}

static int oracle_stmt_bind_placeholders(SQLStatement *stmt, const QoreListNode &l, ExceptionSink *xsink) {
   OraBindGroup *bg = (OraBindGroup *)stmt->getPrivateData();
   assert(bg);

   return bg->bindPlaceholders(&l, xsink);
}

static int oracle_stmt_bind_values(SQLStatement *stmt, const QoreListNode &l, ExceptionSink *xsink) {
   OraBindGroup *bg = (OraBindGroup *)stmt->getPrivateData();
   assert(bg);

   return bg->bindValues(&l, xsink);
}

static int oracle_stmt_exec(SQLStatement *stmt, ExceptionSink *xsink) {
   OraBindGroup *bg = (OraBindGroup *)stmt->getPrivateData();
   assert(bg);

   return bg->exec(xsink);
}

static int oracle_stmt_define(SQLStatement *stmt, ExceptionSink *xsink) {
   OraBindGroup *bg = (OraBindGroup *)stmt->getPrivateData();
   assert(bg);

   return bg->define(xsink);
}

static int oracle_stmt_affected_rows(SQLStatement *stmt, ExceptionSink *xsink) {
   OraBindGroup *bg = (OraBindGroup *)stmt->getPrivateData();
   assert(bg);

   return bg->affectedRows(xsink);
}

static QoreHashNode *oracle_stmt_get_output(SQLStatement *stmt, ExceptionSink *xsink) {
   OraBindGroup *bg = (OraBindGroup *)stmt->getPrivateData();
   assert(bg);

   return bg->getOutput(xsink);
}

static QoreHashNode *oracle_stmt_get_output_rows(SQLStatement *stmt, ExceptionSink *xsink) {
   OraBindGroup *bg = (OraBindGroup *)stmt->getPrivateData();
   assert(bg);

   return bg->getOutputRows(xsink);
}

static QoreHashNode *oracle_stmt_fetch_row(SQLStatement *stmt, ExceptionSink *xsink) {
   OraBindGroup *bg = (OraBindGroup *)stmt->getPrivateData();
   assert(bg);

   return bg->fetchRow(xsink);
}

static QoreListNode *oracle_stmt_fetch_rows(SQLStatement *stmt, int rows, ExceptionSink *xsink) {
   OraBindGroup *bg = (OraBindGroup *)stmt->getPrivateData();
   assert(bg);

   return bg->fetchRows(rows, xsink);
}

static QoreHashNode *oracle_stmt_fetch_columns(SQLStatement *stmt, int rows, ExceptionSink *xsink) {
   OraBindGroup *bg = (OraBindGroup *)stmt->getPrivateData();
   assert(bg);

   return bg->fetchColumns(rows, xsink);
}

static bool oracle_stmt_next(SQLStatement *stmt, ExceptionSink *xsink) {
   OraBindGroup *bg = (OraBindGroup *)stmt->getPrivateData();
   assert(bg);

   return bg->next(xsink);
}

static int oracle_stmt_close(SQLStatement *stmt, ExceptionSink *xsink) {
   OraBindGroup *bg = (OraBindGroup *)stmt->getPrivateData();
   assert(bg);

   bg->reset(xsink);
   delete bg;
   stmt->setPrivateData(0);
   return *xsink ? -1 : 0;
}
#endif // _QORE_HAS_PREPARED_STATMENT_API

static AbstractQoreNode *oracle_exec(Datasource *ds, const QoreString *qstr, const QoreListNode *args, ExceptionSink *xsink) {
   OraBindGroupHelper bg(ds, xsink);

   if (bg.prepare(qstr, args, true, xsink))
      return 0;

   return bg.execWithPrologue(false, xsink);
}

#ifdef _QORE_HAS_DBI_EXECRAW
static AbstractQoreNode *oracle_execRaw(Datasource *ds, const QoreString *qstr, ExceptionSink *xsink) {
   OraBindGroupHelper bg(ds, xsink);

   if (bg.prepare(qstr, 0, false, xsink))
      return 0;

   return bg.execWithPrologue(false, xsink);
}
#endif

static AbstractQoreNode *oracle_select(Datasource *ds, const QoreString *qstr, const QoreListNode *args, ExceptionSink *xsink) {
   OraBindGroupHelper bg(ds, xsink);

   if (bg.prepare(qstr, args, true, xsink))
      return 0;

   return bg.select(xsink);
}

static AbstractQoreNode *oracle_select_rows(Datasource *ds, const QoreString *qstr, const QoreListNode *args, ExceptionSink *xsink) {
   OraBindGroupHelper bg(ds, xsink);

   if (bg.prepare(qstr, args, true, xsink))
      return 0;

   return bg.selectRows(xsink);
}

static int oracle_open(Datasource *ds, ExceptionSink *xsink) {
//    printd(5, "oracle_open() datasource %p for DB=%s open\n", ds, ds->getDBName());

   if (!ds->getUsername()) {
      xsink->raiseException("DATASOURCE-MISSING-USERNAME", "Datasource has an empty username parameter");
      return -1;
   }
   if (!ds->getPassword()) {
      xsink->raiseException("DATASOURCE-MISSING-PASSWORD", "Datasource has an empty password parameter");
      return -1;
   }
   if (!ds->getDBName()) {
      xsink->raiseException("DATASOURCE-MISSING-DBNAME", "Datasource has an empty dbname parameter");
      return -1;
   }

#ifdef QORE_HAS_DATASOURCE_PORT
   int port = ds->getPort();
#else
   int port = 0;
#endif

   if (port && !ds->getHostName()) {
      xsink->raiseException("DATASOURCE-MISSING-HOSTNAME", "port is set to %d, but no hostname is set; both hostname and port must be set to make a direct connection without TNS", port);
      return -1;
   }

   if (!port && ds->getHostName()) {
      xsink->raiseException("DATASOURCE-MISSING-PORT", "hostname is set to '%s', but no port is set; both hostname and port must be set to make a direct connection without TNS", ds->getHostName());
      return -1;
   }   

   QoreString db;
   if (port)
      db.sprintf("//%s:%d/%s", ds->getHostName(), port, ds->getDBName());
   else
      db.concat(ds->getDBName());

   //    printd(5, "oracle_open(): user=%s pass=%s db=%s (oracle encoding=%s)\n",
   // 	  ds->getUsername(), ds->getPassword(), db.getBuffer(), ds->getDBEncoding() ? ds->getDBEncoding() : "(none)");

   std::auto_ptr<OracleData> d_ora(new OracleData(*ds));

   // locking is done on the level above with the Datasource class
   int oci_flags = OCI_DEFAULT|OCI_THREADED|OCI_NO_MUTEX|OCI_OBJECT;

   const char *charset;

   // FIXME: maybe I don't need a temporary environment handle?
   // create temporary environment handle
   OCIEnv *tmpenvhp;
   OCIEnvCreate(&tmpenvhp, oci_flags | OCI_NO_UCB, 0, 0, 0, 0, 0, 0);
   // declare temporary buffer
   char nbuf[OCI_NLS_MAXBUFSZ];
   int need_to_set_charset = 0;

   if (ds->getDBEncoding()) {
      charset = ds->getDBEncoding();
      need_to_set_charset = 1;
   }
   else { // get Oracle character set name from OS character set name
      if ((OCINlsNameMap(tmpenvhp, (oratext *)nbuf, OCI_NLS_MAXBUFSZ, (oratext *)QCS_DEFAULT->getCode(), OCI_NLS_CS_IANA_TO_ORA) != OCI_SUCCESS)) {
	 OCIHandleFree(tmpenvhp, OCI_HTYPE_ENV);
	 xsink->raiseException("DBI:ORACLE:UNKNOWN-CHARACTER-SET", 
			       "cannot map default OS encoding '%s' to Oracle character encoding",
			       QCS_DEFAULT->getCode());
	 return -1;
      }
      ds->setDBEncoding(nbuf);
      ds->setQoreEncoding(QCS_DEFAULT);
      charset = nbuf;
//       printd(5, "oracle_open() setting Oracle encoding to '%s' from default OS encoding '%s'\n",
// 	     charset, QCS_DEFAULT->getCode());
   }

#ifndef HAVE_OCIENVNLSCREATE
#error need to define HAVE_OCIENVNLSCREATE (with Oracle 9i+)
#endif // HAVE_OCIENVNLSCREATE

   // get character set ID
   d_ora->charsetid = OCINlsCharSetNameToId(tmpenvhp, (oratext *)charset);
   // delete temporary environmental handle
   OCIHandleFree(tmpenvhp, OCI_HTYPE_ENV);

   if (!d_ora->charsetid) {
      xsink->raiseException("DBI:ORACLE:UNKNOWN-CHARACTER-SET", "this installation of Oracle does not support the '%s' character encoding", 
			    ds->getDBEncoding());
      return -1;
   }

//    printd(5, "Oracle character encoding '%s' has ID %d, oci_flags=%d\n", charset, d_ora->charsetid, oci_flags);
   // create environment with default character set
   if (OCIEnvNlsCreate(&d_ora->envhp, oci_flags, 0, NULL, NULL, NULL, 0, NULL, d_ora->charsetid, d_ora->charsetid) != OCI_SUCCESS) {
      xsink->raiseException("DBI:ORACLE:OPEN-ERROR", "error creating new environment handle with encoding '%s'", ds->getDBEncoding());
      return -1;
   }

   //printd(0, "oracle_open() ds=%p allocated envhp=%p\n", ds, d_ora->envhp);

   // map the Oracle character set to a qore character set
   if (need_to_set_charset) {
      // map Oracle character encoding name to QORE/OS character encoding name
      if ((OCINlsNameMap(d_ora->envhp, (oratext *)nbuf, OCI_NLS_MAXBUFSZ, (oratext *)ds->getDBEncoding(), OCI_NLS_CS_ORA_TO_IANA) == OCI_SUCCESS)) {
// 	 printd(5, "oracle_open() Oracle character encoding '%s' mapped to '%s' character encoding\n", ds->getDBEncoding(), nbuf);
	 ds->setQoreEncoding(nbuf);
      }
      else {
	 xsink->raiseException("DBI:ORACLE:OPEN-ERROR", "error mapping Oracle encoding '%s' to a qore encoding: unknown encoding", ds->getDBEncoding());
	 return -1;
      }
   }

   if (OCIHandleAlloc(d_ora->envhp, (dvoid **) &d_ora->errhp, OCI_HTYPE_ERROR, 0, 0) != OCI_SUCCESS) {
      OCIHandleFree(d_ora->envhp, OCI_HTYPE_ENV);
      xsink->raiseException("DBI:ORACLE:OPEN-ERROR", "failed to allocate error handle for connection");
      return -1;
   }
   //printd(5, "oracle_open() about to call OCILogon()\n");
   d_ora->checkerr(OCILogon(d_ora->envhp, d_ora->errhp, &d_ora->svchp, (text *)ds->getUsername(), strlen(ds->getUsername()), (text *)ds->getPassword(), strlen(ds->getPassword()), (text *)db.getBuffer(), db.strlen()), 
                   "<open>", xsink);
   if (*xsink) {
      OCIHandleFree(d_ora->errhp, OCI_HTYPE_ERROR);
      OCIHandleFree(d_ora->envhp, OCI_HTYPE_ENV);
      return -1;
   }

//    printd(5, "oracle_open() datasource %p for DB=%s open (envhp=%p)\n", ds, db.getBuffer(), d_ora->envhp);
   
   if (!OCI_Initialize2(&d_ora->ocilib, d_ora->envhp, d_ora->errhp, ocilib_err_handler, NULL, oci_flags)) {
       xsink->raiseException("DBI:ORACLE:OPEN-ERROR", "failed to allocate OCILIB support handlers");
       return -1;
   }

   //printd(5, "d_ora->ocilib=%p mode=%d\n", &d_ora->ocilib, d_ora->ocilib.env_mode);
   
   d_ora->ocilib_cn = new OCI_Connection;
   // fake the OCI_Connection
   d_ora->ocilib_cn->err = d_ora->errhp;
   d_ora->ocilib_cn->cxt = d_ora->svchp;
   d_ora->ocilib_cn->tinfs = OCI_ListCreate2(&d_ora->ocilib, OCI_IPC_TYPE_INFO);
   // then reset unused attributes
   d_ora->ocilib_cn->db = 0;
   d_ora->ocilib_cn->user = 0;      /* user */
   d_ora->ocilib_cn->pwd = 0;       /* password */
   d_ora->ocilib_cn->stmts = 0;     /* list of statements */
   d_ora->ocilib_cn->trsns = 0;     /* list of transactions */

   d_ora->ocilib_cn->trs=0;       /* pointer to current transaction object */
   d_ora->ocilib_cn->pool=0;      /* pointer to connection pool parent */
   d_ora->ocilib_cn->svopt=0;     /* Pointer to server output object */
   d_ora->ocilib_cn->svr=0;       /* OCI server handle */

   /* OCI session handle */
//    d_ora->checkerr(
//                 OCI_HandleAlloc((dvoid *) OCILib.env,
//                                                   (dvoid **) (void *) &d_ora->ocilib_cn->ses,
//                                                   (ub4) OCI_HTYPE_SESSION,
//                                                   (size_t) 0, (dvoid **) NULL),
//                 "oracle_open OCI_HTYPE_SESSION", xsink);
   d_ora->ocilib_cn->ses=0;       /* OCI session handle */
   d_ora->ocilib_cn->autocom=false;   /* auto commit mode */
   d_ora->ocilib_cn->nb_files=0;  /* number of OCI_File opened by the connection */
   d_ora->ocilib_cn->mode=0;      /* session mode */
   d_ora->ocilib_cn->cstate = 0;    /* connection state */
   d_ora->ocilib_cn->usrdata=0;   /* user data */

   d_ora->ocilib_cn->fmt_date = 0;  /* date string format for conversion */
   d_ora->ocilib_cn->fmt_num = 0;   /* numeric string format for conversion */
   d_ora->ocilib_cn->ver_str=0;   /* string  server version*/
   d_ora->ocilib_cn->ver_num = d_ora->ocilib.version_runtime;   /* numeric server version */
   d_ora->ocilib_cn->trace=0;     /* trace information */

   ds->setPrivateData((void *)d_ora.release());

   return 0;
}

static int oracle_close(Datasource *ds) {
   QORE_TRACE("oracle_close()");

   OracleData *d_ora = (OracleData *)ds->getPrivateData();

   //printd(0, "oracle_close() ds=%p envhp=%p ocilib envhp=%p\n", ds, d_ora->envhp, OCILib.env);

//    printd(0, "oracle_close(): connection to %s closed.\n", ds->getDBName());
//    printd(0, "oracle_close(): ptr: %p\n", d_ora);
//    printd(0, "oracle_close(): d_ora->svchp, d_ora->errhp: %p, %p\n", d_ora->svchp, d_ora->errhp);
   OCILogoff(d_ora->svchp, d_ora->errhp);
   OCIHandleFree(d_ora->svchp, OCI_HTYPE_SVCCTX);
//    OCIHandleFree(d_ora->errhp, OCI_HTYPE_ERROR); // deleted in OCI_Cleanup
//    OCIHandleFree(d_ora->envhp, OCI_HTYPE_ENV); // OCI_Cleanup
   OCI_ListForEach(&d_ora->ocilib, d_ora->ocilib_cn->tinfs, (boolean (*)(void *)) OCI_TypeInfoClose);
   OCI_ListFree(&d_ora->ocilib, d_ora->ocilib_cn->tinfs);

   OCI_Cleanup2(&d_ora->ocilib);

   delete d_ora;

   ds->setPrivateData(0);

   return 0;
}

#define VERSION_BUF_SIZE 512
static AbstractQoreNode *oracle_get_server_version(Datasource *ds, ExceptionSink *xsink) {
   // get private data structure for connection
   OracleData *d_ora = (OracleData *)ds->getPrivateData();
   
   // buffer for version information
   char version_buf[VERSION_BUF_SIZE + 1];
   
   // execute OCIServerVersion and check status code
   d_ora->checkerr(OCIServerVersion(d_ora->svchp, d_ora->errhp, (OraText *)version_buf, VERSION_BUF_SIZE, OCI_HTYPE_SVCCTX),
                   "oracle_get_server_version", xsink);
   if (*xsink)
      return 0;
   
   return new QoreStringNode(version_buf);   
}

#ifdef HAVE_OCICLIENTVERSION
static AbstractQoreNode *oracle_get_client_version(const Datasource *ds, ExceptionSink *xsink) {
   sword major, minor, update, patch, port_update;

   OCIClientVersion(&major, &minor, &update, &patch, &port_update);
   QoreHashNode *h = new QoreHashNode();
   h->setKeyValue("major", new QoreBigIntNode(major), NULL);
   h->setKeyValue("minor", new QoreBigIntNode(minor), NULL);
   h->setKeyValue("update", new QoreBigIntNode(update), NULL);
   h->setKeyValue("patch", new QoreBigIntNode(patch), NULL);
   h->setKeyValue("port_update", new QoreBigIntNode(port_update), NULL);
   return h;
}
#endif

QoreStringNode *oracle_module_init() {
   QORE_TRACE("oracle_module_init()");
   
   builtinFunctions.add2("bindOracleObject", f_oracle_object, QC_NO_FLAGS, QDOM_DATABASE, hashTypeInfo, 2, stringTypeInfo, QORE_PARAM_NO_ARG, hashTypeInfo, QORE_PARAM_NO_ARG);
   builtinFunctions.add2("placeholderOracleObject", f_oracle_object_placeholder, QC_NO_FLAGS, QDOM_DATABASE, hashTypeInfo, 1, stringTypeInfo, QORE_PARAM_NO_ARG);
   builtinFunctions.add2("bindOracleCollection", f_oracle_collection, QC_NO_FLAGS, QDOM_DATABASE, hashTypeInfo, 2, stringTypeInfo, QORE_PARAM_NO_ARG, listTypeInfo, QORE_PARAM_NO_ARG);
   builtinFunctions.add2("placeholderOracleCollection", f_oracle_collection_placeholder, QC_NO_FLAGS, QDOM_DATABASE, hashTypeInfo, 1, stringTypeInfo, QORE_PARAM_NO_ARG);

   // register driver with DBI subsystem
   qore_dbi_method_list methods;
   methods.add(QDBI_METHOD_OPEN, oracle_open);
   methods.add(QDBI_METHOD_CLOSE, oracle_close);
   methods.add(QDBI_METHOD_SELECT, oracle_select);
   methods.add(QDBI_METHOD_SELECT_ROWS, oracle_select_rows);
   methods.add(QDBI_METHOD_EXEC, oracle_exec);
#ifdef _QORE_HAS_DBI_EXECRAW
   methods.add(QDBI_METHOD_EXECRAW, oracle_execRaw);
#endif   
   methods.add(QDBI_METHOD_COMMIT, oracle_commit);
   methods.add(QDBI_METHOD_ROLLBACK, oracle_rollback);
   methods.add(QDBI_METHOD_GET_SERVER_VERSION, oracle_get_server_version);
#ifdef HAVE_OCICLIENTVERSION
   methods.add(QDBI_METHOD_GET_CLIENT_VERSION, oracle_get_client_version);
#endif

#ifdef _QORE_HAS_PREPARED_STATMENT_API
   methods.add(QDBI_METHOD_STMT_PREPARE, oracle_stmt_prepare);
   methods.add(QDBI_METHOD_STMT_PREPARE_RAW, oracle_stmt_prepare_raw);
   methods.add(QDBI_METHOD_STMT_BIND, oracle_stmt_bind);
   methods.add(QDBI_METHOD_STMT_BIND_PLACEHOLDERS, oracle_stmt_bind_placeholders);
   methods.add(QDBI_METHOD_STMT_BIND_VALUES, oracle_stmt_bind_values);
   methods.add(QDBI_METHOD_STMT_EXEC, oracle_stmt_exec);
   methods.add(QDBI_METHOD_STMT_DEFINE, oracle_stmt_define);
   methods.add(QDBI_METHOD_STMT_FETCH_ROW, oracle_stmt_fetch_row);
   methods.add(QDBI_METHOD_STMT_FETCH_ROWS, oracle_stmt_fetch_rows);
   methods.add(QDBI_METHOD_STMT_FETCH_COLUMNS, oracle_stmt_fetch_columns);
   methods.add(QDBI_METHOD_STMT_NEXT, oracle_stmt_next);
   methods.add(QDBI_METHOD_STMT_CLOSE, oracle_stmt_close);
   methods.add(QDBI_METHOD_STMT_AFFECTED_ROWS, oracle_stmt_affected_rows);
   methods.add(QDBI_METHOD_STMT_GET_OUTPUT, oracle_stmt_get_output);
   methods.add(QDBI_METHOD_STMT_GET_OUTPUT_ROWS, oracle_stmt_get_output_rows);
#endif // _QORE_HAS_PREPARED_STATMENT_API
   
   DBID_ORACLE = DBI.registerDriver("oracle", methods, DBI_ORACLE_CAPS);

   return NULL;
}

void oracle_module_ns_init(QoreNamespace *rns, QoreNamespace *qns) {
   QORE_TRACE("oracle_module_ns_init()");
}


void oracle_module_delete() {
   QORE_TRACE("oracle_module_delete()");
}
