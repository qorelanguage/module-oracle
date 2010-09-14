/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  oracle.h

  Qore Programming Language

  Copyright (C) 2003 - 2010 David Nichols

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
#include "oracleobject.h"
#include <qore/Qore.h>

#include <vector>
#include <string>

#include <oci.h>

#include "ocilib_types.h"

#include "OracleData.h"
#include "QoreOracleStatement.h"

#define ORACLE_OBJECT "OracleObject"
#define ORACLE_COLLECTION "OracleCollection"

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

// timestamp binding type
#ifdef _QORE_HAS_TIME_ZONES
// use timestamp with time zone if qore supports time zones
#define QORE_SQLT_TIMESTAMP SQLT_TIMESTAMP_TZ
#define QORE_DTYPE_TIMESTAMP  OCI_DTYPE_TIMESTAMP_TZ
#else
#define QORE_SQLT_TIMESTAMP SQLT_TIMESTAMP
#define QORE_DTYPE_TIMESTAMP  OCI_DTYPE_TIMESTAMP
#endif

#ifdef _QORE_HAS_DBI_EXECRAW
#define ORA_DBI_CAP_HAS_EXECRAW DBI_CAP_HAS_EXECRAW
#else
#define ORA_DBI_CAP_HAS_EXECRAW 0
#endif

#ifdef _QORE_HAS_TIME_ZONES
#define ORA_DBI_CAP_TIME_ZONE_SUPPORT DBI_CAP_TIME_ZONE_SUPPORT
#else
#define ORA_DBI_CAP_TIME_ZONE_SUPPORT 0
#endif

// capabilities of this driver
#define DBI_ORACLE_CAPS (DBI_CAP_TRANSACTION_MANAGEMENT | DBI_CAP_STORED_PROCEDURES | DBI_CAP_CHARSET_SUPPORT | DBI_CAP_LOB_SUPPORT | DBI_CAP_BIND_BY_VALUE | DBI_CAP_BIND_BY_PLACEHOLDER | ORA_DBI_CAP_HAS_EXECRAW | ORA_DBI_CAP_TIME_ZONE_SUPPORT)

// maximum string size for an oracle number
#define ORACLE_NUMBER_STR_LEN 30

//! Support defines to enumerate SQLT_NTY subtype for ORACLE_OBJECT and ORACLE_COLLECTION
#define SQLT_NTY_NONE 0
#define SQLT_NTY_OBJECT 1
#define SQLT_NTY_COLLECTION 2

// FIXME: do not hardcode byte widths - could be incorrect on some platforms
union ora_value {
   void *ptr;
   unsigned char date[7];
   int i4;
   int64 i8;
   double f8;
   OCIDateTime *odt;
   OCIInterval *oi;
   //! named type: object
   OCI_Object * oraObj;
   //! named type: collection
   OCI_Coll *oraColl;

   DLLLOCAL void *takePtr() {
      void *rv = ptr;
      ptr = 0;
      return rv;
   }
};

struct OraValue {
   QoreOracleStatement &stmt;
   union ora_value buf;  // union containing data for value
   ub2 dtype;            // Oracle datatype for value
   int subdtype;         // distinguish the SQLT_NTY subtype
   sb2 ind;              // indicator value

   DLLLOCAL OraValue(QoreOracleStatement &n_stmt, ub2 n_dtype = 0, int n_subdtype = SQLT_NTY_NONE) : stmt(n_stmt), dtype(n_dtype), subdtype(n_subdtype) {
   }

   DLLLOCAL void del(ExceptionSink *xsink);

   DLLLOCAL void freeObject() {
      assert(dtype == SQLT_NTY);
      assert(subdtype == SQLT_NTY_OBJECT || subdtype == SQLT_NTY_COLLECTION);

      OracleData *d_ora = stmt.getData();

      // printd(0, "deleting object (OraColumn) buf.oraObj: %p, buf.oraColl: %p\n", buf.oraObj, buf.oraColl);
      // objects are allocated in bind-methods and it has to be freed in any case
      if (subdtype == SQLT_NTY_OBJECT)
         OCI_ObjectFree2(&d_ora->ocilib, buf.oraObj);
      else
         OCI_CollFree2(&d_ora->ocilib, buf.oraColl);
   }

   DLLLOCAL AbstractQoreNode *getValue(ExceptionSink *xsink, bool horizontal, bool destructive = false);
};

class OraColumn : public OraValue {
public:
   QoreString name;
   int maxsize;
   OCIDefine *defp;     // define handle
   ub2 charlen;
   QoreString subdtypename;

   DLLLOCAL OraColumn(QoreOracleStatement &stmt, const char *n, int len, int ms, ub2 dt, ub2 n_charlen, int subdt = SQLT_NTY_NONE, QoreString subdttn = "")
      : OraValue(stmt, dt, subdt), name(n, len, stmt.getEncoding()), maxsize(ms), defp(0), charlen(n_charlen), subdtypename(subdttn) {
      name.tolwr();
   }

   DLLLOCAL ~OraColumn() {
      assert(!defp);
   }

   DLLLOCAL void del(ExceptionSink *xsink) {
      // printf("DLLLOCAL void del(Datasource *ds, ExceptionSink *xsink)\n");
      if (defp) {
         OraValue::del(xsink);
	 OCIHandleFree(defp, OCI_HTYPE_DEFINE);
         defp = 0;
      }	 
   }

   DLLLOCAL AbstractQoreNode *getValue(bool horizontal, ExceptionSink *xsink) {
      return OraValue::getValue(xsink, horizontal, false);
   }
};

typedef std::vector<OraColumn *> clist_t;

class OraColumns {
protected:
   QoreOracleStatement &stmt;

public:
   clist_t clist;

   DLLLOCAL OraColumns(QoreOracleStatement &n_stmt, const char *str, ExceptionSink *xsink);

   DLLLOCAL ~OraColumns() {
      assert(clist.empty());
   }

   DLLLOCAL void del(ExceptionSink *xsink) {
      for (clist_t::iterator i = clist.begin(), e = clist.end(); i != e; ++i) {
         (*i)->del(xsink);
         delete (*i);
      }
      clist.clear();
   }

   DLLLOCAL void add(const char *name, int nlen, int maxsize, ub2 dtype, ub2 char_len, int subtype=SQLT_NTY_NONE, QoreString subdtn = "") {
      OraColumn *c = new OraColumn(stmt, name, nlen, maxsize, dtype, char_len, subtype, subdtn);

      clist.push_back(c);
      // printd(5, "column: '%s'\n", c->name);
      //printd(0, "OraColumns::add() %2d name='%s' (max %d) type=%d\n", size(), c->name, c->maxsize, dtype);
   }

   DLLLOCAL unsigned size() const {
      return clist.size();
   }

   DLLLOCAL void define(const char *str, ExceptionSink *xsink);
};

class ColumnHelper {
protected:
   OraColumns *col;
   ExceptionSink *xsink;

public:
   DLLLOCAL ColumnHelper(QoreOracleStatement &stmt, const char *str, ExceptionSink *n_xsink) : col(new OraColumns(stmt, str, n_xsink)), xsink(n_xsink) {
   }

   DLLLOCAL ~ColumnHelper() {
      col->del(xsink);
      delete col;
   }

   DLLLOCAL OraColumns *operator*() { return col; }
   DLLLOCAL OraColumns *operator->() { return col; }
};

union ora_bind {
   struct {
      char *name;          // name for output hash
      int maxsize;         // maximum size, -1 = default for type
      char *type;          // qore datatype for column
   } ph;
   struct {
      QoreString *tstr;    // temporary string to be deleted
   } v;
};

class OraBindNode : public OraValue {
protected:
   DLLLOCAL void resetPlaceholder(ExceptionSink *xsink, bool free_name = true);

   DLLLOCAL void resetValue(ExceptionSink *xsink);

   DLLLOCAL void setValue(const AbstractQoreNode *v, ExceptionSink *xsink) {
      if (value)
	 value->deref(xsink);
      value = v ? v->refSelf() : 0;
   }

   DLLLOCAL void setPlaceholderIntern(int size, const char *typ, const AbstractQoreNode *v, ExceptionSink *xsink) {
      setValue(v, xsink);
      data.ph.maxsize = size;
      setType(typ);
   }

   DLLLOCAL int setPlaceholder(const AbstractQoreNode *v, ExceptionSink *xsink) {
      resetPlaceholder(xsink, false);

      // assume string if no argument passed
      if (is_nothing(v)) {
	 setPlaceholderIntern(-1, "string", 0, xsink);
	 return 0;
      }

      qore_type_t vtype = v->getType();
      if (vtype == NT_HASH) {
	 const QoreHashNode *h = reinterpret_cast<const QoreHashNode *>(v);
	 // get and check data type
	 const AbstractQoreNode *t = h->getKeyValue("type");
	 if (!t) {
	    xsink->raiseException("DBI-EXEC-EXCEPTION", "missing 'type' key in placeholder hash");
	    return -1;
	 }

	 const QoreStringNode *str = dynamic_cast<const QoreStringNode *>(t);
	 if (!str) {
	    xsink->raiseException("DBI-EXEC-EXCEPTION", "expecting type name as value of 'type' key, got '%s'", t->getTypeName());
	    return -1;
	 }
	 // get and check size
	 const AbstractQoreNode *sz = h->getKeyValue("size");
	 int size = sz ? sz->getAsInt() : -1;
	       
	 //QoreStringValueHelper strdebug(v);
	 //printd(0, "OraBindGroup::parseQuery() adding placeholder name=%s, size=%d, type=%s, value=%s\n", tstr.getBuffer(), size, str->getBuffer(), strdebug->getBuffer());
	 setPlaceholderIntern(size, str->getBuffer(), h, xsink);
      }
      else if (vtype == NT_STRING)
	 setPlaceholderIntern(-1, (reinterpret_cast<const QoreStringNode *>(v))->getBuffer(), 0, xsink);
      else if (vtype == NT_INT)
	 setPlaceholderIntern((reinterpret_cast<const QoreBigIntNode *>(v))->val, "string", 0, xsink);
      else {
	 xsink->raiseException("DBI-BIND-EXCEPTION", "expecting string or hash for placeholder description, got '%s'", v->getTypeName());
	 return -1;
      }

      return 0;
   }

   DLLLOCAL void setType(const char *typ) {
      if (data.ph.type)
	 free(data.ph.type);
      data.ph.type = strdup(typ);
   }

public:
   // value or placeholder
   int bindtype;

   // value to be bound, if any
   AbstractQoreNode *value;

   union ora_bind data;

   OCILobLocator *strlob;
   bool clob_allocated;
   OCIBind *bndp;

   // for value nodes
   DLLLOCAL inline OraBindNode(QoreOracleStatement &stmt, const AbstractQoreNode *v) : 
      OraValue(stmt), 
      bindtype(BN_VALUE), value(v ? v->refSelf() : 0), strlob(0), clob_allocated(false), bndp(0) {
      data.v.tstr = 0;
   }

   // for placeholder nodes
   DLLLOCAL inline OraBindNode(QoreOracleStatement &stmt, char *name, int size, const char *typ, const AbstractQoreNode *v) : 
      OraValue(stmt),
      bindtype(BN_PLACEHOLDER), value(v ? v->refSelf() : 0), strlob(0), clob_allocated(false), bndp(0) {
      data.ph.name = name;
      data.ph.maxsize = size;
      data.ph.type = typ ? strdup(typ) : 0;
   }

   DLLLOCAL inline ~OraBindNode() {
   }

   DLLLOCAL bool isValue() const {
      return bindtype == BN_VALUE;
   }

   // returns -1 = ERROR, 0 = OK
   DLLLOCAL int set(const AbstractQoreNode *v, ExceptionSink *xsink);
   DLLLOCAL void reset(ExceptionSink *xsink, bool free_name = true);

   DLLLOCAL void bindValue(int pos, ExceptionSink *xsink);

   DLLLOCAL int bindDate(int pos, ExceptionSink *xsink);
   DLLLOCAL void bindPlaceholder(int pos, ExceptionSink *xsink);
   DLLLOCAL AbstractQoreNode *getValue(bool horizontal, ExceptionSink *xsink);

   DLLLOCAL int setupDateDescriptor(ExceptionSink *xsink);

   DLLLOCAL int setDateDescriptor(const DateTime &d, ExceptionSink *xsink);
};


typedef std::vector<OraBindNode *> node_list_t;

class OraBindGroup : public QoreOracleStatement {
protected:
   node_list_t node_list;
   QoreString *str;
   OraColumns *columns;
   bool hasOutput;
   bool defined;

   DLLLOCAL void parseQuery(const QoreListNode *args, ExceptionSink *xsink);

   DLLLOCAL void add(OraBindNode *c) {
      node_list.push_back(c);
   }

   DLLLOCAL int bindOracle(ExceptionSink *xsink);

public:
   //DLLLOCAL OraBindGroup(Datasource *ods, const QoreString *ostr, const QoreListNode *args, ExceptionSink *n_xsink, bool doBinding = true);

   DLLLOCAL OraBindGroup(Datasource *ods) : QoreOracleStatement(ods), str(0), columns(0), hasOutput(false), defined(false) {
   }

   DLLLOCAL ~OraBindGroup() {
      assert(!stmthp);
      assert(!columns);
      assert(node_list.empty());
   }

   DLLLOCAL void reset(ExceptionSink *xsink) {
      QoreOracleStatement::reset(xsink);

      delete str;
      str = 0;

      for (node_list_t::iterator i = node_list.begin(), e = node_list.end(); i != e; ++i) {
	 (*i)->reset(xsink);
	 delete *i;
      }
      node_list.clear();

      if (columns) {
	 columns->del(xsink);
	 delete columns;
	 columns = 0;
      }

      hasOutput = false;
      defined = false;
   }
   
   DLLLOCAL int prepare(const QoreString &sql, const QoreListNode *args, bool parse, ExceptionSink *xsink);

   DLLLOCAL OraBindNode *add(const AbstractQoreNode *v) {
      OraBindNode *c = new OraBindNode(*this, v);
      add(c);
      //printd(5, "OraBindGroup::add()\n");
      return c;
   }

   DLLLOCAL OraBindNode *add(char *name, int size = -1, const char *type = 0, const AbstractQoreNode *val = 0) {
      OraBindNode *c = new OraBindNode(*this, name, size, type, val);
      add(c);
      //printd(5, "OraBindGroup::add()\n");
      hasOutput = true;
      return c;
   }

   DLLLOCAL int bind(const QoreListNode *args, ExceptionSink *xsink);
   DLLLOCAL int bindPlaceholders(const QoreListNode *args, ExceptionSink *xsink);
   DLLLOCAL int bindValues(const QoreListNode *args, ExceptionSink *xsink);

   DLLLOCAL int exec(ExceptionSink *xsink) {
      return execute("OraBindGroup::exec()", xsink);
   }

   DLLLOCAL int define(ExceptionSink *xsink) {
      if (defined) {
         xsink->raiseException("DBI:ORACLE-DEFINE-ERROR", "SQLStatement::define() called twice for the same query");
         return -1;
      }

      if (!columns) {
         columns = new OraColumns(*this, "OraBindGroup::define()", xsink);
         if (*xsink)
            return -1;
      }

      defined = true;
      columns->define("OraBindGroup::define()", xsink);
      return *xsink ? -1 : 0;
   }

   DLLLOCAL int affectedRows(ExceptionSink *xsink);

   DLLLOCAL QoreHashNode *getOutput(ExceptionSink *xsink) {
      return getOutputHash(false, xsink);
   }

   DLLLOCAL QoreHashNode *getOutputRows(ExceptionSink *xsink) {
      return getOutputHash(true, xsink);
   }

   DLLLOCAL bool next(ExceptionSink *xsink);

   DLLLOCAL QoreHashNode *fetchRow(ExceptionSink *xsink);

   DLLLOCAL AbstractQoreNode *execWithPrologue(bool rows, ExceptionSink *xsink);

   DLLLOCAL AbstractQoreNode *select(ExceptionSink *xsink);
   DLLLOCAL AbstractQoreNode *selectRows(ExceptionSink *xsink);

   // rows = true means get a list of hashes, otherwise the default is a hash of lists
   DLLLOCAL QoreHashNode *getOutputHash(bool rows, ExceptionSink *xsink);
};

class OraBindGroupHelper : public OraBindGroup {
protected:
   ExceptionSink *xsink;

public:
   DLLLOCAL OraBindGroupHelper(Datasource *ds, ExceptionSink *n_xsink) : OraBindGroup(ds), xsink(n_xsink) {
   }
   
   DLLLOCAL ~OraBindGroupHelper() {
      reset(xsink);
   }
};

#endif
