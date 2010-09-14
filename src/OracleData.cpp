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

#include "oracle.h"

int OracleData::checkerr(sword status, const char *query_name, ExceptionSink *xsink) {
   sb4 errcode = 0;

   //printd(5, "OracleData::checkerr(%p, %d, %s, isEvent=%d)\n", errhp, status, query_name ? query_name : "none", *xsink);
   switch (status) {
      case OCI_SUCCESS:
	 return 0;
      case OCI_SUCCESS_WITH_INFO: {
	 text errbuf[512];

	 OCIErrorGet((dvoid *)errhp, (ub4) 1, (text *) NULL, &errcode, errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
// 	 printd(0, "WARNING: %s returned OCI_SUCCESS_WITH_INFO: %s\n", query_name ? query_name : "<unknown>", remove_trailing_newlines((char *)errbuf));
	 // ignore SUCCESS_WITH_INFO codes
	 return 0;
      }

      case OCI_ERROR: {
	 text errbuf[512];
	 OCIErrorGet((dvoid *)errhp, (ub4) 1, (text *) NULL, &errcode, errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
	 if (query_name)
	    xsink->raiseException("DBI:ORACLE:OCI-ERROR", "%s@%s: %s: %s", ds.getUsername(), ds.getDBName(), query_name, remove_trailing_newlines((char *)errbuf));
	 else
	    xsink->raiseException("DBI:ORACLE:OCI-ERROR", "%s@%s: %s", ds.getUsername(), ds.getDBName(), remove_trailing_newlines((char *)errbuf));
	 break;
      }
      case OCI_INVALID_HANDLE:
	 if (query_name)
	    xsink->raiseException("DBI:ORACLE:OCI-INVALID-HANDLE", "%s@%s: %s: an invalid OCI handle was used", ds.getUsername(), ds.getDBName(), query_name);
	 else
	    xsink->raiseException("DBI:ORACLE:OCI-INVALID-HANDLE", "%s@%s: an invalid OCI handle was used", ds.getUsername(), ds.getDBName());
	 break;
      case OCI_NEED_DATA:
	 xsink->raiseException("DBI:ORACLE:OCI-NEED-DATA", "Oracle OCI error");
	 break;
      case OCI_NO_DATA:
	 xsink->raiseException("DBI:ORACLE:OCI-NODATA", "Oracle OCI error");
	 break;
      case OCI_STILL_EXECUTING:
	 xsink->raiseException("DBI:ORACLE:OCI-STILL-EXECUTING", "Oracle OCI error");
	 break;
      case OCI_CONTINUE:
	 xsink->raiseException("DBI:ORACLE:OCI-CONTINUE", "Oracle OCI error");
	 break;
      default:
	 xsink->raiseException("DBI:ORACLE:UNKNOWN-ERROR", "unknown OCI error code %d", status);
	 break;
   }
   return -1;
}

int OracleData::descriptorAlloc(void **descpp, unsigned type, const char *who, ExceptionSink *xsink) {
   return checkerr(OCIDescriptorAlloc(envhp, descpp, type, 0, 0), who, xsink);
}

int OracleData::handleAlloc(void **hndlpp, unsigned type, const char *who, ExceptionSink *xsink) {
   return checkerr(OCIHandleAlloc(envhp, hndlpp, type, 0, 0), who, xsink);
}

int OracleData::commit(ExceptionSink *xsink) {
   return checkerr(OCITransCommit(svchp, errhp, (ub4) 0), "OracleData:commit()", xsink);
}

int OracleData::rollback(ExceptionSink *xsink) {
   return checkerr(OCITransRollback(svchp, errhp, (ub4) 0), "OracleData:rollback()", xsink);
}

DateTimeNode *OracleData::getTimestamp(bool get_tz, OCIDateTime *odt, ExceptionSink *xsink) {
   //printd(5, "OraColumn::getValue() using TIMESTAMP handle %p\n", val.odt);
   sb2 year;
   ub1 month, day;
   if (checkerr(OCIDateTimeGetDate(envhp, errhp, odt, &year, &month, &day), "OCIDateTimeGetDate()", xsink))
      return 0;

   ub1 hour, minute, second;
   ub4 ns; // nanoseconds
   if (checkerr(OCIDateTimeGetTime(envhp, errhp, odt, &hour, &minute, &second, &ns), "OCIDateTimeGetTime()", xsink))
      return 0;

#ifdef _QORE_HAS_TIME_ZONES
   const AbstractQoreZoneInfo *zone;
   if (!get_tz)
      zone = currentTZ();
   else {
      // try to get time zone from date value
      // time zone offset, hour and minute
      sb1 oh = 0, om = 0;
      sword err = OCIDateTimeGetTimeZoneOffset(envhp, errhp, odt, &oh, &om);
      if (err == OCI_SUCCESS) {
	 //printd(5, "err=%d, oh=%d, om=%d, se=%d\n", err, (int)oh, (int)om, oh * 3600 + om * 60);
	 zone = findCreateOffsetZone(oh * 3600 + om * 60);
      }
      else {
	 printd(0, "OracleData::getTimestamp() this=%p time zone retrieval failed (%04d-%02d-%02d %02d:%02d:%02d)\n", this, year, month, day, hour, minute, second);
 	 // no time zone info, assume local time
	 zone = currentTZ();
      }
   }
   return DateTimeNode::makeAbsolute(zone, year, month, day, hour, minute, second, ns / 1000);
#else
   return new DateTimeNode(year, month, day, hour, minute, second, ns / 1000000);
#endif
}
