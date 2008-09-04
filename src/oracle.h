/*
  oracle.h

  Qore Programming Language

  Copyright (C) 2003, 2004, 2005, 2006

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

#include <qore/Qore.h>

#include "oracle-config.h"

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

      union ora_value val;

      class OraColumn *next;

      DLLLOCAL inline OraColumn(const char *n, int len, int ms, ub2 dt)
      {
	 name = (char *)malloc(sizeof(char) * (len + 1));
	 strncpy(name, n, len);
	 name[len] = '\0';
	 strtolower(name);
	 maxsize = ms;
	 dtype = dt;
	 defp = NULL;

	 next = NULL;
      }
      DLLLOCAL inline ~OraColumn()
      {
	 free(name);
	 if (defp)
	 {
	    switch (dtype)
	    {
	       case SQLT_INT:
	       case SQLT_UIN:
	       case SQLT_FLT:
	       case SQLT_BFLOAT:
	       case SQLT_BDOUBLE:
	       case SQLT_IBFLOAT:
	       case SQLT_IBDOUBLE:
	       case SQLT_DAT:
		  break;

	       case SQLT_CLOB:
	       case SQLT_BLOB:
		  if (val.ptr)
		     OCIDescriptorFree(val.ptr, OCI_DTYPE_LOB);
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

	       default:	  // for all columns where data must be allocated
		  if (val.ptr)
		     free(val.ptr);
		  break;
	    }
	    OCIHandleFree(defp, OCI_HTYPE_DEFINE);
	 }	 
      }
      DLLLOCAL class AbstractQoreNode *getValue(class Datasource *ds, class ExceptionSink *xsink);
};

class OraColumns {
  private:
      int len;
      class OraColumn *head, *tail;

   public:
      DLLLOCAL OraColumns(OCIStmt *stmthp, class Datasource *ds, const char *str, ExceptionSink *xsink);
      DLLLOCAL inline ~OraColumns()
      {
	 class OraColumn *w = head;
	 while (w)
	 {
	    head = w->next;
	    delete w;
	    w = head;
	 }
      }
      DLLLOCAL inline void add(const char *name, int nlen, int maxsize, ub2 dtype)
      {
	 len++;
	 class OraColumn *c = new OraColumn(name, nlen, maxsize, dtype);

	 if (!tail)
	    head = c;
	 else
	    tail->next = c;
	 tail = c;

	 // printd(5, "column: '%s'\n", c->name);
	 printd(5, "OraColumns::add() %2d name='%s' (max %d) type=%d\n", size(), c->name, c->maxsize, dtype);
      }
      DLLLOCAL inline class OraColumn *getHead() 
      {
	 return head;
      }
      DLLLOCAL inline int size()
      {
	 return len;
      }

      DLLLOCAL void define(OCIStmt *stmthp, class Datasource *ds, const char *str, ExceptionSink *xsink);
};

union ora_bind {
      struct {
	    char *name;       // name for output hash
	    int maxsize;      // maximum size, -1 = default for type
	    const char *type;       // qore datatype for column
      } ph;
      struct {
	    const AbstractQoreNode *value;   // value to be bound
	    class QoreString *tstr;   // temporary string to be deleted
      } v;
};

class OraBindNode {
   public:
      int bindtype;
      union ora_bind data;
      ub2 buftype;
      union ora_value buf; // for bind buffers
      sb2 ind;             // NULL indicator for OCI calls
      class OraBindNode *next;

      DLLLOCAL inline OraBindNode(const AbstractQoreNode *v) // for value nodes
      {
	 bindtype = BN_VALUE;
	 data.v.value = v;
	 data.v.tstr = NULL;
	 buftype = 0;
	 next = NULL;
      }
      DLLLOCAL inline OraBindNode(char *name, int size, const char *typ)
      {
	 bindtype = BN_PLACEHOLDER;
	 data.ph.name = name;
	 data.ph.maxsize = size;
	 data.ph.type = typ;
	 buftype = 0;
	 next = NULL;
      }
      DLLLOCAL inline ~OraBindNode()
      {
	 if (bindtype == BN_PLACEHOLDER)
	 {
	    if (data.ph.name)
	       free(data.ph.name);

	    // free buffer data if any
	    if ((buftype == SQLT_STR
		 || buftype == SQLT_LBI
		 || buftype == SQLT_VBI
		 || buftype == SQLT_LVB)
		&& buf.ptr)
	       free(buf.ptr);
	    else if (buftype == SQLT_RSET && buf.ptr)
	       OCIHandleFree((OCIStmt *)buf.ptr, OCI_HTYPE_STMT);
	    else if ((buftype == SQLT_BLOB || buftype ==SQLT_CLOB) && buf.ptr)
	       OCIDescriptorFree(buf.ptr, OCI_DTYPE_LOB);
	    else if (buftype == SQLT_DATE && buf.odt)
	       OCIDescriptorFree(buf.odt, OCI_DTYPE_TIMESTAMP);
	 }
	 else 
	 {
	    if (data.v.tstr)
	       delete data.v.tstr;
	 }
      }

      DLLLOCAL void bindValue(class Datasource *ds, OCIStmt *stmthp, int pos, class ExceptionSink *xsink);
      DLLLOCAL void bindPlaceholder(class Datasource *ds, OCIStmt *stmthp, int pos, class ExceptionSink *xsink);
      DLLLOCAL class AbstractQoreNode *getValue(class Datasource *ds, class ExceptionSink *xsink);
};

class OraBindGroup {
  private:
      int len;
      OraBindNode *head, *tail;
      QoreString *str;
      OCIStmt *stmthp;
      Datasource *ds;
      bool hasOutput;

      DLLLOCAL void parseOld(class QoreHashNode *h, class ExceptionSink *xsink);
      DLLLOCAL void parseQuery(const QoreListNode *args, class ExceptionSink *xsink);
      DLLLOCAL QoreHashNode *getOutputHash(class ExceptionSink *xsink);

      DLLLOCAL void add(class OraBindNode *c)
      {
	 len++;
	 if (!tail)
	    head = c;
	 else
	    tail->next = c;
	 tail = c;
      }

      // exec with auto-reconnect (if possible)
      DLLLOCAL int oci_exec(const char *who, ub4 iters, ExceptionSink *xsink);

   public:
      DLLLOCAL OraBindGroup(class Datasource *ods, const QoreString *ostr, const QoreListNode *args, ExceptionSink *xsink);
      DLLLOCAL inline ~OraBindGroup()
      {
	 // free OCI handle
	 if (stmthp)
	    OCIHandleFree(stmthp, OCI_HTYPE_STMT);

	 if (str)
	    delete str;

	 class OraBindNode *w = head;
	 while (w)
	 {
	    head = w->next;
	    delete w;
	    w = head;
	 }
      }
      DLLLOCAL inline void add(const AbstractQoreNode *v)
      {
	 class OraBindNode *c = new OraBindNode(v);
	 add(c);
	 printd(5, "OraBindGroup::add()\n");
      }
      DLLLOCAL inline void add(char *name, int size, const char *type)
      {
	 class OraBindNode *c = new OraBindNode(name, size, type);
	 add(c);
	 printd(5, "OraBindGroup::add()\n");
	 hasOutput = true;
      }

      DLLLOCAL class AbstractQoreNode *exec(class ExceptionSink *xsink);
      DLLLOCAL class AbstractQoreNode *select(class ExceptionSink *xsink);
      DLLLOCAL class AbstractQoreNode *selectRows(class ExceptionSink *xsink);
};

#endif
