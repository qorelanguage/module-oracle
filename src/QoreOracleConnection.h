/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  QoreOracleConnection.h

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

#define QORE_OCI_FLAGS (OCI_DEFAULT|OCI_THREADED|OCI_NO_MUTEX|OCI_OBJECT)

class QoreOracleEnvironment {
protected:
   OCIEnv *envhp;

public:
   DLLLOCAL QoreOracleEnvironment() : envhp(0) {
   }

   DLLLOCAL ~QoreOracleEnvironment() {
      if (envhp)
         OCIHandleFree(envhp, OCI_HTYPE_ENV);
   }

   DLLLOCAL int init() {
      return OCIEnvCreate(&envhp, QORE_OCI_FLAGS | OCI_NO_UCB, 0, 0, 0, 0, 0, 0) == OCI_SUCCESS ? 0 : -1;
   }

   DLLLOCAL int init(unsigned short charset) {
      return OCIEnvNlsCreate(&envhp, QORE_OCI_FLAGS | OCI_NO_UCB, 0, 0, 0, 0, 0, 0, charset, charset) == OCI_SUCCESS ? 0 : -1;
   }

   DLLLOCAL int nlsNameMap(const char *name, QoreString &out) {
      assert(envhp);

      out.clear();
      out.reserve(OCI_NLS_MAXBUFSZ);

      return OCINlsNameMap(envhp, (oratext *)out.getBuffer(), OCI_NLS_MAXBUFSZ, (oratext *)name, OCI_NLS_CS_IANA_TO_ORA) == OCI_SUCCESS ? 0 : -1;
   }

   DLLLOCAL unsigned short nlsCharSetNameToId(const char *name) {
      assert(envhp);
      return OCINlsCharSetNameToId(envhp, (oratext *)name);
   }

   DLLLOCAL operator bool() const {
      return envhp;
   }

   DLLLOCAL OCIEnv *operator*() const {
      return envhp;
   }
};

class QoreOracleConnection {
  protected:
   DLLLOCAL static sb4 readClobCallback(void *sp, CONST dvoid *bufp, ub4 len, ub1 piece) {
      //printd(5, "QoreOracleConnection::readClobCallback(%p, %p, %d, %d)\n", sp, bufp, len, piece);
      (reinterpret_cast<QoreStringNode *>(sp))->concat((char *)bufp, len);
      return OCI_CONTINUE;
   }

   DLLLOCAL static sb4 readBlobCallback(void *bp, CONST dvoid *bufp, ub4 len, ub1 piece) {
      //printd(5, "QoreOracleConnection::readBlobCallback(%p, %p, %d, %d)\n", bp, bufp, len, piece);
      ((BinaryNode *)bp)->append((char *)bufp, len);
      return OCI_CONTINUE;
   }

public:
   QoreOracleEnvironment env;
   OCIError *errhp;
   OCISvcCtx *svchp;
   ub2 charsetid;
   // "fake" connection for OCILIB stuff
   OCI_Connection *ocilib_cn;
   Datasource &ds;
   bool ocilib_init;

   OCI_Library ocilib;

   QoreString cstr; // connection string

   DLLLOCAL QoreOracleConnection(Datasource &n_ds, ExceptionSink *xsink);
   DLLLOCAL ~QoreOracleConnection();

   DLLLOCAL int checkerr(sword status, const char *query_name, ExceptionSink *xsink);

   DLLLOCAL int descriptorAlloc(void **descpp, unsigned type, const char *who, ExceptionSink *xsink);

   DLLLOCAL int handleAlloc(void **descpp, unsigned type, const char *who, ExceptionSink *xsink);

   DLLLOCAL int logon(ExceptionSink *xsink) {
      const std::string &user = ds.getUsernameStr();
      const std::string &pass = ds.getPasswordStr();
      return checkerr(OCILogon(*env, errhp, &svchp, (text *)user.c_str(), user.size(), (text *)pass.c_str(), pass.size(), (text *)cstr.getBuffer(), cstr.strlen()), "QoreOracleConnection::logon()", xsink);
   }

   // logoff but do not process error return values
   DLLLOCAL int logoff() {
      assert(svchp);
      int rc = OCILogoff(svchp, errhp);
      OCIHandleFree(svchp, OCI_HTYPE_SVCCTX);
      svchp = 0;
      return rc;
   }

   DLLLOCAL int commit(ExceptionSink *xsink);
   DLLLOCAL int rollback(ExceptionSink *xsink);

   DLLLOCAL DateTimeNode *getTimestamp(bool get_tz, OCIDateTime *odt, ExceptionSink *xsink);

   DLLLOCAL DateTimeNode *getIntervalYearMonth(OCIInterval *oi, ExceptionSink *xsink) {
      sb4 year, month;
      if (checkerr(OCIIntervalGetYearMonth(*env, errhp, &year, &month, oi), "QoreOracleConnection::getIntervalYearMonth()", xsink))
	 return 0;

      return new DateTimeNode(year, month, 0, 0, 0, 0, 0, true);
   }

   DLLLOCAL DateTimeNode *getIntervalDaySecond(OCIInterval *oi, ExceptionSink *xsink) {
	 //printd(5, "QoreOracleConnection::getIntervalDaySecond() using INTERVAL_DS handle %p\n", buf.oi);
	 sb4 day, hour, minute, second, nanosecond;
	 if (checkerr(OCIIntervalGetDaySecond(*env, errhp, &day, &hour, &minute, &second, &nanosecond, oi), "QoreOracleConnection::getIntervalDaySecond()", xsink))
	    return 0;

#ifdef _QORE_HAS_TIME_ZONES
	 return DateTimeNode::makeRelative(0, 0, day, hour, minute, second, nanosecond / 1000);
#else
	 return new DateTimeNode(0, 0, day, hour, minute, second, nanosecond / 1000000, true);
#endif
   }

   DLLLOCAL BinaryNode *getBinary(OCIRaw *rawp) {
      BinaryNode *b = new BinaryNode;
      b->append(OCIRawPtr(*env, rawp), OCIRawSize(*env, rawp));
      return b;
   }

   DLLLOCAL int rawResize(OCIRaw **rawp, unsigned short size, ExceptionSink *xsink) {
      return checkerr(OCIRawResize(*env, errhp, size, rawp), "QoreOracleConnection::rawResize()", xsink);
   }

   DLLLOCAL int rawFree(OCIRaw **rawp, ExceptionSink *xsink) {
      return rawResize(rawp, 0, xsink);
   }

   DLLLOCAL BinaryNode *readBlob(OCILobLocator *lobp, ExceptionSink *xsink);
   DLLLOCAL QoreStringNode *readClob(OCILobLocator *lobp, const QoreEncoding *enc, ExceptionSink *xsink);

   DLLLOCAL static void descriptorFree(void *descp, unsigned type) {
      OCIDescriptorFree(descp, type);
   }

   DLLLOCAL static void handleFree(void *hndlp, unsigned type) {
      OCIHandleFree(hndlp, type);
   }
};

#endif
