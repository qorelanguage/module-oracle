/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  QorePreparedStatement.h

  Qore Programming Language

  Copyright (C) 2006 - 2014 Qore Technologies, sro
  
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

#ifndef _QOREPREPAREDSTATEMENT_H
#define _QOREPREPAREDSTATEMENT_H

union ora_bind_u {
   struct {
      char* name;          // name for output hash
      int maxsize;         // maximum size, -1 = default for type
      char* type;          // qore datatype for column
   } ph;
   QoreString* tstr;    // temporary string to be deleted
   BinaryNode* bin;
};

#define OBT_PH    0
#define OBT_BIND  1
#define OBT_STR   2
#define OBT_BIN   3

struct ora_bind {
   unsigned char type;
   ora_bind_u v;

   DLLLOCAL ora_bind() : type(OBT_BIND) {
   }

   DLLLOCAL ora_bind(char* name, int size, const char* typ) : type(OBT_PH) {
      v.ph.name = name;
      v.ph.maxsize = size;
      v.ph.type = typ ? strdup(typ) : 0;
   }

   DLLLOCAL ~ora_bind() {
#ifdef DEBUG
      if (type == OBT_PH) {
         assert(!v.ph.name);
         assert(!v.ph.type);
      }
      else
         assert(type == OBT_BIND);
#endif
   }

   DLLLOCAL void resetBind() {
      if (type == OBT_BIND)
         return;

      assert(type != OBT_PH);

      switch (type) {
         case OBT_STR:
            assert(v.tstr);
            delete v.tstr;
            break;
         case OBT_BIN:
            assert(v.bin);
            v.bin->deref();
            break;
      }

      type = OBT_BIND;
   }

   DLLLOCAL void setMaxSize(int ms) {
      assert(type == OBT_PH);
      v.ph.maxsize = ms;
   }

   DLLLOCAL void setType(const char* typ) {
      assert(type == OBT_PH);
      if (v.ph.type)
	 free(v.ph.type);
      v.ph.type = strdup(typ);
   }

   DLLLOCAL void resetPlaceholder(bool free_name = true) {
      assert(type == OBT_PH);

      if (free_name && v.ph.name) {
         free(v.ph.name);
         v.ph.name = 0;
      }

      if (v.ph.type) {
         free(v.ph.type);
         v.ph.type = 0;
      }
   }

   DLLLOCAL void save(QoreString* nstr) {
      assert(type == OBT_BIND);
      type = OBT_STR;
      v.tstr = nstr;
   }
   
   DLLLOCAL void save(BinaryNode* tb) {
      assert(type == OBT_BIND);
      type = OBT_BIN;
      v.bin = tb;
   }
   
   DLLLOCAL bool isType(const char* t) const {
      assert(type == OBT_PH);
      return !strcmp(v.ph.type, t);
   }

   DLLLOCAL bool isValue() const {
      return type != OBT_PH;
   }

   DLLLOCAL bool isPlaceholder() const {
      return type == OBT_PH;
   }
};


class OraBindNode : public OraColumnValue {
protected:
   DLLLOCAL void resetPlaceholder(ExceptionSink* xsink, bool free_name = true);

   DLLLOCAL void resetValue(ExceptionSink* xsink);

   DLLLOCAL void setValue(const AbstractQoreNode *v, ExceptionSink* xsink) {
      if (value)
	 value->deref(xsink);
      value = v ? v->refSelf() : 0;
   }

   DLLLOCAL void setPlaceholderIntern(int size, const char *typ, const AbstractQoreNode *v, ExceptionSink* xsink) {
      setValue(v, xsink);
      data.setMaxSize(size);
      data.setType(typ);
   }

   DLLLOCAL int setPlaceholder(const AbstractQoreNode *v, ExceptionSink* xsink) {
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
	 //printd(5, "OraBindNode::setPlaceholder() adding placeholder name=%s, size=%d, type=%s, value=%s\n", tstr.getBuffer(), size, str->getBuffer(), strdebug->getBuffer());
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

   DLLLOCAL void setType(const char* typ) {
      data.setType(typ);
   }

public:
   // value to be bound, if any
   AbstractQoreNode *value;

   ora_bind data;

   OCILobLocator *strlob;
   bool clob_allocated;
   OCIBind *bndp;

   // Variable indicator is used as a holder for QoreOracleStatement::bindByPos() indp argument
   // which holds information about NULLs in the bound/placeholder-ed value. See:
   //
   // Pointer to an indicator variable or array. For all data types, this is a pointer to sb2 or an array of sb2s
   // The only exception is SQLT_NTY, when this pointer is ignored and the actual pointer to the indicator structure
   //or an array of indicator structures is initialized by OCIBindObject(). Ignored for dynamic binds.
   //
   // It prevents (for example) ORA-01405: fetched column value is NULL errors
   // when is the Type::Date bound to OUT argument of PL/SQL procedure.
   // See: OraBindNode::getValue() code.
   sb2 indicator;
   dvoid *pIndicator;
   
   // for value nodes
   DLLLOCAL inline OraBindNode(QoreOracleStatement& stmt, const AbstractQoreNode* v) : 
      OraColumnValue(stmt), 
      value(v ? v->refSelf() : 0), strlob(0), clob_allocated(false), bndp(0) {
      indicator = 0;
      pIndicator = (dvoid *)&indicator;
   }

   // for placeholder nodes
   DLLLOCAL inline OraBindNode(QoreOracleStatement& stmt, char* name, int size, const char* typ, const AbstractQoreNode* v) : 
      OraColumnValue(stmt),
      value(v ? v->refSelf() : 0), data(name, size, typ), strlob(0), clob_allocated(false), bndp(0) {
      indicator = 0;
      pIndicator = (dvoid*)&indicator;
   }

   DLLLOCAL inline ~OraBindNode() {
   }

   DLLLOCAL bool isValue() const {
      return data.isValue();
   }

   DLLLOCAL bool isPlaceholder() const {
      return data.isPlaceholder();
   }

   // returns -1 = ERROR, 0 = OK
   DLLLOCAL int set(const AbstractQoreNode* v, ExceptionSink* xsink);
   DLLLOCAL void reset(ExceptionSink* xsink, bool free_name = true);

   DLLLOCAL void bindValue(int pos, ExceptionSink* xsink);

   DLLLOCAL int bindDate(int pos, ExceptionSink* xsink);
   DLLLOCAL void bindPlaceholder(int pos, ExceptionSink* xsink);
   DLLLOCAL AbstractQoreNode *getValue(bool horizontal, ExceptionSink* xsink);

   DLLLOCAL int setupDateDescriptor(ExceptionSink* xsink);
};

typedef std::vector<OraBindNode *> node_list_t;

class QorePreparedStatement : public QoreOracleStatement {
protected:
   node_list_t node_list;
   QoreString *str;
   OraResultSet *columns;
   bool hasOutput;
   bool defined;

   DLLLOCAL void parseQuery(const QoreListNode *args, ExceptionSink* xsink);

   DLLLOCAL void add(OraBindNode *c) {
      node_list.push_back(c);
   }

   DLLLOCAL int bindOracle(ExceptionSink* xsink);

public:
   //DLLLOCAL QorePreparedStatement(Datasource *ods, const QoreString *ostr, const QoreListNode *args, ExceptionSink* n_xsink, bool doBinding = true);

   DLLLOCAL QorePreparedStatement(Datasource *ods) : QoreOracleStatement(ods), str(0), columns(0), hasOutput(false), defined(false) {
   }

   DLLLOCAL ~QorePreparedStatement() {
      assert(!stmthp);
      assert(!columns);
      assert(node_list.empty());
   }

   DLLLOCAL void reset(ExceptionSink* xsink);
   
   DLLLOCAL int prepare(const QoreString &sql, const QoreListNode *args, bool parse, ExceptionSink* xsink);

   DLLLOCAL OraBindNode *add(const AbstractQoreNode *v) {
      OraBindNode *c = new OraBindNode(*this, v);
      add(c);
      //printd(5, "QorePreparedStatement::add()\n");
      return c;
   }

   DLLLOCAL OraBindNode *add(char *name, int size = -1, const char *type = 0, const AbstractQoreNode *val = 0) {
      OraBindNode *c = new OraBindNode(*this, name, size, type, val);
      add(c);
      //printd(5, "QorePreparedStatement::add()\n");
      hasOutput = true;
      return c;
   }

   DLLLOCAL int bind(const QoreListNode *args, ExceptionSink* xsink);
   DLLLOCAL int bindPlaceholders(const QoreListNode *args, ExceptionSink* xsink);
   DLLLOCAL int bindValues(const QoreListNode *args, ExceptionSink* xsink);

   DLLLOCAL int exec(ExceptionSink* xsink) {
      return execute("QorePreparedStatement::exec()", xsink);
   }

   DLLLOCAL int define(ExceptionSink* xsink);

   DLLLOCAL int affectedRows(ExceptionSink* xsink);

   DLLLOCAL QoreHashNode *getOutput(ExceptionSink* xsink) {
      return getOutputHash(false, xsink);
   }

   DLLLOCAL QoreHashNode *getOutputRows(ExceptionSink* xsink) {
      return getOutputHash(true, xsink);
   }

   DLLLOCAL QoreHashNode *fetchRow(ExceptionSink* xsink);
   DLLLOCAL QoreListNode *fetchRows(int rows, ExceptionSink* xsink);
   DLLLOCAL QoreHashNode *fetchColumns(int rows, ExceptionSink* xsink);

   DLLLOCAL QoreHashNode *describe(ExceptionSink *xsink);

   DLLLOCAL AbstractQoreNode *execWithPrologue(bool rows, ExceptionSink *xsink);

#ifdef _QORE_HAS_DBI_SELECT_ROW
   DLLLOCAL QoreHashNode *selectRow(ExceptionSink* xsink);
#endif

   // rows = true means get a list of hashes, otherwise the default is a hash of lists
   DLLLOCAL QoreHashNode *getOutputHash(bool rows, ExceptionSink* xsink);
};

class QorePreparedStatementHelper : public QorePreparedStatement {
protected:
   ExceptionSink* xsink;

public:
   DLLLOCAL QorePreparedStatementHelper(Datasource *ds, ExceptionSink* n_xsink) : QorePreparedStatement(ds), xsink(n_xsink) {
   }
   
   DLLLOCAL ~QorePreparedStatementHelper() {
      reset(xsink);
   }
};

#endif
