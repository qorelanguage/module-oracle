/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  OracleData.h

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

#ifndef _QORE_ORACLEDATA_H

#define _QORE_ORACLEDATA_H

class OracleData {
public:
   OCIEnv *envhp;
   OCIError *errhp;
   OCISvcCtx *svchp;
   ub2 charsetid;
   // "fake" connection for OCILIB stuff
   OCI_Connection *ocilib_cn;
   Datasource &ds;

   OCI_Library ocilib;

   DLLLOCAL OracleData(Datasource &n_ds) : envhp(0), errhp(0), svchp(0), ocilib_cn(0), ds(n_ds) {
   }

   DLLLOCAL ~OracleData() {
      // c++ will ignore "delete NULL;"
      delete ocilib_cn;
   }

   DLLLOCAL int checkerr(sword status, const char *query_name, ExceptionSink *xsink);

   DLLLOCAL int descriptorAlloc(void **descpp, unsigned type, const char *who, ExceptionSink *xsink);

   DLLLOCAL int handleAlloc(void **descpp, unsigned type, const char *who, ExceptionSink *xsink);

   DLLLOCAL int commit(ExceptionSink *xsink);
   DLLLOCAL int rollback(ExceptionSink *xsink);

   DLLLOCAL DateTimeNode *getTimestamp(bool get_tz, OCIDateTime *odt, ExceptionSink *xsink);

   DLLLOCAL static void descriptorFree(void *descp, unsigned type) {
      OCIDescriptorFree(descp, type);
   }

   DLLLOCAL static void handleFree(void *hndlp, unsigned type) {
      OCIHandleFree(hndlp, type);
   }
};

#endif
