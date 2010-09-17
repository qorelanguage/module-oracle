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

#include "oracle.h"

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
   QoreOracleConnection *conn = (QoreOracleConnection *)stmt.getData();

   dtype = QORE_SQLT_TIMESTAMP;
   buf.odt = NULL;

   if (conn->descriptorAlloc((dvoid **)&buf.odt, QORE_DTYPE_TIMESTAMP, "OraBindNode::setupDateDecriptor()", xsink))
      return -1;
   return 0;
}

int OraBindNode::bindDate(int pos, ExceptionSink *xsink) {
   return stmt.bindByPos(bndp, pos, &buf.odt, 0, QORE_SQLT_TIMESTAMP, xsink);
}

void OraBindNode::bindValue(int pos, ExceptionSink *xsink) {
   QoreOracleConnection *conn = (QoreOracleConnection *)stmt.getData();

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
	 if (conn->descriptorAlloc((dvoid **)&strlob, OCI_DTYPE_LOB, "OraBindNode::bindValue() alloc LOB descriptor", xsink))
	    return;

	 // create temporary BLOB
	 if (conn->checkerr(OCILobCreateTemporary(conn->svchp, conn->errhp, strlob, OCI_DEFAULT, OCI_DEFAULT, OCI_TEMP_CLOB, FALSE, OCI_DURATION_SESSION),
                             "OraBindNode::bindValue() create temporary CLOB", xsink))
	    return;

	 clob_allocated = true;

	 // write the buffer data into the CLOB
	 ub4 amtp = len;
	 if (conn->checkerr(OCILobWrite(conn->svchp, conn->errhp, strlob, &amtp, 1, buf.ptr, len + 1, OCI_ONE_PIECE, 0, 0, conn->charsetid, SQLCS_IMPLICIT),
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

      if (conn->dateTimeConstruct(buf.odt, *d, xsink))
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
           buf.oraObj = objBindQore(conn, h, xsink);
           if (*xsink || !buf.oraObj)
               return;

           stmt.bindByPos(bndp, pos, 0, 0, SQLT_NTY, xsink);

           conn->checkerr(OCIBindObject(bndp, conn->errhp,
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
           buf.oraColl = collBindQore(conn, h, xsink);
           if (*xsink || !buf.oraColl)
               return;

           stmt.bindByPos(bndp, pos, 0, 0, SQLT_NTY, xsink);
      
           conn->checkerr(OCIBindObject(bndp, conn->errhp,
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
   QoreOracleConnection *conn = (QoreOracleConnection *)stmt.getData();

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

      if (setupDateDescriptor(xsink))
	 return;

      if (value) {
	 DateTimeNodeValueHelper d(value);

	 if (conn->dateTimeConstruct(buf.odt, **d, xsink))
	    return;
      }

      //conn->checkerr(OCIBindByPos(stmthp, &bndp, conn->errhp, pos, &buf.odt, 0, QORE_SQLT_TIMESTAMP, &ind, (ub2 *)NULL, (ub2 *)NULL, (ub4)0, (ub4 *)NULL, OCI_DEFAULT), "OraBindNode::bindPlaceholder() timestamp", xsink);
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

	    if (conn->checkerr(OCIRawAssignBytes(*conn->env, conn->errhp, (const ub1*)bin->getPtr(), bin->size(), (OCIRaw**)&buf.ptr), "OraBindNode::bindPlaceholder() bind binary value", xsink)) {
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

	    if (conn->checkerr(OCIRawAssignBytes(*conn->env, conn->errhp, (const ub1*)str->getBuffer(), str->strlen(), (OCIRaw**)&buf.ptr), "OraBindNode::bindPlaceholder() bind binary value from string", xsink))
	       return;
	 }
      }
      else {
	 data.ph.maxsize = ORA_RAW_SIZE;
	 buf.ptr = 0;

	 if (conn->checkerr(OCIRawResize(*conn->env, conn->errhp, ORA_RAW_SIZE, (OCIRaw**)&buf.ptr), "OraBindNode::bindPlaceholder() setup binary placeholder", xsink))
	    return;
      }

      stmt.bindByPos(bndp, pos, buf.ptr, ORA_RAW_SIZE, SQLT_LVB, xsink, &ind);
   }
   else if (!strcmp(data.ph.type, "clob")) {
      dtype = SQLT_CLOB;
      buf.ptr = NULL;

      if (conn->descriptorAlloc(&buf.ptr, OCI_DTYPE_LOB, "OraBindNode::bindPlaceholder() allocate clob descriptor", xsink))
	 return;

      //       printd(5, "OraBindNode::bindPlaceholder() got LOB locator handle %p\n", buf.ptr);
      stmt.bindByPos(bndp, pos, &buf.ptr, 0, SQLT_CLOB, xsink, &ind);
   }
   else if (!strcmp(data.ph.type, "blob")) {
      dtype = SQLT_BLOB;
      buf.ptr = NULL;

      conn->descriptorAlloc(&buf.ptr, OCI_DTYPE_LOB, "OraBindNode::bindPlaceholder() allocate blob descriptor", xsink);

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
      if (conn->handleAlloc(&buf.ptr, OCI_HTYPE_STMT, "OraBindNode::bindPlaceHolder() allocate statement handle", xsink))
	 buf.ptr = 0;
      else
         stmt.bindByPos(bndp, pos, &buf.ptr, 0, SQLT_RSET, xsink, &ind);
   }
   else if (!strcmp(data.ph.type, ORACLE_OBJECT)) {
       subdtype = SQLT_NTY_OBJECT;
       dtype = SQLT_NTY;

       const QoreHashNode * h = reinterpret_cast<const QoreHashNode*>(value);
       if (h->existsKey("^values^"))
           buf.oraObj = objBindQore(conn, h, xsink); // IN/OUT
       else {
           const QoreStringNode * str = reinterpret_cast<const QoreStringNode*>(h->getKeyValue("value"));
           buf.oraObj = objPlaceholderQore(conn, str->getBuffer(), xsink); // IN
       }

       if (*xsink)
           return;

       stmt.bindByPos(bndp, pos,  0, 0, SQLT_NTY, xsink, &ind);

       conn->checkerr(OCIBindObject(bndp, conn->errhp,
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
           buf.oraColl = collBindQore(conn, h, xsink); // IN/OUT
       else {
           const QoreStringNode * str = reinterpret_cast<const QoreStringNode*>(h->getKeyValue("value"));
           buf.oraColl = collPlaceholderQore(conn, str->getBuffer(), xsink); // IN
       }

       if (*xsink)
           return;

       stmt.bindByPos(bndp, pos,  0, 0, SQLT_NTY, xsink, &ind);

       conn->checkerr(
                    OCIBindObject(bndp, conn->errhp,
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
         QoreOracleConnection *conn = stmt.getData();
         //printd(5, "deallocating temporary clob\n");
         conn->checkerr(OCILobFreeTemporary(conn->svchp, conn->errhp, strlob), "OraBindNode::resetValue() free temporary CLOB", xsink);
      }
      //printd(5, "freeing clob descriptor\n");
      OCIDescriptorFree(strlob, OCI_DTYPE_LOB);
   }
   else if (dtype == SQLT_NTY)
      freeObject();
}

AbstractQoreNode *OraBindNode::getValue(bool horizontal, ExceptionSink *xsink) {
   return OraColumnValue::getValue(xsink, horizontal, true);
}

void QorePreparedStatement::reset(ExceptionSink *xsink) {
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

int QorePreparedStatement::define(ExceptionSink *xsink) {
   if (defined) {
      xsink->raiseException("DBI:ORACLE-DEFINE-ERROR", "SQLStatement::define() called twice for the same query");
      return -1;
   }

   if (!columns) {
      columns = new OraResultSet(*this, "QorePreparedStatement::define()", xsink);
      if (*xsink)
	 return -1;
   }

   defined = true;
   columns->define("QorePreparedStatement::define()", xsink);
   return *xsink ? -1 : 0;
}

int QorePreparedStatement::prepare(const QoreString &sql, const QoreListNode *args, bool parse, ExceptionSink *xsink) {
   // create copy of string and convert encoding if necessary
   str = sql.convertEncoding(getEncoding(), xsink);
   if (*xsink)
      return -1;

   //printd(4, "QorePreparedStatement::prepare() ds=%p, conn=%p, SQL='%s', args=%d\n", ds, conn, str->getBuffer(), args ? args->size() : 0);

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

int QorePreparedStatement::bindOracle(ExceptionSink *xsink) {
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
   //printd(0, "QorePreparedStatement::bindOracle() bound %d position(s), statement handle: %p\n", pos - 1, stmthp);

   return 0;
}

int QorePreparedStatement::bind(const QoreListNode *args, ExceptionSink *xsink) {
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

int QorePreparedStatement::bindPlaceholders(const QoreListNode *args, ExceptionSink *xsink) {
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

int QorePreparedStatement::bindValues(const QoreListNode *args, ExceptionSink *xsink) {
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

void QorePreparedStatement::parseQuery(const QoreListNode *args, ExceptionSink *xsink) {
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

	 // 	 printd(5, "QorePreparedStatement::parseQuery() newstr=%s\n", str->getBuffer());
	 // 	 printd(5, "QorePreparedStatement::parseQuery() adding value type=%s\n",v ? v->getTypeName() : "<NULL>");
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

QoreHashNode *QorePreparedStatement::getOutputHash(bool rows, ExceptionSink *xsink) {
   ReferenceHolder<QoreHashNode> h(new QoreHashNode, xsink);

   for (node_list_t::iterator i = node_list.begin(), e = node_list.end(); i != e; ++i) {
      if ((*i)->bindtype == BN_PLACEHOLDER)
	 h->setKeyValue((*i)->data.ph.name, (*i)->getValue(rows, xsink), xsink);
   }

   return *xsink ? 0 : h.release();
}

AbstractQoreNode *QorePreparedStatement::execWithPrologue(bool rows, ExceptionSink *xsink) {
   if (exec(xsink))
      return 0;

   ReferenceHolder<AbstractQoreNode> rv(xsink);

   // if there are output variables, then fix values if necessary and return
   if (is_select) {
      if (rows)
	 rv = QoreOracleStatement::fetchRows(xsink);
      else
         rv = QoreOracleStatement::fetchColumns(xsink);

      if (*xsink)
	 return 0;
   } else if (hasOutput)
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

int QorePreparedStatement::affectedRows(ExceptionSink *xsink) {
   int rc = 0;
   getData()->checkerr(OCIAttrGet(stmthp, OCI_HTYPE_STMT, &rc, 0, OCI_ATTR_ROW_COUNT, getData()->errhp), "QorePreparedStatement::affectedRows()", xsink);
   return rc;
}

QoreHashNode *QorePreparedStatement::fetchRow(ExceptionSink *xsink) {
   assert(columns);
   return QoreOracleStatement::fetchRow(*columns, xsink);
}

QoreListNode *QorePreparedStatement::fetchRows(int rows, ExceptionSink *xsink) {
   assert(columns);
   return QoreOracleStatement::fetchRows(*columns, rows, xsink);
}

QoreHashNode *QorePreparedStatement::fetchColumns(int rows, ExceptionSink *xsink) {
   assert(columns);
   return QoreOracleStatement::fetchColumns(*columns, rows, xsink);
}

