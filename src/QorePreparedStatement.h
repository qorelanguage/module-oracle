/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  QorePreparedStatement.h

  Qore Programming Language

  Copyright (C) 2006 - 2015 Qore Technologies, sro
  
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

union ora_tmp_u {
   QoreString* tstr;    // temporary string to be deleted
   BinaryNode* bin;
};

#define OB_PH     0
#define OB_BIND   1

#define OBT_NONE  0
#define OBT_STR   1
#define OBT_BIN   2

struct ora_bind {
protected:
   DLLLOCAL void resetTmp() {
      if (tmp_type == OBT_NONE)
         return;

      switch (tmp_type) {
         case OBT_STR:
            assert(tmp.tstr);
            delete tmp.tstr;
            break;

         case OBT_BIN:
            assert(tmp.bin);
            tmp.bin->deref();
            break;

         default:
            assert(false);
      }

      tmp_type = OBT_NONE;
   }

public:
   unsigned char type;

   // placeholder: output name
   char* ph_name;          // name for output hash
   int ph_maxsize;         // maximum size, -1 = default for type
   char* ph_type;          // qore datatype for column

   unsigned char tmp_type;
   ora_tmp_u tmp;

   DLLLOCAL ora_bind() : type(OB_BIND), ph_name(0), ph_maxsize(0), ph_type(0), tmp_type(OBT_NONE) {
   }

   DLLLOCAL ora_bind(char* name, int size, const char* typ, unsigned char t = OB_PH) 
      : type(t), ph_name(name), 
        ph_maxsize(size), ph_type(typ ? strdup(typ) : 0), tmp_type(OBT_NONE) {
      assert(name);
   }

   DLLLOCAL ~ora_bind() {
      assert(!ph_name);
      assert(!ph_type);
      assert(tmp_type == OBT_NONE);
   }

   DLLLOCAL const char* getName() const {
      assert(ph_name);
      return ph_name;
   }

   DLLLOCAL void resetBind() {
      resetTmp();
   }

   DLLLOCAL void setMaxSize(int ms) {
      assert(type == OB_PH);
      ph_maxsize = ms;
   }

   DLLLOCAL void setType(const char* typ) {
      assert(type == OB_PH);
      if (ph_type)
         free(ph_type);
      ph_type = strdup(typ);
   }

   DLLLOCAL void resetPlaceholder(bool free_name = true) {
      assert(type == OB_PH);

      if (free_name && ph_name) {
         free(ph_name);
         ph_name = 0;
      }

      if (ph_type) {
         free(ph_type);
         ph_type = 0;
      }

      resetTmp();
   }

   DLLLOCAL void save(QoreString* nstr) {
      assert(tmp_type == OBT_NONE);
      tmp_type = OBT_STR;
      tmp.tstr = nstr;
   }
   
   DLLLOCAL void save(BinaryNode* tb) {
      assert(tmp_type == OBT_NONE);
      tmp_type = OBT_BIN;
      tmp.bin = tb;
   }
   
   DLLLOCAL bool isType(const char* t) const {
      assert(type == OB_PH);
      return !strcmp(ph_type, t);
   }

   DLLLOCAL bool isValue() const {
      return type == OB_BIND;
   }

   DLLLOCAL bool isPlaceholder() const {
      return type == OB_PH;
   }
};

class OraBindNode : public OraColumnValue {
protected:
   DLLLOCAL void resetPlaceholder(ExceptionSink* xsink, bool free_name = true);

   DLLLOCAL void resetValue(ExceptionSink* xsink);

   DLLLOCAL void setValue(const AbstractQoreNode* v, ExceptionSink* xsink) {
      if (value)
         value->deref(xsink);
      value = v ? v->refSelf() : 0;
   }

   DLLLOCAL void setPlaceholderIntern(int size, const char* typ, ExceptionSink* xsink) {
      assert(!array);
      data.setMaxSize(size);
      data.setType(typ);
   }

   DLLLOCAL int setPlaceholder(const AbstractQoreNode* v, ExceptionSink* xsink) {
      resetPlaceholder(xsink, false);

      assert(!array);

      // assume string if no argument passed
      if (is_nothing(v)) {
         setPlaceholderIntern(-1, "string", xsink);
         return 0;
      }

      qore_type_t vtype = v->getType();
      if (vtype == NT_HASH) {
         const QoreHashNode* h = reinterpret_cast<const QoreHashNode*>(v);

         int size = -1;
         const AbstractQoreNode* t = h->getKeyValue("value");
         if (t) {
            assert(!value);
            value = t->refSelf();            
         }
         else {
            // get and check size
            const AbstractQoreNode* sz = h->getKeyValue("size");
            size = sz ? sz->getAsInt() : -1;
         }

         // get and check data type
         t = h->getKeyValue("type");
         if (!t) {
            if (value) {
               setPlaceholderIntern(size, value->getTypeName(), xsink);
               return 0;
            }
            xsink->raiseException("DBI-EXEC-EXCEPTION", "missing 'type' key in placeholder hash");
            return -1;
         }

         if (t->getType() != NT_STRING) {
            xsink->raiseException("DBI-EXEC-EXCEPTION", "expecting type name as value of 'type' key, got '%s'", t->getTypeName());
            return -1;
         }
         const QoreStringNode* str = reinterpret_cast<const QoreStringNode*>(t);

         //QoreStringValueHelper strdebug(v);
         //printd(5, "OraBindNode::setPlaceholder() adding placeholder name=%s, size=%d, type=%s, value=%s\n", tstr.getBuffer(), size, str->getBuffer(), strdebug->getBuffer());
         setPlaceholderIntern(size, str->getBuffer(), xsink);
      }
      else if (vtype == NT_STRING)
         setPlaceholderIntern(-1, (reinterpret_cast<const QoreStringNode*>(v))->getBuffer(), xsink);
      else if (vtype == NT_INT)
         setPlaceholderIntern((reinterpret_cast<const QoreBigIntNode*>(v))->val, "string", xsink);
      else {
         xsink->raiseException("DBI-BIND-EXCEPTION", "expecting string or hash for placeholder description, got '%s'", v->getTypeName());
         return -1;
      }

      return 0;
   }

   /*
   DLLLOCAL void setType(const char* typ) {
      data.setType(typ);
   }
   */

   DLLLOCAL void bindListValue(ExceptionSink* xsink, int pos, const AbstractQoreNode* v, bool in_only);
   
   DLLLOCAL void bindValue(ExceptionSink* xsink, int pos, const AbstractQoreNode* v, bool& is_nty, bool in_only = true);
   DLLLOCAL void bindPlaceholder(int pos, bool& is_nty, ExceptionSink* xsink);
   DLLLOCAL int bindDate(int pos, ExceptionSink* xsink);

public:
   // value to be bound, if any
   AbstractQoreNode* value;
   ora_bind data;
   OCILobLocator* strlob;
   OCIBind* bndp;

   bool clob_allocated;

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
   dvoid* pIndicator;
   
   // for value nodes
   DLLLOCAL OraBindNode(QoreOracleStatement& stmt, const AbstractQoreNode* v) : 
      OraColumnValue(stmt), 
      value(v ? v->refSelf() : 0), strlob(0), bndp(0), clob_allocated(false) {
      indicator = 0;
      pIndicator = (dvoid*)&indicator;
   }

   // for placeholder nodes
   DLLLOCAL OraBindNode(QoreOracleStatement& stmt, char* name, int size, const char* typ, const AbstractQoreNode* v = 0) : 
      OraColumnValue(stmt),
      value(v ? v->refSelf() : 0), data(name, size, typ), strlob(0), bndp(0), clob_allocated(false) {
      indicator = 0;
      pIndicator = (dvoid*)&indicator;
   }

   DLLLOCAL ~OraBindNode() {
      assert(!value);
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

   DLLLOCAL void bind(int pos, bool& is_nty, ExceptionSink* xsink);

   DLLLOCAL AbstractQoreNode* getValue(bool horizontal, ExceptionSink* xsink);

   DLLLOCAL int setupDateDescriptor(ExceptionSink* xsink);
};

typedef std::vector<OraBindNode*> node_list_t;

class QorePreparedStatement : public QoreOracleStatement {
protected:
   node_list_t node_list;
   QoreString* str;
   OraResultSet* columns;
   bool hasOutput;
   bool defined;
   bool has_nty;

   DLLLOCAL void parseQuery(const QoreListNode* args, ExceptionSink* xsink);

   DLLLOCAL void add(OraBindNode* c) {
      node_list.push_back(c);
   }

   DLLLOCAL int bindOracle(ExceptionSink* xsink);
   
public:
   //DLLLOCAL QorePreparedStatement(Datasource* ods, const QoreString* ostr, const QoreListNode* args, ExceptionSink* n_xsink, bool doBinding = true);

   DLLLOCAL QorePreparedStatement(Datasource* ods) : QoreOracleStatement(ods), str(0), columns(0), hasOutput(false), defined(false), has_nty(false) {
   }

   DLLLOCAL virtual ~QorePreparedStatement() {
      assert(!stmthp);
      assert(!columns);
      assert(node_list.empty());
   }

   // this virtual function is called when the connection is closed while executing SQL so that
   // the current state can be freed while the driver-specific context data is still present
   DLLLOCAL virtual void clearAbortedConnection(ExceptionSink* xsink) {
      reset(xsink);
   }

   // this virtual function is called after the connection has been closed and reopened while executing SQL
   DLLLOCAL virtual int resetAbortedConnection(ExceptionSink* xsink) {
      // if we have NTYs then we have to reset the statement and cannot recover it in a new logon session
      if (has_nty) {
         reset(xsink);
         return -1;
      }

      return 0;
   }

   DLLLOCAL void reset(ExceptionSink* xsink);
   
   DLLLOCAL int prepare(const QoreString& sql, const QoreListNode* args, bool parse, ExceptionSink* xsink);

   DLLLOCAL OraBindNode* add(const AbstractQoreNode* v) {
      OraBindNode* c = new OraBindNode(*this, v);
      add(c);
      //printd(5, "QorePreparedStatement::add()\n");
      return c;
   }

   DLLLOCAL OraBindNode* add(char* name, int size = -1, const char* type = 0, const AbstractQoreNode* val = 0) {
      OraBindNode* c = new OraBindNode(*this, name, size, type, val);
      add(c);
      //printd(5, "QorePreparedStatement::add()\n");
      hasOutput = true;
      return c;
   }

   DLLLOCAL int bind(const QoreListNode* args, ExceptionSink* xsink);
   DLLLOCAL int bindPlaceholders(const QoreListNode* args, ExceptionSink* xsink);
   DLLLOCAL int bindValues(const QoreListNode* args, ExceptionSink* xsink);

   DLLLOCAL int exec(ExceptionSink* xsink);

   DLLLOCAL int define(ExceptionSink* xsink);

   DLLLOCAL int affectedRows(ExceptionSink* xsink);

   DLLLOCAL QoreHashNode* getOutput(ExceptionSink* xsink) {
      return getOutputHash(false, xsink);
   }

   DLLLOCAL QoreHashNode* getOutputRows(ExceptionSink* xsink) {
      return getOutputHash(true, xsink);
   }

   DLLLOCAL QoreHashNode* fetchRow(ExceptionSink* xsink);
   DLLLOCAL QoreListNode* fetchRows(int rows, ExceptionSink* xsink);
   DLLLOCAL QoreHashNode* fetchColumns(int rows, ExceptionSink* xsink);

#ifdef _QORE_HAS_DBI_DESCRIBE
   DLLLOCAL QoreHashNode* describe(ExceptionSink* xsink);
#endif

   DLLLOCAL AbstractQoreNode* execWithPrologue(bool rows, ExceptionSink* xsink);

#ifdef _QORE_HAS_DBI_SELECT_ROW
   DLLLOCAL QoreHashNode* selectRow(ExceptionSink* xsink);
#endif

   // rows = true means get a list of hashes, otherwise the default is a hash of lists
   DLLLOCAL QoreHashNode* getOutputHash(bool rows, ExceptionSink* xsink);
};

class QorePreparedStatementHelper : public QorePreparedStatement {
protected:
   ExceptionSink* xsink;

public:
   DLLLOCAL QorePreparedStatementHelper(Datasource* ds, ExceptionSink* n_xsink) : QorePreparedStatement(ds), xsink(n_xsink) {
   }
   
   DLLLOCAL ~QorePreparedStatementHelper() {
      reset(xsink);
   }
};

class AbstractDynamicArrayBindData {
protected:
   typedef std::vector<sb2> sb2_list_t;
   sb2_list_t ind_list;
   
   const QoreListNode* l;
   
public:
   DLLLOCAL AbstractDynamicArrayBindData(const QoreListNode* n_l) : l(n_l) {
   }

   DLLLOCAL virtual ~AbstractDynamicArrayBindData() {
   }

   DLLLOCAL int setupBind(OraBindNode& bn, int pos, bool in_only, ExceptionSink* xsink) {
      assert(!l || l->size());
      ind_list.resize(l ? l->size() : 1);

      return setupBindImpl(bn, pos, in_only, xsink);
   }

   DLLLOCAL virtual int setupBindImpl(OraBindNode& bn, int pos, bool in_only, ExceptionSink* xsink) = 0;

   DLLLOCAL void bindNoDataCallback(OCIBind* bindp, ub4 iter, void** bufpp, ub4* alenp, ub1* piecep, void** indp) {
      assert((ind_list.size() + 1) >= iter);
      *bufpp = (void*)0;
      *alenp = 0;
      *piecep = OCI_ONE_PIECE;
      ind_list[iter] = -1;
      *indp = (void*)&ind_list[iter];
      //printd(5, "AbstractDynamicArrayBindData::bindNoDataCallback() iter: %d alen: %d ind: %d\n", iter, alen_list[iter], ind_list[iter]);
   }
   
   DLLLOCAL void bindCallback(OCIBind* bindp, ub4 iter, void** bufpp, ub4* alenp, ub1* piecep, void** indp) {
      assert(!ind_list.empty());
      assert(!l || (ind_list.size() + 1) >= iter);
      *piecep = OCI_ONE_PIECE;
      *indp = (void*)&ind_list[l ? iter : 0];
      //printd(5, "AbstractDynamicArrayBindData::bindCallback() iter: %d alen: %d ind: %d\n", iter, alen_list[iter], ind_list[iter]);
      
      bindCallbackImpl(bindp, iter, bufpp, alenp);
   }
   
   DLLLOCAL virtual void bindCallbackImpl(OCIBind* bindp, ub4 iter, void** bufpp, ub4* alenp) = 0;

   DLLLOCAL int setupOutputBind(OraBindNode& bn, int pos, ExceptionSink* xsink) {
      ind_list.resize(bn.stmt.getArraySize());
      return setupOutputBindImpl(bn, pos, xsink);
   }

   DLLLOCAL virtual int setupOutputBindImpl(OraBindNode& bn, int pos, ExceptionSink* xsink) = 0;

   DLLLOCAL void bindPlaceholderCallback(OCIBind* bindp, ub4 iter, void** bufpp, ub4** alenp, ub1* piecep, void** indp, ub2** rcodepp) {
      *piecep = OCI_ONE_PIECE;
      *indp = (void*)&ind_list[iter];
      *rcodepp = 0;
      //printd(5, "AbstractDynamicArrayBindData::bindCallback() iter: %d alen: %d ind: %d\n", iter, alen_list[iter], ind_list[iter]);
      
      bindPlaceholderCallbackImpl(bindp, iter, bufpp, alenp);
   }
   
   DLLLOCAL virtual void bindPlaceholderCallbackImpl(OCIBind* bindp, ub4 iter, void** bufpp, ub4** alenp) = 0;

   DLLLOCAL AbstractQoreNode* getOutputValue(ExceptionSink* xsink, OraBindNode& bn, bool destructive) {
      return getOutputValueImpl(xsink, bn, destructive);
   }

   DLLLOCAL virtual AbstractQoreNode* getOutputValueImpl(ExceptionSink* xsink, OraBindNode& bn, bool destructive) = 0;

   DLLLOCAL int reset(ExceptionSink* xsink) {
      return resetImpl(xsink);
   }

   DLLLOCAL virtual int resetImpl(ExceptionSink* xsink) = 0;
};

#endif
