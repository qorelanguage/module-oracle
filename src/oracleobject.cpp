/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  oracleobject.cpp

  Oracle OCI Interface to Qore DBI layer

  Qore Programming Language

  Copyright 2003 - 2014 Qore Technologies, sro

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

#include "oracleobject.h"
#include "oracle.h"
#include "ocilib_types.h"

void ocilib_err_handler(OCI_Error *err, ExceptionSink* xsink) {
   if (xsink && !err->warning) {
      QoreStringNode* desc = new QoreStringNodeMaker("ORA-%05d: %s", OCI_ErrorGetOCICode(err), OCI_ErrorGetString(err));
      desc->trim_trailing();
      xsink->raiseException("ORACLE-OCI-ERROR", desc);
   }
   else {
      // TODO/FIXME: xsink handling here.
      printf("internal OCILIB error:\n"
             "  code  : ORA-%05i\n"
             "  msg   : %s\n"
             "  sql   : %s\n",
             OCI_ErrorGetOCICode(err), 
             OCI_ErrorGetString(err),
             //OCI_GetSql(OCI_ErrorGetStatement(err))
             "<not available>"
         );
      assert(false);
   }
};

/* ------------------------------------------------------------------------ *
 * OCI_CollGetStruct
 * ------------------------------------------------------------------------ */
#include "ocilib_internal.h"
boolean OCI_API OCI_CollGetStruct(OCI_Library* pOCILib, OCI_Coll* obj, void** pp_struct, void** pp_ind, ExceptionSink* xsink) {
   OCI_CHECK_PTRQ(pOCILib, OCI_IPC_OBJECT, obj, FALSE, xsink);

   OCI_RESULT(pOCILib, TRUE);

   *pp_struct = (void *) obj->handle;

   if (pp_ind)
      *pp_ind = (void *) obj->tab_ind;

   OCI_RESULT(pOCILib, TRUE);

   return TRUE;
}

// return NTY object type - ORACLE_COLLECTION or ORACLE_OBJECT
// should be called id it's sure it's a NTY (after ntyCheckType()
// and/or in SQLT_NTY cases
const char* ntyHashType(const QoreHashNode* n) {
    if (!n)
        return 0;

    const AbstractQoreNode* node = n->getKeyValue("type");
    if (get_node_type(node) != NT_STRING)
       return 0;

    return reinterpret_cast<const QoreStringNode*>(node)->getBuffer();
}

// check if is the hash really oracle NTY object
bool ntyCheckType(const char * tname, const QoreHashNode * n, qore_type_t t) {
    const QoreStringNode* s;
    s = dynamic_cast<const QoreStringNode*>(n->getKeyValue("type"));
    if (!s) {
//         printf("ntyCheckType() type %p\n", s);
        return false;
    }
    s = dynamic_cast<const QoreStringNode*>(n->getKeyValue("^oratype^"));
    if (!s) {
//         printf("ntyCheckType() ^oratype^ %p\n", s);
        return false;
    }

    const AbstractQoreNode* p = n->getKeyValue("^values^");
    if (get_node_type(p) != t)
       return false;

//     printf("ntyCheckType() ^oratype^ %s\n", s->getBuffer());
    const char* givenName = ntyHashType(n);
//     printf("ntyCheckType(%s, %s) strcmp(givenName, tname) != 0 => %d\n", givenName, tname, strcmp(givenName, tname) != 0);
    return givenName && (strcmp(givenName, tname) == 0);
}

OCI_Object* objPlaceholderQore(QoreOracleConnection * conn, const char * tname, ExceptionSink *xsink) {
   OCI_TypeInfo * info = OCI_TypeInfoGet2(&conn->ocilib, conn->ocilib_cn, tname, OCI_TIF_TYPE, xsink);
   if (!info) {
      if (!*xsink)
         xsink->raiseException("ORACLE-OBJECT-ERROR", "could not get type info for object type '%s'", tname);
      return 0;
   }
   OCI_Object * obj = OCI_ObjectCreate2(&conn->ocilib, conn->ocilib_cn, info, xsink);
   if (!obj && !*xsink)
      xsink->raiseException("ORACLE-OBJECT-ERROR", "could not create placeholder buffer for object type '%s'", tname);
   return obj;
}

OCI_Object* objBindQore(QoreOracleConnection * d, const QoreHashNode * h, ExceptionSink * xsink) {
//     QoreNodeAsStringHelper str(h, 1, xsink);
//     printf("obj hash= %s\n", str->getBuffer());

   if (!ntyCheckType(ORACLE_OBJECT, h, NT_HASH)) {
        xsink->raiseException("BIND-ORACLE-OBJECT-ERROR",
                              "Bind value was not created with bindOracleObject()");
        return 0;
    }

    const QoreHashNode* th = reinterpret_cast<const QoreHashNode*>(h->getKeyValue("^values^"));
    const char* tname = reinterpret_cast<const QoreStringNode*>(h->getKeyValue("^oratype^"))->getBuffer();
    
    OCI_TypeInfo * info = OCI_TypeInfoGet2(&d->ocilib, d->ocilib_cn, tname, OCI_TIF_TYPE, xsink);
    if (!info) {
       if (!*xsink)
          xsink->raiseException("BIND-ORACLE-OBJECT-ERROR", "No type '%s' defined in the DB", tname);
        return 0;
    }
    OCI_Object* obj = OCI_ObjectCreate2(&d->ocilib, d->ocilib_cn, info, xsink);
    if (!obj)
       return 0;

    int n = OCI_TypeInfoGetColumnCount2(&d->ocilib, info);
    for (int i = 1; i <= n; ++i) {
       OCI_Column* col = OCI_TypeInfoGetColumn2(&d->ocilib, info, i, xsink);
       if (*xsink)
          return 0;
       if (!col) {
          xsink->raiseException("BIND-NTY-ERROR", "failed to retrieve column information for column %d", 1);
          return 0;
       }
        
       const char* cname = OCI_ColumnGetName2(&d->ocilib, col, xsink);
       if (*xsink)
          return 0;
       if (!cname) {
          xsink->raiseException("BIND-NTY-ERROR", "failed to retrieve column information for column %d", 1);
          return 0;
       }

//         printf("Binding attribute: %s (%d/%d)\n", cname, i, n);
        // This test has been removed to allow binding of filtered hashes (only required attributes)
        //if (! th->existsKey(cname)) {
            //xsink->raiseException("BIND-ORACLE-OBJECT-ERROR", "Key %s (case sensitive) does not exist in the object hash", cname);
	    //OCI_ObjectFree2(&d->ocilib, obj);
            //return 0;
        //}
        bool e;
        const AbstractQoreNode* val = th->getKeyValueExistence(cname, e);

//         qore_type_t ntype = val->getType();
        
        if (!e || is_null(val) || is_nothing(val)) {
	   OCI_ObjectSetNull2(&d->ocilib, obj, cname);
	   continue;
        }
        
        switch (col->ocode) {
            case SQLT_LNG: // long
            case SQLT_AFC:
            case SQLT_AVC:
            case SQLT_STR:
            case SQLT_CHR: {
                // strings
               QoreStringValueHelper str(val, d->ds.getQoreEncoding(), xsink);
               if (*xsink)
                  return 0;
               if (str->empty())
                  OCI_ObjectSetNull2(&d->ocilib, obj, cname);
               else
                  OCI_ObjectSetString2(&d->ocilib, obj, cname, str->empty() ? 0 : str->getBuffer());
               break;
            }

            case SQLT_NUM: {
                switch (val->getType()) {
                    case NT_STRING:
                    case NT_FLOAT: {
                        OCI_ObjectSetDouble2(&d->ocilib, obj, cname, val->getAsFloat());
                        break;
                    }
                    case NT_INT: {
                        OCI_ObjectSetBigInt2(&d->ocilib, obj, cname, val->getAsBigInt());
                        break;
                    }
#if 0
                    case NT_NUMBER: {
                        QoreStringNodeValueHelper str(val);
                        if (str->is_equal_soft(val, xsink)) {
                            if (!*xsink) {
                                xsink->raiseException("BIND-NTY-ERROR", "NUMBER: unable to bind value of type: '%s' to object attribute %s.%s. Out of boundaries.", val->getTypeName(), tname, cname);
                                return 0;
                            }
                        }
                        if (!OCI_ObjectSetDouble2(&d->ocilib, *obj, cname, val->getAsFloat(), xsink)) {
                            if (!*xsink)
                                xsink->raiseException("BIND-NTY-ERROR", "NUMBER: unable to assign a value to number object attribute %s.%s", tname, cname);
                            return 0;
                        }
                        break;
                    }
#endif
                    default:
                        xsink->raiseException("BIND-NTY-ERROR", "NUMBER: unable to bind value of Qore type: '%s' to object attribute %s.%s", val->getTypeName(), tname, cname);
                        return 0;
                    break;
                }
                break;
            }

            case SQLT_INT:
	       //printd(5, "objBindQore() binding int64: %lld\n", val->getAsBigInt());
	       OCI_ObjectSetBigInt2(&d->ocilib, obj, cname, val->getAsBigInt());
	       break;

            case SQLT_FLT:
#if OCI_VERSION_COMPILE >= OCI_10_1
            case SQLT_BFLOAT:
            case SQLT_IBFLOAT:
            case SQLT_BDOUBLE:
            case SQLT_IBDOUBLE:
#endif
                // float
                OCI_ObjectSetDouble2(&d->ocilib, obj, cname, val->getAsFloat());
                break;

            case SQLT_DAT:
            case SQLT_ODT:
            case SQLT_DATE: 
#if OCI_VERSION_COMPILE >= OCI_9_0
            case SQLT_TIMESTAMP:
            case SQLT_TIMESTAMP_TZ:
            case SQLT_TIMESTAMP_LTZ:
            case SQLT_INTERVAL_YM:
            case SQLT_INTERVAL_DS:
#endif
            {
                // date
                DateTimeValueHelper dn(val);
                
                if (col->type == OCI_CDT_TIMESTAMP) {
                    OCI_Timestamp * dt = OCI_TimestampCreate2(&d->ocilib, d->ocilib_cn, col->subtype);
                    OCI_TimestampConstruct2(&d->ocilib, dt,
                                           dn->getYear(), dn->getMonth(), dn->getDay(),
                                           dn->getHour(), dn->getMinute(), dn->getSecond(),
                                           (dn->getMillisecond() * 1000000),
                                           0 // no TZ here
                                           );
                    OCI_ObjectSetTimestamp2(&d->ocilib,obj, cname, dt);
                    OCI_TimestampFree2(&d->ocilib, dt);
                }
                else if (col->type == OCI_CDT_DATETIME) {
                    OCI_Date * dt = OCI_DateCreate(&d->ocilib, d->ocilib_cn);
                    OCI_DateSetDateTime(&d->ocilib, dt,
                                        dn->getYear(), dn->getMonth(), dn->getDay(),
                                        dn->getHour(), dn->getMinute(), dn->getSecond()
                                       );
                    OCI_ObjectSetDate2(&d->ocilib, obj, cname, dt);
                    OCI_DateFree(&d->ocilib, dt);
                }
                // intervals
                else if (col->type == OCI_CDT_INTERVAL) {
                    OCI_Interval * dt;
                    if (col->ocode == SQLT_INTERVAL_YM) {
                        dt = OCI_IntervalCreate2(&d->ocilib, d->ocilib_cn, OCI_INTERVAL_YM);
                        OCI_IntervalSetYearMonth2(&d->ocilib, dt, dn->getYear(), dn->getMonth());
                        OCI_ObjectSetInterval2(&d->ocilib, obj, cname, dt);
                    }
                    else  {
                        // SQLT_INTERVAL_DS
                        dt = OCI_IntervalCreate2(&d->ocilib, d->ocilib_cn, OCI_INTERVAL_DS);
                        OCI_IntervalSetDaySecond2(&d->ocilib, dt,
                                                 dn->getDay(),
                                                 dn->getHour(), dn->getMinute(), dn->getSecond(),
                                                 (dn->getMillisecond() * 1000000)
                                                );
                        OCI_ObjectSetInterval2(&d->ocilib, obj, cname, dt);
                    }
                    OCI_IntervalFree2(&d->ocilib, dt);
                }
                else {
                    xsink->raiseException("BIND-NTY-ERROR", "Unknown DATE-like argument for %s (type: %d)", cname, col->type);
		    OCI_ObjectFree2(&d->ocilib, obj);
		    return 0;
                }
                break;
            }


//             case SQLT_RDD:
//             case SQLT_RID:
                // rowid

//             case SQLT_BIN:
//             case SQLT_LBI:
                // raw/bin

//             case SQLT_RSET:
                // resultset

//             case SQLT_CUR:
                // cusror

            case SQLT_CLOB:
            case SQLT_BLOB: {
               OCI_Lob* l = 0;
                qore_type_t ntype = val->getType();
                if (ntype == NT_BINARY) {
                    const BinaryNode * bn = reinterpret_cast<const BinaryNode*>(val);
                    unsigned int size = bn->size();
                    if (size) {
                       l = OCI_LobCreate2(&d->ocilib, d->ocilib_cn, OCI_BLOB);
                       if (!l) {
                          xsink->raiseException("BIND-NTY-ERROR", "failed to bind LOB to NTY");
                          return 0;
                       }
                       OCI_LobWrite2(&d->ocilib, l, (void*)bn->getPtr(), &size, &size, xsink);
                    }
                }
                else {
                    QoreStringValueHelper str(val, d->ds.getQoreEncoding(), xsink);
                    if (*xsink)
                       return 0;
                    unsigned int sizelen = str->strlen();
                    if (sizelen) {
                       // clobs
                       l = OCI_LobCreate2(&d->ocilib, d->ocilib_cn, OCI_CLOB);
                       if (!l) {
                          xsink->raiseException("BIND-NTY-ERROR", "failed to bind LOB to NTY");
                          return 0;
                       }
                       unsigned int size = str->length();
                       OCI_LobWrite2(&d->ocilib, l, (void*)str->getBuffer(), &size, &sizelen, xsink);
                    }
                }
                if (!l)
                   OCI_ObjectSetNull2(&d->ocilib, obj, cname);
                else {
                   if (!*xsink)
                      OCI_ObjectSetLob2(&d->ocilib, obj, cname, l, xsink);
                   OCI_LobFree(&d->ocilib, l);
                }
                break;
            }

//             case SQLT_BFILE:
                // bfile

//             case SQLT_CFILE:
                // cfile

//             case SQLT_REF:
                // ref

#if OCI_VERSION_COMPILE >= OCI_9_0
            case SQLT_PNTY:
#endif
            case SQLT_NTY: {
                const QoreHashNode* n = dynamic_cast<const QoreHashNode*>(val);
                const char *t = ntyHashType(n);
                if (t && !strcmp(t, ORACLE_OBJECT)) {                    
                    OCI_Object *o = objBindQore(d, n, xsink);
                    if (o) {
                       OCI_ObjectSetObject2(&d->ocilib, obj, cname, o, xsink);
                       OCI_ObjectFree2(&d->ocilib, o);
                    }
                }
                else if (t && !strcmp(t, ORACLE_COLLECTION)) {
                    OCI_Coll *o = collBindQore(d, n, xsink);
                    if (o) {
                       OCI_ObjectSetColl2(&d->ocilib, obj, cname, o, xsink);
                       OCI_CollFree2(&d->ocilib, o);
                    }
                }
                else {
                    xsink->raiseException("BIND-NTY-ERROR", "Unknown NTY-like argument for %s (type: %d)", cname, col->type);
		    OCI_ObjectFree2(&d->ocilib, obj);
		    return 0;
                }
                break;
            }

            default:
                xsink->raiseException("BIND-OBJECT-ERROR", "unknown datatype to bind as an attribute (unsupported): %s", cname);
		OCI_ObjectFree2(&d->ocilib, obj);
		return 0;
        } // switch
    }

    return obj;
}

#define ORA_NUM_BUFSIZE 100
// here we use the special "text minimum" format
#define ORA_NUM_FORMAT "TM"
#define ORA_NUM_FORMAT_SIZE (sizeof(ORA_NUM_FORMAT)-1)

AbstractQoreNode* objToQore(QoreOracleConnection * conn, OCI_Object * obj, ExceptionSink *xsink) {
   ReferenceHolder<QoreHashNode> rv(new QoreHashNode, xsink);

   int n = OCI_TypeInfoGetColumnCount2(&conn->ocilib, obj->typinf);
   for (int i = 1; i <= n; ++i) {        
      OCI_Column *col = OCI_TypeInfoGetColumn2(&conn->ocilib, obj->typinf, i, xsink);
      if (*xsink)
         return 0;
      if (!col)
         return xsink->raiseException("FETCH-NTY-ERROR", "failed to retrieve column information for column %d", i);

      const char * cname = OCI_ColumnGetName2(&conn->ocilib, col, xsink);
      if (*xsink)
         return 0;
       if (!cname) {
          xsink->raiseException("FETCH-NTY-ERROR", "failed to retrieve column information for column %d", 1);
          return 0;
       }
        
      if (OCI_ObjectIsNull2(&conn->ocilib, obj, cname)) {
	 rv->setKeyValue(cname, null(), xsink);
	 continue;
      }

      switch (col->ocode) {
	 case SQLT_LNG: // long
	 case SQLT_AFC:
	 case SQLT_AVC:
	 case SQLT_STR:
	 case SQLT_CHR: {
	    // strings
            const char* str = OCI_ObjectGetString2(&conn->ocilib, obj, cname, xsink);
            if (*xsink)
               return 0;
	    rv->setKeyValue(cname, new QoreStringNode(str, conn->ds.getQoreEncoding()), xsink);
	    break;
	 }

	 case SQLT_NUM: {
            int index = OCI_ObjectGetAttrIndex2(&conn->ocilib, obj, cname, OCI_CDT_NUMERIC, xsink);
            if (*xsink)
               return 0;
            assert(index >= 0);
            OCIInd* ind = 0;
            OCINumber* num = (OCINumber*)OCI_ObjectGetAttr(obj, index, &ind);
            if (!num || *ind == OCI_IND_NULL) {
               rv->setKeyValue(cname, &Null, xsink);
               break;
            }

            char buf[ORA_NUM_BUFSIZE + 1];
            ub4 bs = ORA_NUM_BUFSIZE;
            if (conn->checkerr(OCINumberToText(conn->errhp, num, (const OraText*)ORA_NUM_FORMAT, ORA_NUM_FORMAT_SIZE, 0, 0, &bs, (OraText*)buf), 
                               "objToQore() converting NUMERIC value to text", xsink))
               return 0;
            buf[bs] = 0;

            // convert oracle numeric value to Qore value according to connection options
            AbstractQoreNode* nv = 0;
            int nopt = conn->getNumberOption();
            switch (nopt) {
               case OPT_NUM_OPTIMAL:
                  nv = conn->getNumberOptimal(buf);
                  break;
               case OPT_NUM_STRING:
                  nv = new QoreStringNode(buf, conn->ds.getQoreEncoding());
                  break;
               case OPT_NUM_NUMERIC:
                  nv = new QoreNumberNode(buf);
                  break;
               default:
                  assert(false);
            }
            rv->setKeyValue(cname, nv, xsink);
            break;
         }

	 case SQLT_INT: {
	    int64 i = OCI_ObjectGetBigInt2(&conn->ocilib, obj, cname);
	    //printd(5, "objToQore() i: %lld\n", i);
	    rv->setKeyValue(cname, new QoreBigIntNode(i), xsink);
	    break;
	 }

	 case SQLT_FLT:
#if OCI_VERSION_COMPILE >= OCI_10_1
	 case SQLT_BFLOAT:
	 case SQLT_IBFLOAT:
	 case SQLT_BDOUBLE:
	 case SQLT_IBDOUBLE:
#endif
	    // float
	    rv->setKeyValue(cname, new QoreFloatNode(OCI_ObjectGetDouble2(&conn->ocilib, obj, cname)), xsink);
	    break;


	 case SQLT_DAT:
	 case SQLT_ODT:
	 case SQLT_DATE: 
#if OCI_VERSION_COMPILE >= OCI_9_0
	 case SQLT_TIMESTAMP:
	 case SQLT_TIMESTAMP_TZ:
	 case SQLT_TIMESTAMP_LTZ:
	 case SQLT_INTERVAL_YM:
	 case SQLT_INTERVAL_DS:
#endif
	 {
	    // timestamps-like dates
	    if (col->type == OCI_CDT_TIMESTAMP) {
	       OCI_Timestamp *dt = OCI_ObjectGetTimestamp2(&conn->ocilib, obj, cname);
               if (!dt)
                  xsink->raiseException("FETCH-NTY-ERROR", "failed to retrieve timestamp attribute '%s'", cname);
               else {
                  // only SQLT_TIMESTAMP gets the default TZ
                  rv->setKeyValue(cname, conn->getTimestamp(col->ocode != SQLT_TIMESTAMP, dt->handle, xsink), xsink);
               }
	    }
	    // pure DATE like
	    else if (col->type == OCI_CDT_DATETIME) {
	       OCI_Date* dt = OCI_ObjectGetDate2(&conn->ocilib, obj, cname);
               if (!dt)
                  xsink->raiseException("FETCH-NTY-ERROR", "failed to retrieve date attribute '%s'", cname);
               else {
                  DateTimeNode* dn = conn->getDate(dt->handle);
                  rv->setKeyValue(cname, dn, xsink);
               }
	    }
	    // intervals
	    else if (col->type == OCI_CDT_INTERVAL) {
	       OCI_Interval * dt = OCI_ObjectGetInterval2(&conn->ocilib, obj, cname);
               if (!dt)
                  xsink->raiseException("FETCH-NTY-ERROR", "failed to retrieve date attribute '%s'", cname);
               else {
                  if (col->ocode == SQLT_INTERVAL_YM) {
                     int y, m;
                     OCI_IntervalGetYearMonth2(&conn->ocilib, dt, &y, &m);
                     rv->setKeyValue(cname, new DateTimeNode(y, m, 0, 0, 0, 0, 0, true), xsink);
                  }
                  else  {
                     // SQLT_INTERVAL_DS
                     int d, h, mi, s, fs;
                     OCI_IntervalGetDaySecond2(&conn->ocilib, dt, &d, &h, &mi, &s, &fs);
#ifdef _QORE_HAS_TIME_ZONES
                     rv->setKeyValue(cname, DateTimeNode::makeRelative(0, 0, d, h, mi, s, fs / 1000), xsink);
#else
                     rv->setKeyValue(cname, new DateTimeNode(0, 0,  d, h, mi, s, fs / 1000000, true), xsink);
#endif
                  }
               }
	    }
	    else {
	       xsink->raiseException("FETCH-NTY-ERROR", "Unknown DATE-like argument for %s (type: %d)", cname, col->type);
	       return 0;
	    }
	    break;
	 }


//             case SQLT_RDD:
//             case SQLT_RID:
	 // rowid

//             case SQLT_BIN:
//             case SQLT_LBI:
	 // raw/bin

//             case SQLT_RSET:
	 // resultset

//             case SQLT_CUR:
	 // cusror

	 case SQLT_CLOB:
	 case SQLT_BLOB: {
	    OCI_Lob * l = OCI_ObjectGetLob2(&conn->ocilib, obj, cname);
            if (!l) {
               xsink->raiseException("FETCH-NTY-ERROR", "failed to retrieve LOB attribute '%s'", cname);
               break;
            }

	    // The returned value is in bytes for BLOBS and characters for CLOBS/NCLOBs
	    unsigned int len = OCI_LobGetLength(&conn->ocilib, l);
	    void *buf = malloc(len);
	    OCI_LobRead2(&conn->ocilib, l, buf, &len, &len, xsink);
            
            if (!*xsink) {
               if (OCI_LobGetType(&conn->ocilib, l) == OCI_BLOB) {
                  SimpleRefHolder<BinaryNode> b(new BinaryNode());
                  b->append(buf, len);
                  rv->setKeyValue(cname, b.release(), xsink);
               }
               else {
                  // clobs
                  QoreStringNodeHolder str(new QoreStringNode(conn->ds.getQoreEncoding()));
                  str->concat((const char*)buf, len);
                  rv->setKeyValue(cname, str.release(), xsink);                  
               }
            }
	    free(buf);
	    OCI_LobFree(&conn->ocilib, l);
	    break;
	 }

//             case SQLT_BFILE:
	    // bfile

//             case SQLT_CFILE:
	    // cfile

//             case SQLT_REF:
	    // ref

#if OCI_VERSION_COMPILE >= OCI_9_0
	 case SQLT_PNTY:
#endif
	 case SQLT_NTY:
	    if (col->typinf->ccode) {
	       // collection
               OCI_Coll* c = OCI_ObjectGetColl2(&conn->ocilib, obj, cname, xsink);
               if (!c)
                  return 0;
	       rv->setKeyValue(cname, collToQore(conn, c, xsink), xsink);
	    } else {
	       // object
               OCI_Object* o = OCI_ObjectGetObject2(&conn->ocilib, obj, cname, xsink);
               if (!o)
                  return 0;
               rv->setKeyValue(cname, objToQore(conn, o, xsink), xsink);
	    }
	    break;

	 default:
	    xsink->raiseException("BIND-OBJECT-ERROR", "unknown datatype to fetch as an attribute (unsupported): %s", col->typinf->name);
	    return 0;
      } // switch
   }
    
   return rv.release();
}

OCI_Coll* collBindQore(QoreOracleConnection * d, const QoreHashNode * h, ExceptionSink * xsink) {
//     QoreNodeAsStringHelper str(h, 1, xsink);
//     printf("list= %s\n", str->getBuffer());

   if (!ntyCheckType(ORACLE_COLLECTION, h, NT_LIST)) {
      xsink->raiseException("BIND-ORACLE-COLLECTION-ERROR",
			    "Bind value was not created with bindOracleCollection()");
      return 0;
   }

   const QoreListNode * th = reinterpret_cast<const QoreListNode*>(h->getKeyValue("^values^"));
   const char * tname = reinterpret_cast<const QoreStringNode*>(h->getKeyValue("^oratype^"))->getBuffer();
    
   OCI_TypeInfo * info = OCI_TypeInfoGet2(&d->ocilib, d->ocilib_cn, tname, OCI_TIF_TYPE, xsink);
   if (!info) {
      if (!*xsink)
         xsink->raiseException("BIND-ORACLE-COLLECTION-ERROR",
                               "No type '%s' defined in the DB", tname);
      return 0;
   }

//     printf("onfo: %d\n", info->nb_cols);
   OCI_Coll * obj = OCI_CollCreate2(&d->ocilib, info, xsink);
   if (*xsink)
      return 0;
   OCI_Column *col = OCI_TypeInfoGetColumn2(&d->ocilib, info, 1, xsink);
   if (*xsink)
      return 0;
   if (!col) {
      xsink->raiseException("BIND-NTY-ERROR", "failed to retrieve column information for column %d", 1);
      return 0;
   }
        
//     const char * cname = OCI_GetColumnName(col);
//     printf("Binding attribute: %s\n", cname);
   OCI_Elem * e = OCI_ElemCreate2(&d->ocilib, info);
   if (!e) {
      xsink->raiseException("BIND-NTY-ERROR", "failed to create element for bind");      
      return 0;
   }
   ON_BLOCK_EXIT(OCI_ElemFree2, &d->ocilib, e);
    
   for (size_t i = 0; i < th->size(); ++i) {
      const AbstractQoreNode * val = th->retrieve_entry(i);

      if (is_null(val) || is_nothing(val)) {
	 OCI_ElemSetNull2(&d->ocilib, e);
	 OCI_CollAppend2(&d->ocilib, obj, e);
	 continue;
      }

      switch (col->ocode)      {
	 case SQLT_LNG: // long
	 case SQLT_AFC:
	 case SQLT_AVC:
	 case SQLT_STR:
	 case SQLT_CHR: {
	    // strings
	    QoreStringValueHelper str(val, d->ds.getQoreEncoding(), xsink);
            if (*xsink)
               return 0;
//                 printf("val str: %s\n", str->getBuffer());
            if (str->empty())
               OCI_ElemSetNull2(&d->ocilib, e);
            else
               OCI_ElemSetString2(&d->ocilib, e, str->getBuffer());
//                 printf("OCI_ElemSetString %s\n", OCI_ElemGetString(e));
	    break;
	 }

            case SQLT_NUM: {
                switch (val->getType()) {
                    case NT_STRING:
                    case NT_FLOAT: {
                        OCI_ElemSetDouble(&d->ocilib, e, val->getAsFloat());
                        break;
                    }
                    case NT_INT: {
                        OCI_ElemSetBigInt(&d->ocilib, e, val->getAsBigInt());
                        break;
                    }
                    default:
                        xsink->raiseException("BIND-NTY-ERROR", "NUMBER: unable to bind value of Qore type: '%s' element for collection '%s'", val->getTypeName(), tname);
                        return 0;
                    break;
                }
                break;
            }

	 case SQLT_INT:
	    OCI_ElemSetBigInt(&d->ocilib, e, val->getAsBigInt());
	    break;

	 case SQLT_FLT:
#if OCI_VERSION_COMPILE >= OCI_10_1
	 case SQLT_BFLOAT:
	 case SQLT_IBFLOAT:
	 case SQLT_BDOUBLE:
	 case SQLT_IBDOUBLE:
#endif
	    // float
	    OCI_ElemSetDouble(&d->ocilib, e, val->getAsFloat());
	    break;


	 case SQLT_DAT:
	 case SQLT_ODT:
	 case SQLT_DATE: 
#if OCI_VERSION_COMPILE >= OCI_9_0
	 case SQLT_TIMESTAMP:
	 case SQLT_TIMESTAMP_TZ:
	 case SQLT_TIMESTAMP_LTZ:
	 case SQLT_INTERVAL_YM:
	 case SQLT_INTERVAL_DS:
#endif
	 {
	    // date
	    const DateTimeNode * dn = reinterpret_cast<const DateTimeNode*>(val);
	    if (col->type == OCI_CDT_TIMESTAMP) {
	       OCI_Timestamp *dt = OCI_TimestampCreate2(&d->ocilib, d->ocilib_cn, col->subtype);
	       OCI_TimestampConstruct2(&d->ocilib, dt,
				       dn->getYear(), dn->getMonth(), dn->getDay(),
				       dn->getHour(), dn->getMinute(), dn->getSecond(),
				       (dn->getMillisecond() * 1000000),
				       0 // no TZ here
		  );
	       OCI_ElemSetTimestamp2(&d->ocilib, e, dt);
	       OCI_TimestampFree2(&d->ocilib, dt);
	    }
	    else if (col->type == OCI_CDT_DATETIME) {
	       OCI_Date * dt = OCI_DateCreate(&d->ocilib, d->ocilib_cn);
	       OCI_DateSetDateTime(&d->ocilib, dt,
				   dn->getYear(), dn->getMonth(), dn->getDay(),
				   dn->getHour(), dn->getMinute(), dn->getSecond()
		  );
	       OCI_ElemSetDate2(&d->ocilib, e, dt);
	       OCI_DateFree(&d->ocilib, dt);
	    }
	    // intervals
	    else if (col->type == OCI_CDT_INTERVAL) {
	       OCI_Interval * dt;
	       if (col->ocode == SQLT_INTERVAL_YM) {
		  dt = OCI_IntervalCreate2(&d->ocilib, d->ocilib_cn, OCI_INTERVAL_YM);
		  OCI_IntervalSetYearMonth2(&d->ocilib, dt, dn->getYear(), dn->getMonth());
		  OCI_ElemSetInterval2(&d->ocilib, e, dt);
	       }
	       else  {
		  // SQLT_INTERVAL_DS
		  dt = OCI_IntervalCreate2(&d->ocilib, d->ocilib_cn, OCI_INTERVAL_DS);
		  OCI_IntervalSetDaySecond2(&d->ocilib, dt,
					    dn->getDay(),
					    dn->getHour(), dn->getMinute(), dn->getSecond(),
					    (dn->getMillisecond() * 1000000)
		     );
		  OCI_ElemSetInterval2(&d->ocilib, e, dt);
	       }
	       OCI_IntervalFree2(&d->ocilib, dt);
	    }
	    else {
	       xsink->raiseException("BIND-NTY-ERROR", "Unknown DATE-like argument (type: %d)", col->type);
	       OCI_CollFree2(&d->ocilib, obj);
	       return 0;
	    }
	    break;
	 }
                

//             case SQLT_RDD:
//             case SQLT_RID:
	 // rowid

//             case SQLT_BIN:
//             case SQLT_LBI:
	 // raw/bin

//             case SQLT_RSET:
	 // resultset

//             case SQLT_CUR:
	 // cusror

	 case SQLT_CLOB:
	 case SQLT_BLOB: {
	    OCI_Lob * l = 0;
	    qore_type_t ntype = val->getType();
	    if (ntype == NT_BINARY) {
	       const BinaryNode * bn = reinterpret_cast<const BinaryNode*>(val);
	       unsigned int size = bn->size();
               if (size) {
                  l = OCI_LobCreate2(&d->ocilib, d->ocilib_cn, OCI_BLOB);
                  if (!l) {
                     xsink->raiseException("BIND-NTY-ERROR", "failed to bind LOB to NTY");
                     return 0;
                  }
                  OCI_LobWrite2(&d->ocilib, l, (void*)bn->getPtr(), &size, &size, xsink);
               }
	    }
	    else {
	       QoreStringValueHelper str(val, d->ds.getQoreEncoding(), xsink);
               if (*xsink)
                  return 0;
	       unsigned int sizelen = str->strlen();
               if (sizelen) {
                  // clobs
                  l = OCI_LobCreate2(&d->ocilib, d->ocilib_cn, OCI_CLOB);
                  if (!l) {
                     xsink->raiseException("BIND-NTY-ERROR", "failed to bind LOB to NTY");
                     return 0;
                  }
                  unsigned int size = str->length();
                  OCI_LobWrite2(&d->ocilib, l, (void*)str->getBuffer(), &size, &sizelen, xsink);
               }
	    }
            if (!l) {
               OCI_ElemSetNull2(&d->ocilib, e);
            }
            else {
               if (!*xsink)
                  OCI_ElemSetLob2(&d->ocilib, e, l);
               OCI_LobFree(&d->ocilib, l);
            }
	    break;
	 }

//             case SQLT_BFILE:
	    // bfile

//             case SQLT_CFILE:
	    // cfile

//             case SQLT_REF:
	    // ref

#if OCI_VERSION_COMPILE >= OCI_9_0
	 case SQLT_PNTY:
#endif
	 case SQLT_NTY: {
	    const QoreHashNode * n = dynamic_cast<const QoreHashNode *>(val);
	    const char * t = ntyHashType(n);
	    if (t && !strcmp(t, ORACLE_OBJECT)) {
	       OCI_Object * o = objBindQore(d, n, xsink);
               if (o) {
                  OCI_ElemSetObject2(&d->ocilib, e, o, xsink);
                  OCI_ObjectFree2(&d->ocilib, o);
               }
            }
	    else if (t && !strcmp(t, ORACLE_COLLECTION)) {
	       OCI_Coll * o = collBindQore(d, n, xsink);
               if (o) {
                  OCI_ElemSetColl2(&d->ocilib, e, o, xsink);
                  OCI_CollFree2(&d->ocilib, o);
               }
            }
	    else {
	       xsink->raiseException("BIND-NTY-ERROR", "Unknown NTY-like argument (type: %d)", col->type);
	       OCI_CollFree2(&d->ocilib, obj);
	       return 0;
	    }

	    break;
	 }

	 default:
	    xsink->raiseException("BIND-COLLECTION-ERROR", "unknown datatype to bind as an attribute (unsupported): %s", col->typinf->name);
	    OCI_CollFree2(&d->ocilib, obj);
	    return 0;
      } // switch
        
      OCI_CollAppend2(&d->ocilib, obj, e);
   }
    
   return obj;
}

AbstractQoreNode* collToQore(QoreOracleConnection * conn, OCI_Coll * obj, ExceptionSink *xsink) {
   ReferenceHolder<QoreListNode> rv(new QoreListNode, xsink);
    
   OCI_Elem * e;
    
   int count = OCI_CollGetSize2(&conn->ocilib, obj);
   OCI_Column *col = OCI_TypeInfoGetColumn2(&conn->ocilib, obj->typinf, 1, xsink);
   if (*xsink)
      return 0;
   if (!col)
      return xsink->raiseException("FETCH-NTY-ERROR", "failed to retrieve column information for column %d", 1);
        
   for (int i = 1; i <= count; ++i) {
      e = OCI_CollGetAt2(&conn->ocilib, obj, i, xsink);
      if (*xsink)
         return 0;
      if (!e)
         return xsink->raiseException("FETCH-NTY-ERROR", "failed to retrieve collection element %d", i);

      if (OCI_ElemIsNull2(&conn->ocilib, e)) {
	 rv->set_entry(rv->size(), null(), xsink);
	 continue;
      }

      switch (col->ocode) {
	 case SQLT_LNG: // long
	 case SQLT_AFC:
	 case SQLT_AVC:
	 case SQLT_STR:
	 case SQLT_CHR: {
	    // strings
            const char* str = OCI_ElemGetString2(&conn->ocilib, e);
	    rv->set_entry(rv->size(), new QoreStringNode(str, conn->ds.getQoreEncoding()), xsink);
	    break;
	 }

	 case SQLT_NUM:
	    if (col->scale == -127 && col->prec > 0) {
	       // float
	       rv->set_entry(rv->size(), new QoreFloatNode(OCI_ElemGetDouble2(&conn->ocilib, e)), xsink);
	       break;
	    }
	    // else fall through to SQLT_INT

	 case SQLT_INT: {
	    int64 i = OCI_ElemGetBigInt2(&conn->ocilib, e);
	    //printd(5, "collToQore() i: %lld\n", i);
	    rv->set_entry(rv->size(), new QoreBigIntNode(i), xsink);
	    break;
	 }

	 case SQLT_FLT:
#if OCI_VERSION_COMPILE >= OCI_10_1
	 case SQLT_BFLOAT:
	 case SQLT_IBFLOAT:
	 case SQLT_BDOUBLE:
	 case SQLT_IBDOUBLE:
#endif
	    // float
	    rv->set_entry(rv->size(), new QoreFloatNode(OCI_ElemGetDouble2(&conn->ocilib, e)), xsink);
	    break;

	 case SQLT_DAT:
	 case SQLT_ODT:
	 case SQLT_DATE: 
#if OCI_VERSION_COMPILE >= OCI_9_0
	 case SQLT_TIMESTAMP:
	 case SQLT_TIMESTAMP_TZ:
	 case SQLT_TIMESTAMP_LTZ:
	 case SQLT_INTERVAL_YM:
	 case SQLT_INTERVAL_DS:
#endif
	 {
	    // timestamps-like dates
	    if (col->type == OCI_CDT_TIMESTAMP) {
	       OCI_Timestamp* dt = OCI_ElemGetTimestamp2(&conn->ocilib, e); 
               if (!dt)
                  return xsink->raiseException("FETCH-NTY-ERROR", "failed to retrieve TIMESTAMP element");

	       // only SQLT_TIMESTAMP gets the default TZ
               rv->set_entry(rv->size(), conn->getTimestamp(col->ocode != SQLT_TIMESTAMP, dt->handle, xsink), xsink);
	    }
	    // pure DATE like
	    else if (col->type == OCI_CDT_DATETIME) {
	       OCI_Date* dt = OCI_ElemGetDate2(&conn->ocilib, e);
               if (!dt)
                  return xsink->raiseException("FETCH-NTY-ERROR", "failed to retrieve DATE element");
	       rv->set_entry(rv->size(), conn->getDate(dt->handle), xsink);
	    }
	    // intervals
	    else if (col->type == OCI_CDT_INTERVAL) {
	       OCI_Interval* dt = OCI_ElemGetInterval2(&conn->ocilib, e);
               if (!dt)
                  return xsink->raiseException("FETCH-NTY-ERROR", "failed to retrieve INTERVAL element");
               if (col->ocode == SQLT_INTERVAL_YM) {
                  int y, m;
                  OCI_IntervalGetYearMonth2(&conn->ocilib, dt, &y, &m);
                  rv->set_entry(rv->size(), new DateTimeNode(y, m, 0, 0, 0, 0, 0, true), xsink);
               }
               else  {
                  // SQLT_INTERVAL_DS
                  int d, h, mi, s, fs;
                  OCI_IntervalGetDaySecond2(&conn->ocilib, dt, &d, &h, &mi, &s, &fs);
#ifdef _QORE_HAS_TIME_ZONES
                  rv->set_entry(rv->size(), DateTimeNode::makeRelative(0, 0, d, h, mi, s, fs / 1000), xsink);
#else
                  rv->set_entry(rv->size(), new DateTimeNode(0, 0,  d, h, mi, s, fs / 1000000, true), xsink);
#endif
               }
	    }
	    else {
	       xsink->raiseException("FETCH-NTY-ERROR", "Unknown DATE-like argument (code: %d)", col->ocode);
	       return 0;
	    }
	    break;
	 }
                

//             case SQLT_RDD:
//             case SQLT_RID:
	 // rowid

//             case SQLT_BIN:
//             case SQLT_LBI:
	 // raw/bin

//             case SQLT_RSET:
	 // resultset

//             case SQLT_CUR:
	 // cusror

	 case SQLT_CLOB:
	 case SQLT_BLOB: {
	    OCI_Lob* l = OCI_ElemGetLob2(&conn->ocilib, e);
            if (!l)
               return xsink->raiseException("FETCH-NTY-ERROR", "failed to retrieve LOB element");

	    // The returned value is in bytes for BLOBS and characters for CLOBS/NCLOBs
	    unsigned int len = OCI_LobGetLength(&conn->ocilib, l);
	    void *buf = malloc(len);
	    OCI_LobRead2(&conn->ocilib, l, buf, &len, &len, xsink);

            if (!*xsink) {
               if (OCI_LobGetType(&conn->ocilib, l) == OCI_BLOB) {
                  SimpleRefHolder<BinaryNode> b(new BinaryNode());
                  b->append(buf, len);
                  rv->set_entry(rv->size(), b.release(), xsink);
               }
               else {
                  // clobs
                  QoreStringNodeHolder str(new QoreStringNode(conn->ds.getQoreEncoding()));
                  str->concat((const char*)buf, len);
                  rv->set_entry(rv->size(), str.release(), xsink);
               }
            }
	    free(buf);
	    OCI_LobFree(&conn->ocilib, l);
	    break;
	 }

//             case SQLT_BFILE:
	    // bfile

//             case SQLT_CFILE:
	    // cfile

//             case SQLT_REF:
	    // ref

#if OCI_VERSION_COMPILE >= OCI_9_0
	 case SQLT_PNTY:
#endif
	 case SQLT_NTY:
	    if (col->typinf->ccode) {
	       // collection
               OCI_Coll* coll = OCI_ElemGetColl2(&conn->ocilib, e, xsink);
               if (!coll)
                  return xsink->raiseException("FETCH-NTY-ERROR", "failed to retrieve COLLECTION element");
               AbstractQoreNode* v = collToQore(conn, coll, xsink);
               if (*xsink) {
                  assert(!v);
                  return 0;
               }
               rv->set_entry(rv->size(), v, xsink);
	    } else {
	       // object
               OCI_Object* obj = OCI_ElemGetObject2(&conn->ocilib, e, xsink);
               if (!obj) {
                  if (!*xsink)
                     return xsink->raiseException("FETCH-NTY-ERROR", "failed to retrieve NTY element");
                  return 0;
               }
               AbstractQoreNode* v = objToQore(conn, obj, xsink);
               if (*xsink) {
                  assert(!v);
                  return 0;
               }
               rv->set_entry(rv->size(), v, xsink);
	    }
	    break;

	 default:
	    xsink->raiseException("BIND-COLLECTION-ERROR", "unknown datatype to fetch as an attribute: %d (define SQLT_...)", col->ocode);
	    return 0;
      } // switch
        
   }

   return rv.release();
}

OCI_Coll* collPlaceholderQore(QoreOracleConnection *conn, const char * tname, ExceptionSink *xsink) {
   OCI_TypeInfo* info = OCI_TypeInfoGet2(&conn->ocilib, conn->ocilib_cn, tname, OCI_TIF_TYPE, xsink);
   if (!info) {
      if (!*xsink)
         xsink->raiseException("PLACEHOLDER-ORACLE-COLLECTION-ERROR",
                               "No type '%s' defined in the DB", tname);
      return 0;
   }
   OCI_Coll* obj = OCI_CollCreate2(&conn->ocilib, info, xsink);
   if (!obj && !*xsink)
      xsink->raiseException("ORACLE-COLLECTION-ERROR", "could not create placeholder buffer for object type '%s'", tname);
   return obj;
}
