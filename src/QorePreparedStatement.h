/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  QorePreparedStatement.h

  Qore Programming Language

  Copyright (C) 2006 - 2010 Qore Technologies, sro
  
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

class OraBindNode : public OraColumnValue {
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
      OraColumnValue(stmt), 
      bindtype(BN_VALUE), value(v ? v->refSelf() : 0), strlob(0), clob_allocated(false), bndp(0) {
      data.v.tstr = 0;
   }

   // for placeholder nodes
   DLLLOCAL inline OraBindNode(QoreOracleStatement &stmt, char *name, int size, const char *typ, const AbstractQoreNode *v) : 
      OraColumnValue(stmt),
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
};

typedef std::vector<OraBindNode *> node_list_t;

class QorePreparedStatement : public QoreOracleStatement {
protected:
   node_list_t node_list;
   QoreString *str;
   OraResultSet *columns;
   bool hasOutput;
   bool defined;

   DLLLOCAL void parseQuery(const QoreListNode *args, ExceptionSink *xsink);

   DLLLOCAL void add(OraBindNode *c) {
      node_list.push_back(c);
   }

   DLLLOCAL int bindOracle(ExceptionSink *xsink);

public:
   //DLLLOCAL QorePreparedStatement(Datasource *ods, const QoreString *ostr, const QoreListNode *args, ExceptionSink *n_xsink, bool doBinding = true);

   DLLLOCAL QorePreparedStatement(Datasource *ods) : QoreOracleStatement(ods), str(0), columns(0), hasOutput(false), defined(false) {
   }

   DLLLOCAL ~QorePreparedStatement() {
      assert(!stmthp);
      assert(!columns);
      assert(node_list.empty());
   }

   DLLLOCAL void reset(ExceptionSink *xsink);
   
   DLLLOCAL int prepare(const QoreString &sql, const QoreListNode *args, bool parse, ExceptionSink *xsink);

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

   DLLLOCAL int bind(const QoreListNode *args, ExceptionSink *xsink);
   DLLLOCAL int bindPlaceholders(const QoreListNode *args, ExceptionSink *xsink);
   DLLLOCAL int bindValues(const QoreListNode *args, ExceptionSink *xsink);

   DLLLOCAL int exec(ExceptionSink *xsink) {
      return execute("QorePreparedStatement::exec()", xsink);
   }

   DLLLOCAL int define(ExceptionSink *xsink);

   DLLLOCAL int affectedRows(ExceptionSink *xsink);

   DLLLOCAL QoreHashNode *getOutput(ExceptionSink *xsink) {
      return getOutputHash(false, xsink);
   }

   DLLLOCAL QoreHashNode *getOutputRows(ExceptionSink *xsink) {
      return getOutputHash(true, xsink);
   }

   DLLLOCAL QoreHashNode *fetchRow(ExceptionSink *xsink);
   DLLLOCAL QoreListNode *fetchRows(int rows, ExceptionSink *xsink);
   DLLLOCAL QoreHashNode *fetchColumns(int rows, ExceptionSink *xsink);

   DLLLOCAL AbstractQoreNode *execWithPrologue(bool rows, ExceptionSink *xsink);

   // rows = true means get a list of hashes, otherwise the default is a hash of lists
   DLLLOCAL QoreHashNode *getOutputHash(bool rows, ExceptionSink *xsink);
};

class QorePreparedStatementHelper : public QorePreparedStatement {
protected:
   ExceptionSink *xsink;

public:
   DLLLOCAL QorePreparedStatementHelper(Datasource *ds, ExceptionSink *n_xsink) : QorePreparedStatement(ds), xsink(n_xsink) {
   }
   
   DLLLOCAL ~QorePreparedStatementHelper() {
      reset(xsink);
   }
};

#endif
