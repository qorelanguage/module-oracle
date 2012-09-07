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

//#define QORE_OCI_FLAGS (OCI_DEFAULT|OCI_THREADED|OCI_NO_MUTEX|OCI_OBJECT)
#define QORE_OCI_FLAGS (OCI_DEFAULT|OCI_THREADED|OCI_OBJECT)

#ifndef VERSION_BUF_SIZE
#define VERSION_BUF_SIZE 512
#endif

// date format used when creating OCIDateTime values with year < 2 as OCIDateTimeConstruct will fail
#define ORA_BACKUP_DATE_FMT "YYYYMMDDHH24MISSFF6"

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

   DLLLOCAL int nlsNameMapToOracle(const char *name, QoreString &out) {
      return nlsNameMap(name, out, OCI_NLS_CS_IANA_TO_ORA);
   }

   DLLLOCAL int nlsNameMapToQore(const char *name, QoreString &out) {
      return nlsNameMap(name, out, OCI_NLS_CS_ORA_TO_IANA);
   }

   DLLLOCAL int nlsNameMap(const char *name, QoreString &out, int dir) {
      assert(envhp);

      out.clear();
      out.reserve(OCI_NLS_MAXBUFSZ);
      
      int rc = OCINlsNameMap(envhp, (oratext *)out.getBuffer(), OCI_NLS_MAXBUFSZ, (oratext *)name, dir) == OCI_SUCCESS ? 0 : -1;
      if (!rc)
         out.terminate(strlen(out.getBuffer()));
      return rc;
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
   
   DLLLOCAL int checkWarnings(ExceptionSink *xsink) {
      ub4 ix = 1;
      int errcode;
      text errbuf[512];
#ifdef DARWIN
      bool hasError = false;
#endif
      while (OCIErrorGet(errhp, ix, (text *) NULL, &errcode, errbuf, (ub4)sizeof(errbuf), OCI_HTYPE_ERROR) != OCI_NO_DATA) {
         fprintf(stderr, "Oracle OCI Warning - %.*s\n", 512, errbuf);
#ifdef DARWIN
         hasError = true;
#endif
         ++ix;
      }
     
#ifdef DARWIN
      // ORA-28002: the password will expire within 20 days and probably all other warnings
      // cause crashes (invalid handle) on macosx (client 10). I'm not sure if it's mac
      // issue or client issue (all linux versions are running on 11 here)
      if (hasError) {
         char output[512];
         sprintf(output, "Oracle OCI Warning - %.*s", 512, errbuf);
         xsink->raiseException("OCI-WARNING-ERROR", output);
         return -1;
      }
#endif
      return 0;
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

   DLLLOCAL int dateTimeConstruct(OCIDateTime *odt, short year, unsigned char month, unsigned char day, unsigned char hour, unsigned char minute,
                                  unsigned char second, unsigned ns, const char *tz, ExceptionSink *xsink) {
      return checkerr(OCIDateTimeConstruct(*env, errhp, odt, year, month, day, hour, minute, second, ns, 0, 0), "QoreOracleConnection::dateTimeConstruct()", xsink);
   }

   DLLLOCAL int dateTimeConstruct(OCIDateTime *odt, const DateTime &d, ExceptionSink *xsink) {
#ifdef _QORE_HAS_TIME_ZONES
      // get broken-down time information in the current time zone
      qore_tm info;
      d.getInfo(currentTZ(), info);

      // only use OCIDateTimeConstruct if the year > 0001
      if (info.year > 1) {
         char tz[7];

         // setup time zone string
         int se = info.utc_secs_east;

         if (se < 0) {
            tz[0] = '-';
            se = -se;
         }
         else
            tz[0] = '+';

         int hours = se / 3600;
         sprintf(&tz[1], "%02d:", hours);

         se %= 3600;
         sprintf(&tz[4], "%02d", se / 60);   
         tz[6] = '\0';

         //printd(5, "QoreOracleConnection::dateTimeConstruct(year=%d, month=%d, day=%d, hour=%d, minute=%d, second=%d, us=%d, tz=%s) %s\n", info.year, info.month, info.day, info.hour, info.minute, info.second, info.us, tz, info.regionName());
         return checkerr(OCIDateTimeConstruct(*env, errhp, odt, info.year, info.month, info.day, info.hour, info.minute, info.second, (info.us * 1000), (oratext *)tz, 6), "QoreOracleConnection::dateTimeConstruct()", xsink);
      }
      
      QoreString dstr;
      dstr.sprintf("%04d%02d%02d%02d%02d%06d", info.year, info.month, info.day, info.hour, info.minute, info.second, info.us);

      //printd(5, "QoreOracleConnection::dateTimeConstruct() d=%s (%s)\n", dstr.getBuffer(), ORA_BACKUP_DATE_FMT);

      return checkerr(OCIDateTimeFromText(*env, errhp, (OraText *)dstr.getBuffer(),
                                          dstr.strlen(), (OraText *)ORA_BACKUP_DATE_FMT,
                                          sizeof(ORA_BACKUP_DATE_FMT), 0, 0, odt), "QoreORacleConnection::dateTimeConstruct() fromText", xsink);
#else
      return checkerr(OCIDateTimeConstruct(*env, errhp, odt, d.getYear(), d.getMonth(), d.getDay(), d.getHour(), d.getMinute(), d.getSecond(),
                                           (d.getMillisecond() * 1000000), 0, 0), "QoreOracleConnection::dateTimeConstruct()", xsink))
#endif
   }

   DLLLOCAL QoreStringNode *getServerVersion(ExceptionSink *xsink) {
      //printd(0, "QoreOracleConnection::getServerVersion() this=%p ds=%p envhp=%p svchp=%p errhp=%p\n", this, &ds, *env, svchp, errhp);
      // buffer for version information
      char version_buf[VERSION_BUF_SIZE + 1];

      // execute OCIServerVersion and check status code
      if (checkerr(OCIServerVersion(svchp, errhp, (OraText *)version_buf, VERSION_BUF_SIZE, OCI_HTYPE_SVCCTX), "QoreOracleConnection::getServerVersion()", xsink))
         return 0;

      return new QoreStringNode(version_buf);
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
