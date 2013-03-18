/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  OraColumnValue.h

  Qore Programming Language

  Copyright (C) 2003 - 2013 David Nichols

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

#ifndef _ORACOLUMNVALUE_H

#define _ORACOLUMNVALUE_H

struct q_lng {
   ub4 size;
   QoreStringNode* str;
};

struct q_lngraw {
   ub4 size;
   BinaryNode* b;
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
   //! named type: object
   OCI_Object *oraObj;
   //! named type: collection
   OCI_Coll *oraColl;
   q_lng lng;
   q_lngraw lngraw;

   DLLLOCAL void* takePtr() {
      void* rv = ptr;
      ptr = 0;
      return rv;
   }

   DLLLOCAL QoreStringNode* takeLongString() {
      assert(lng.str);
      QoreStringNode* rv = lng.str;
      lng.str = 0;
      rv->terminate(rv->size() + lng.size);
      return rv;
   }
};

class QoreOracleStatement;

struct OraColumnValue {
   QoreOracleStatement &stmt;
   union ora_value buf;  // union containing data for value
   ub2 dtype;            // Oracle datatype for value
   int subdtype;         // distinguish the SQLT_NTY subtype
   sb2 ind;              // indicator value

   DLLLOCAL OraColumnValue(QoreOracleStatement &n_stmt, ub2 n_dtype = 0, int n_subdtype = SQLT_NTY_NONE) : stmt(n_stmt), dtype(n_dtype), subdtype(n_subdtype), ind(0) {
   }

   DLLLOCAL void del(ExceptionSink *xsink);

   DLLLOCAL QoreStringNode* doReturnString(bool destructive);

   DLLLOCAL void freeObject();

   DLLLOCAL AbstractQoreNode *getValue(ExceptionSink *xsink, bool horizontal, bool destructive = false);
};

#endif
