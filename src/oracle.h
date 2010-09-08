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

int ora_checkerr(OCIError *errhp, sword status, const char *query_name, Datasource *ds, ExceptionSink *xsink);

DateTimeNode *get_oracle_timestamp(bool get_tz, Datasource *ds, OCIDateTime *odt, ExceptionSink *xsink);


class OracleData {
public:
   OCIEnv *envhp;
   OCIError *errhp;
   OCISvcCtx *svchp;
   ub2 charsetid;
   // "fake" connection for OCILIB stuff
   OCI_Connection *ocilib_cn;

   OCI_Library ocilib;

   DLLLOCAL OracleData() : envhp(0), errhp(0), svchp(0), ocilib_cn(0) {
   }

   DLLLOCAL ~OracleData() {
      // c++ will ignore "delete NULL;"
      delete ocilib_cn;
   }
};

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
   OCI_Coll * oraColl;
};

class OraColumn {
public:
   char *name;
   int maxsize;
   ub2 dtype;           // Oracle datatype for column
   OCIDefine *defp;     // define handle
   sb2 ind;             // indicator value
   ub2 charlen;
   //! distinguish the SQLT_NTY subtype
   int subdtype;
   QoreString subdtypename;

   union ora_value val;

   OraColumn *next;

   DLLLOCAL inline OraColumn(const char *n, int len, int ms, ub2 dt, ub2 n_charlen, int subdt=SQLT_NTY_NONE, QoreString subdttn="") {
      name = (char *)malloc(sizeof(char) * (len + 1));
      strncpy(name, n, len);
      name[len] = '\0';
      strtolower(name);
      maxsize = ms;
      dtype = dt;
      charlen = n_charlen;
      subdtype = subdt;
      subdtypename = subdttn;

      defp = NULL;

      next = NULL;
   }

   DLLLOCAL void del(Datasource *ds, ExceptionSink *xsink) {
//        printf("DLLLOCAL void del(Datasource *ds, ExceptionSink *xsink)\n");
      free(name);
      if (defp) {
// 	 printd(0, "freeing type %d\n", dtype);
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
	    
            case SQLT_NTY: {
	       OracleData *d_ora = (OracleData *)ds->getPrivateData();
//                 printf("Deleting object (OraColumn) val.oraObj: %p, val.oraColl: %p\n", val.oraObj, val.oraColl);
                // objects are allocated in bind-methods and it has to be freed in any case
                if (subdtype == SQLT_NTY_OBJECT)
                    OCI_ObjectFree2(&d_ora->ocilib, val.oraObj);
                else if (subdtype == SQLT_NTY_COLLECTION)
                    OCI_CollFree2(&d_ora->ocilib, val.oraColl);
                else
                    xsink->raiseException("FREE-NTY-ERROR", "An attempt to free unknown NTY type");
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
   DLLLOCAL inline void add(const char *name, int nlen, int maxsize, ub2 dtype, ub2 char_len, int subtype=SQLT_NTY_NONE, QoreString subdtn="") {
      len++;
      OraColumn *c = new OraColumn(name, nlen, maxsize, dtype, char_len, subtype, subdtn);

      if (!tail)
	 head = c;
      else
	 tail->next = c;
      tail = c;

      // printd(5, "column: '%s'\n", c->name);
//       printd(5, "OraColumns::add() %2d name='%s' (max %d) type=%d\n", size(), c->name, c->maxsize, dtype);
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
      char *name;          // name for output hash
      int maxsize;         // maximum size, -1 = default for type
      char *type;          // qore datatype for column
   } ph;
   struct {
      QoreString *tstr;    // temporary string to be deleted
   } v;
};

class AbstractOraBindNode {
protected:
   // value to be bound, if any
   AbstractQoreNode *value;

   // buffer type
   ub2 buftype;

   // NULL indicator for OCI calls
   sb2 ind;

public:
   DLLLOCAL AbstractOraBindNode(AbstractQoreNode *n_value = 0) : value(n_value), buftype(0), ind(0) {
   }

   DLLLOCAL virtual ~AbstractOraBindNode() {
   }

   DLLLOCAL virtual bool isValue() const = 0;
};

class OraBindNode {
public:
   // value or placeholder
   int bindtype;

   // value to be bound, if any
   AbstractQoreNode *value;

   union ora_bind data;

   // buffer type
   ub2 buftype;

   //! distinguish the SQLT_NTY subtype
   int bufsubtype;

   // for bind buffers
   union ora_value buf;

   // NULL indicator for OCI calls
   sb2 ind;

   OCILobLocator *strlob;
   bool clob_allocated;
   OCIBind *bndp;

   // for value nodes
   DLLLOCAL inline OraBindNode(const AbstractQoreNode *v) : 
      bindtype(BN_VALUE), value(v ? v->refSelf() : 0), buftype(0), bufsubtype(SQLT_NTY_NONE), 
      strlob(0), clob_allocated(false), bndp(0) {
      data.v.tstr = 0;
   }

   // for placeholder nodes
   DLLLOCAL inline OraBindNode(char *name, int size, const char *typ, const AbstractQoreNode *v) : 
      bindtype(BN_PLACEHOLDER), value(v ? v->refSelf() : 0), buftype(0), bufsubtype(SQLT_NTY_NONE),
      strlob(0), clob_allocated(false), bndp(0) {
      data.ph.name = name;
      data.ph.maxsize = size;
      data.ph.type = typ ? strdup(typ) : 0;
   }

   DLLLOCAL inline ~OraBindNode() {
   }

   DLLLOCAL bool isValue() const {
      return bindtype == BN_VALUE;
   }

   DLLLOCAL void setValue(const AbstractQoreNode *v, ExceptionSink *xsink) {
      if (value)
	 value->deref(xsink);
      value = v ? v->refSelf() : 0;
   }

   DLLLOCAL void setType(const char *typ) {
      if (data.ph.type)
	 free(data.ph.type);
      data.ph.type = strdup(typ);
   }

   // returns -1 = ERROR, 0 = OK
   DLLLOCAL int set(const AbstractQoreNode *v, ExceptionSink *xsink) {
      if (bindtype == BN_VALUE) {
	 setValue(v, xsink);
	 return *xsink ? -1 : 0;
      }

      // assume string if no argument passed
      if (is_nothing(v)) {
	 set(-1, "string", 0, xsink);
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
	 set(size, str->getBuffer(), h, xsink);
      }
      else if (vtype == NT_STRING)
	 set(-1, (reinterpret_cast<const QoreStringNode *>(v))->getBuffer(), 0, xsink);
      else if (vtype == NT_INT)
	 set((reinterpret_cast<const QoreBigIntNode *>(v))->val, "string", 0, xsink);
      else {
	 xsink->raiseException("DBI-BIND-EXCEPTION", "expecting string or hash for placeholder description, got '%s'", v->getTypeName());
	 return -1;
      }

      return 0;
   }

   DLLLOCAL void set(int size, const char *typ, const AbstractQoreNode *v, ExceptionSink *xsink) {
      setValue(v, xsink);
      data.ph.maxsize = size;
      setType(typ);
   }

   DLLLOCAL void del(Datasource *ds, ExceptionSink *xsink) {
      if (value)
	 value->deref(xsink);

//        printf("DLLLOCAL void OraBindNode::del(Datasource *ds, ExceptionSink *xsink) %d\n", bindtype == BN_PLACEHOLDER);
      if (bindtype == BN_PLACEHOLDER) {
	 if (data.ph.name)
	    free(data.ph.name);

	 if (data.ph.type)
	    free(data.ph.type);

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
         else if (buftype == SQLT_NTY) {
	    OracleData *d_ora = (OracleData *)ds->getPrivateData();

//             printf("Deleting object (OraBindNode BN_PLACEHOLDER) buf.oraObj: %p, buf.oraColl: %p\n", buf.oraObj, buf.oraColl);
            // objects are allocated in bind-methods - placeholder and it has to be freed in any case
            if (bufsubtype == SQLT_NTY_OBJECT)
                OCI_ObjectFree2(&d_ora->ocilib, buf.oraObj);
            else if (bufsubtype == SQLT_NTY_COLLECTION)
                OCI_CollFree2(&d_ora->ocilib, buf.oraColl);
            else
                xsink->raiseException("FREE-NTY-ERROR", "An attempt to free unknown NTY type (BN_PLACEHOLDER)");
         }
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
         else if (buftype == SQLT_NTY) {
//             printf("Deleting object (OraBindNode IN value) buf.oraObj: %p, buf.oraColl: %p (type: %d (obj=1,coll=2,err=0))\n", buf.oraObj, buf.oraColl, bufsubtype);
	    OracleData *d_ora = (OracleData *)ds->getPrivateData();

            if (bufsubtype == SQLT_NTY_OBJECT)
                OCI_ObjectFree2(&d_ora->ocilib, buf.oraObj);
            else if (bufsubtype == SQLT_NTY_COLLECTION)
                OCI_CollFree2(&d_ora->ocilib, buf.oraColl);
            else
                xsink->raiseException("FREE-NTY-ERROR", "An attempt to free unknown NTY type (BN_VALUE)");
         }
      }
   }

   DLLLOCAL void bindValue(Datasource *ds, OCIStmt *stmthp, int pos, ExceptionSink *xsink);

   DLLLOCAL int bindDate(Datasource *ds, OracleData *d_ora, OCIStmt *stmthp, int pos, ExceptionSink *xsink);
   DLLLOCAL void bindPlaceholder(Datasource *ds, OCIStmt *stmthp, int pos, ExceptionSink *xsink);
   DLLLOCAL AbstractQoreNode *getValue(Datasource *ds, bool horizontal, ExceptionSink *xsink);

   DLLLOCAL int setupDateDescriptor(Datasource *ds, OracleData *d_ora, ExceptionSink *xsink);

   DLLLOCAL int setDateDescriptor(Datasource *ds, OracleData *d_ora, const DateTime &d, ExceptionSink *xsink);
};

typedef std::vector<OraBindNode *> node_list_t;

class OraBindGroup {
private:
   node_list_t node_list;
   QoreString *str;
   OCIStmt *stmthp;
   Datasource *ds;
   bool hasOutput;
   bool bound;
   //ExceptionSink *xsink;

   DLLLOCAL void parseQuery(const QoreListNode *args, ExceptionSink *xsink);
   DLLLOCAL QoreHashNode *getOutputHash(ExceptionSink *xsink);

   DLLLOCAL void add(OraBindNode *c) {
      node_list.push_back(c);
   }

   // exec with auto-reconnect (if possible)
   DLLLOCAL int oci_exec(const char *who, ub4 iters, ExceptionSink *xsink);

   DLLLOCAL int bindOracle(ExceptionSink *xsink);

public:
   //DLLLOCAL OraBindGroup(Datasource *ods, const QoreString *ostr, const QoreListNode *args, ExceptionSink *n_xsink, bool doBinding = true);

   DLLLOCAL OraBindGroup(Datasource *ods) : str(0), stmthp(0), ds(ods), hasOutput(false), bound(false) {
   }

   DLLLOCAL ~OraBindGroup() {
      assert(!stmthp);
      assert(node_list.empty());
   }

   DLLLOCAL void reset(ExceptionSink *xsink) {
      // free OCI handle
      if (stmthp) {
	 OCIHandleFree(stmthp, OCI_HTYPE_STMT);
	 stmthp = 0;
      }

      delete str;
      str = 0;

      for (node_list_t::iterator i = node_list.begin(), e = node_list.end(); i != e; ++i) {
	 (*i)->del(ds, xsink);
	 delete *i;
      }
      node_list.clear();
   }
   
   DLLLOCAL int prepare(const QoreString *sql, const QoreListNode *args, bool parse, ExceptionSink *xsink);

   DLLLOCAL OraBindNode *add(const AbstractQoreNode *v) {
      OraBindNode *c = new OraBindNode(v);
      add(c);
      //printd(5, "OraBindGroup::add()\n");
      return c;
   }

   DLLLOCAL OraBindNode *add(char *name, int size = -1, const char *type = "", const AbstractQoreNode *val = 0) {
      OraBindNode *c = new OraBindNode(name, size, type, val);
      add(c);
      //printd(5, "OraBindGroup::add()\n");
      hasOutput = true;
      return c;
   }

   DLLLOCAL int bind(const QoreListNode *args, ExceptionSink *xsink);
   DLLLOCAL int bindPlaceholders(const QoreListNode *args, ExceptionSink *xsink);
   DLLLOCAL int bindValues(const QoreListNode *args, ExceptionSink *xsink);

   DLLLOCAL AbstractQoreNode *exec(ExceptionSink *xsink);
   DLLLOCAL AbstractQoreNode *select(ExceptionSink *xsink);
   DLLLOCAL AbstractQoreNode *selectRows(ExceptionSink *xsink);
};

class OraBindGroupHelper {
protected:
   OraBindGroup &bg;
   ExceptionSink *xsink;

public:
   DLLLOCAL OraBindGroupHelper(OraBindGroup &n_bg, ExceptionSink *n_xsink) : bg(n_bg), xsink(n_xsink) {
   }
   
   DLLLOCAL ~OraBindGroupHelper() {
      bg.reset(xsink);
   }
};

#endif
