/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  OraColumnValue.h

  Qore Programming Language

  Copyright (C) 2003 - 2015 David Nichols

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

#include <vector>

struct q_lng {
   const QoreEncoding* enc;
   sb2 ind;
   ub4 size;
   QoreStringNode* str;

   DLLLOCAL q_lng(const QoreEncoding* e) : enc(e), ind(0), size(0), str(0) {
   }

   DLLLOCAL ~q_lng() {
      if (str)
         str->deref();
   }

   DLLLOCAL AbstractQoreNode* takeValue() {
      AbstractQoreNode* rv;
      if (ind)
         rv = null();
      else {
         assert(str);

         QoreStringNode* rvstr = str;
         str = 0;
         rvstr->terminate(rvstr->size() + size);
         rv = rvstr;
         //printd(5, "ora_value::takeLongString() returning str: %p\n", rvstr);
      }

      ind = 0;
      size = 0;

      //printd(5, "q_lng::takeValue() returning %p %s\n", rv, get_type_name(rv));
      return rv;
   }
};

/*
struct q_lngraw {
   ub2 rc;
   sb2 ind;
   ub4 size;
   BinaryNode* b;
};
*/

class QoreOracleStatement;

class AbstractDynamicArrayBindData {
protected:
   typedef std::vector<sb2> sb2_list_t;
   //typedef std::vector<ub2> ub2_list_t;
   typedef std::vector<ub4> ub4_list_t;
   
   const QoreListNode* l;
   
   sb2_list_t ind_list;
   ub4_list_t alen_list;
   
public:
   DLLLOCAL AbstractDynamicArrayBindData(const QoreListNode* n_l) : l(n_l) {
   }

   DLLLOCAL virtual ~AbstractDynamicArrayBindData() {
   }

   DLLLOCAL virtual int setupBind(QoreOracleStatement& stmt, bool in_only, ExceptionSink* xsink) = 0;

   DLLLOCAL void bindCallback(OCIBind* bindp, ub4 iter, void** bufpp, ub4* alenp, ub1* piecep, void** indp) {
      *alenp = alen_list[iter];
      *piecep = OCI_ONE_PIECE;
      *indp = (void*)&ind_list[iter];
      //printd(5, "AbstractDynamicArrayBindData::bindCallback() iter: %d alen: %d ind: %d\n", iter, alen_list[iter], ind_list[iter]);
      
      bindCallbackImpl(bindp, iter, bufpp);
   }
   
   DLLLOCAL virtual void bindCallbackImpl(OCIBind* bindp, ub4 iter, void** bufpp) = 0;
};

// FIXME: do not hardcode byte widths - could be incorrect on some platforms
union ora_value {
   void* ptr;
   unsigned char date[7];
   int i4;
   int64 i8;
   double f8;
   OCIDateTime* odt;
   OCIInterval* oi;
   //! named type: object
   OCI_Object* oraObj;
   //! named type: collection
   OCI_Coll* oraColl;
   q_lng* lng;
   AbstractDynamicArrayBindData* arraybind;
   
   DLLLOCAL void* takePtr() {
      void* rv = ptr;
      ptr = 0;
      return rv;
   }
};

struct OraColumnValue {
   QoreOracleStatement& stmt;
   union ora_value buf;  // union containing data for value
   ub2 dtype;            // Oracle datatype for value
   int subdtype;         // distinguish the SQLT_NTY subtype
   sb2 ind;              // indicator value
   bool array;
   
   DLLLOCAL OraColumnValue(QoreOracleStatement& n_stmt, ub2 n_dtype = 0, int n_subdtype = SQLT_NTY_NONE) : stmt(n_stmt), dtype(n_dtype), subdtype(n_subdtype), ind(0), array(false) {
   }

   DLLLOCAL void del(ExceptionSink* xsink);
   DLLLOCAL QoreStringNode* doReturnString(bool destructive);
   DLLLOCAL void freeObject(ExceptionSink* xsink);
   DLLLOCAL AbstractQoreNode* getValue(ExceptionSink* xsink, bool horizontal, bool destructive = false);
};

#endif
