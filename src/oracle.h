/*
  oracle.h

  Qore Programming Language

  Copyright (C) 2003 - 2009 David Nichols

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

#ifndef QORE_ORACLE_H

#define QORE_ORACLE_H

#include "../config.h"
#include "oracle-config.h"
#include <qore/Qore.h>

#include <oci.h>

// with 10g on Linux the streaming *lob callback function would 
// never get more than 1024 bytes of data at a time, however with a 9i
// server and client on Solaris, it would not work unless my 
// buffer size was at least twice as big as my CLOB!
#ifdef NEED_ORACLE_LOB_WORKAROUND
#define LOB_BLOCK_SIZE 128*1024
#else
#define LOB_BLOCK_SIZE 1024
#endif

#define MAXINT32 2147483647   // 2^^32 - 1

#define ORA_RAW_SIZE 65535

// should be 32767
#define CLOB_THRESHOLD 30000

static int ora_checkerr(OCIError *errhp, sword status, const char *query_name, Datasource *ds, ExceptionSink *xsink);

class OracleData {
   public:
      OCIEnv *envhp;
      OCIError *errhp;
      OCISvcCtx *svchp;
      ub2 charsetid;
};

// FIXME: do not hardcode byte widths - could be incorrect on some platforms
union ora_value {
      void *ptr;
      unsigned char date[7];
      int i4;
      int64 i8;
      double f8;
      OCIDateTime *odt;
      OCIInterval *oi;
};

class OraColumn {
   public:
      char *name;
      int maxsize;
      ub2 dtype;           // Oracle datatype for column
      OCIDefine *defp;     // define handle
      sb2 ind;             // indicator value
      ub2 charlen;

      union ora_value val;

      OraColumn *next;

      DLLLOCAL inline OraColumn(const char *n, int len, int ms, ub2 dt, ub2 n_charlen) {
	 name = (char *)malloc(sizeof(char) * (len + 1));
	 strncpy(name, n, len);
	 name[len] = '\0';
	 strtolower(name);
	 maxsize = ms;
	 dtype = dt;
	 charlen = n_charlen;

	 defp = NULL;

	 next = NULL;
      }

      DLLLOCAL void del(Datasource *ds, ExceptionSink *xsink) {
	 free(name);
	 if (defp) {
	    //printd(5, "freeing type %d\n", dtype);
	    switch (dtype) {
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
		  if (val.ptr)
		     OCIDescriptorFree(val.ptr, OCI_DTYPE_LOB);
		  break;

	       case SQLT_RSET:
		  if (val.ptr)
		     OCIHandleFree(val.ptr, OCI_HTYPE_STMT);
		  break;

	       case SQLT_TIMESTAMP:
	       case SQLT_TIMESTAMP_TZ:
	       case SQLT_TIMESTAMP_LTZ:
	       case SQLT_DATE:
		  if (val.odt)
		     OCIDescriptorFree(val.odt, OCI_DTYPE_TIMESTAMP);
		  break;

	       case SQLT_INTERVAL_YM:
		  if (val.oi)
		     OCIDescriptorFree(val.odt, OCI_DTYPE_INTERVAL_YM);
		  break;

	       case SQLT_INTERVAL_DS:
		  if (val.oi)
		     OCIDescriptorFree(val.odt, OCI_DTYPE_INTERVAL_DS);
		  break;

	       case SQLT_LVB: {
		  OracleData *d_ora = (OracleData *)ds->getPrivateData();
		  //printd(5, "freeing binary pointer for SQLT_LVB %p\n", val.ptr);
		  ora_checkerr(d_ora->errhp, OCIRawResize(d_ora->envhp, d_ora->errhp, 0, (OCIRaw**)&val.ptr), "OraColumns::del() free binary buffer", ds, xsink);
		  break;
	       }

	       default:	  // for all columns where data must be allocated
		  if (val.ptr)
		     free(val.ptr);
		  break;
	    }
	    OCIHandleFree(defp, OCI_HTYPE_DEFINE);
	 }	 
      }

      DLLLOCAL inline ~OraColumn() {
      }
      DLLLOCAL AbstractQoreNode *getValue(Datasource *ds, bool horizontal, ExceptionSink *xsink);
};

class OraColumns {
  private:
      int len;
      OraColumn *head, *tail;
      Datasource *ds;
      ExceptionSink *xsink;

   public:
      DLLLOCAL OraColumns(OCIStmt *stmthp, Datasource *n_ds, const char *str, ExceptionSink *n_xsink);
      DLLLOCAL inline ~OraColumns() {
	 OraColumn *w = head;
	 while (w) {
	    head = w->next;
	    w->del(ds, xsink);
	    delete w;
	    w = head;
	 }
      }
      DLLLOCAL inline void add(const char *name, int nlen, int maxsize, ub2 dtype, ub2 char_len) {
	 len++;
	 OraColumn *c = new OraColumn(name, nlen, maxsize, dtype, char_len);

	 if (!tail)
	    head = c;
	 else
	    tail->next = c;
	 tail = c;

	 // printd(5, "column: '%s'\n", c->name);
	 printd(5, "OraColumns::add() %2d name='%s' (max %d) type=%d\n", size(), c->name, c->maxsize, dtype);
      }
      DLLLOCAL inline OraColumn *getHead() {
	 return head;
      }
      DLLLOCAL inline int size() {
	 return len;
      }

      DLLLOCAL void define(OCIStmt *stmthp, const char *str);
};

union ora_bind {
      struct {
	    char *name;                      // name for output hash
	    int maxsize;                     // maximum size, -1 = default for type
	    const char *type;                // qore datatype for column
	    const AbstractQoreNode *value;   // value to be bound, if any
      } ph;
      struct {
	    const AbstractQoreNode *value;   // value to be bound
	    QoreString *tstr;                // temporary string to be deleted
      } v;
};

class OraBindNode {
   public:
      int bindtype;
      union ora_bind data;
      ub2 buftype;
      union ora_value buf; // for bind buffers
      sb2 ind;             // NULL indicator for OCI calls
      OraBindNode *next;
      OCILobLocator *strlob;
      bool clob_allocated;

      DLLLOCAL inline OraBindNode(const AbstractQoreNode *v) : strlob(0), clob_allocated(false) { // for value nodes
	 bindtype = BN_VALUE;
	 data.v.value = v;
	 data.v.tstr = NULL;
	 buftype = 0;
	 next = NULL;
      }
      DLLLOCAL inline OraBindNode(char *name, int size, const char *typ, const AbstractQoreNode *v) : strlob(0), clob_allocated(false) {
	 bindtype = BN_PLACEHOLDER;
	 data.ph.name = name;
	 data.ph.maxsize = size;
	 data.ph.type = typ;
	 data.ph.value = v;
	 buftype = 0;
	 next = NULL;
      }
      DLLLOCAL inline ~OraBindNode() {
      }

      DLLLOCAL void del(Datasource *ds, ExceptionSink *xsink) {
	 if (bindtype == BN_PLACEHOLDER) {
	    if (data.ph.name)
	       free(data.ph.name);

	    // free buffer data if any
	    if ((buftype == SQLT_STR
		 || buftype == SQLT_LBI
		 || buftype == SQLT_VBI)
		&& buf.ptr)
	       free(buf.ptr);
	    else if (buftype == SQLT_LVB) {
	       OracleData *d_ora = (OracleData *)ds->getPrivateData();
	       //printd(5, "freeing binary pointer for SQLT_LVB %p\n", buf.ptr);
	       ora_checkerr(d_ora->errhp, OCIRawResize(d_ora->envhp, d_ora->errhp, 0, (OCIRaw**)&buf.ptr), "OraBindNode::del() free binary buffer", ds, xsink);
	    }
	    else if (buftype == SQLT_RSET && buf.ptr)
	       OCIHandleFree((OCIStmt *)buf.ptr, OCI_HTYPE_STMT);
	    else if ((buftype == SQLT_BLOB || buftype == SQLT_CLOB) && buf.ptr) {
	       //printd(5, "OraBindNode::del() freeing binary pointer for SQLT_*LOB %p\n", buf.ptr);
	       OCIDescriptorFree(buf.ptr, OCI_DTYPE_LOB);
	    }
	    else if (buftype == SQLT_DATE && buf.odt)
	       OCIDescriptorFree(buf.odt, OCI_DTYPE_TIMESTAMP);
	 }
	 else {
	    if (data.v.tstr)
	       delete data.v.tstr;

	    if (strlob) {
	       if (clob_allocated) {
		  OracleData *d_ora = (OracleData *)ds->getPrivateData();
		  //printd(5, "deallocating temporary clob\n");
		  ora_checkerr(d_ora->errhp,
			       OCILobFreeTemporary(d_ora->svchp, d_ora->errhp, strlob),
			       "OraBindNode::del() free temporary CLOB", ds, xsink);
	       }
	       //printd(5, "freeing clob descriptor\n");
	       OCIDescriptorFree(strlob, OCI_DTYPE_LOB);
	    }
	 }
      }

      DLLLOCAL void bindValue(Datasource *ds, OCIStmt *stmthp, int pos, ExceptionSink *xsink);
      DLLLOCAL void bindPlaceholder(Datasource *ds, OCIStmt *stmthp, int pos, ExceptionSink *xsink);
      DLLLOCAL AbstractQoreNode *getValue(Datasource *ds, bool horizontal, ExceptionSink *xsink);
};

class OraBindGroup {
  private:
      int len;
      OraBindNode *head, *tail;
      QoreString *str;
      OCIStmt *stmthp;
      Datasource *ds;
      bool hasOutput;
      bool binding;
      ExceptionSink *xsink;

      DLLLOCAL void parseQuery(const QoreListNode *args);
      DLLLOCAL QoreHashNode *getOutputHash();

      DLLLOCAL void add(OraBindNode *c) {
	 len++;
	 if (!tail)
	    head = c;
	 else
	    tail->next = c;
	 tail = c;
      }

      // exec with auto-reconnect (if possible)
      DLLLOCAL int oci_exec(const char *who, ub4 iters);

   public:
      DLLLOCAL OraBindGroup(Datasource *ods, const QoreString *ostr, const QoreListNode *args, ExceptionSink *n_xsink, bool doBinding=true);
      DLLLOCAL inline ~OraBindGroup() {
	 // free OCI handle
	 if (stmthp)
	    OCIHandleFree(stmthp, OCI_HTYPE_STMT);

	 if (str)
	    delete str;

	 OraBindNode *w = head;
	 while (w) {
	    head = w->next;
	    w->del(ds, xsink);
	    delete w;
	    w = head;
	 }
      }
      DLLLOCAL inline void add(const AbstractQoreNode *v) {
	 OraBindNode *c = new OraBindNode(v);
	 add(c);
	 printd(5, "OraBindGroup::add()\n");
      }
      DLLLOCAL inline void add(char *name, int size, const char *type, const AbstractQoreNode *val) {
	 OraBindNode *c = new OraBindNode(name, size, type, val);
	 add(c);
	 printd(5, "OraBindGroup::add()\n");
	 hasOutput = true;
      }

      DLLLOCAL AbstractQoreNode *exec();
      DLLLOCAL AbstractQoreNode *select();
      DLLLOCAL AbstractQoreNode *selectRows();
};

#endif
