#include "oracleobject.h"
#include "oracle.h"
#include "ocilib_types.h"


DLLEXPORT AbstractQoreNode * f_oracle_object(const QoreListNode *params, ExceptionSink *xsink) {
    const QoreStringNode * tname = HARD_QORE_STRING(params, 0);
    const QoreHashNode * values = HARD_QORE_HASH(params, 1);
    // Now create a new hash with ( "ORA_TYPE_NAME" : tname, "VALUES" : values )
    // for binding
    QoreHashNode * h = new QoreHashNode();
    h->setKeyValue("type", new QoreStringNode(ORACLE_OBJECT), xsink);
    h->setKeyValue("^oratype^", tname->refSelf(), xsink);
    h->setKeyValue("^values^", values->refSelf(), xsink);
    return h;
}

DLLEXPORT AbstractQoreNode * f_oracle_object_placeholder(const QoreListNode *params, ExceptionSink *xsink) {
    const QoreStringNode * tname = HARD_QORE_STRING(params, 0);
    QoreHashNode * h = new QoreHashNode();
    h->setKeyValue("type", new QoreStringNode(ORACLE_OBJECT), xsink);
    h->setKeyValue("value", tname->refSelf(), xsink);
    return h;
}

DLLEXPORT AbstractQoreNode * f_oracle_collection(const QoreListNode *params, ExceptionSink *xsink) {
    const QoreStringNode * tname = HARD_QORE_STRING(params, 0);
    const QoreListNode * values = HARD_QORE_LIST(params, 1);
    // Now create a new hash with ( "ORA_TYPE_NAME" : tname, "VALUES" : values )
    // for binding
    QoreHashNode * h = new QoreHashNode();
    h->setKeyValue("type", new QoreStringNode(ORACLE_COLLECTION), xsink);
    h->setKeyValue("^oratype^", tname->refSelf(), xsink);
    h->setKeyValue("^values^", values->refSelf(), xsink);
    return h;
}

DLLEXPORT AbstractQoreNode * f_oracle_collection_placeholder(const QoreListNode *params, ExceptionSink *xsink) {
    const QoreStringNode * tname = HARD_QORE_STRING(params, 0);
    QoreHashNode * h = new QoreHashNode();
    h->setKeyValue("type", new QoreStringNode(ORACLE_COLLECTION), xsink);
    h->setKeyValue("value", tname->refSelf(), xsink);
    return h;
}

void ocilib_err_handler(OCI_Error *err)
{
    // TODO/FIXME: xsink handling here.
    printf(     "internal OCILIB error:\n"
                "  code  : ORA-%05i\n"
                "  msg   : %s\n"
                "  sql   : %s\n",
                OCI_ErrorGetOCICode(err), 
                OCI_ErrorGetString(err),
                OCI_GetSql(OCI_ErrorGetStatement(err))
           );
};


// return NTY object type - ORACLE_COLLECTION or ORACLE_OBJECT
// should be called id it's sure it's a NTY (after ntyCheckType()
// and/or in SQLT_NTY cases
const char * ntyHashType(const QoreHashNode * n) {
    const QoreStringNode * s = reinterpret_cast<const QoreStringNode*>(n->getKeyValue("type"));
    return s->getBuffer();
}

// check if is the hash really oracle NTY object
bool ntyCheckType(const char * tname, const QoreHashNode * n) {
    const QoreStringNode *s;
    s = dynamic_cast<const QoreStringNode*>(n->getKeyValue("type"));
    if (!s)
        return false;
    s = dynamic_cast<const QoreStringNode*>(n->getKeyValue("^oratype^"));
    if (!s)
        return false;
    const char * givenName = ntyHashType(n);
    return strcmp(givenName, tname) != 0;
}

OCI_Object* objPlaceholderQore(OracleData * d_ora, const char * tname, ExceptionSink *xsink)
{
    OCI_TypeInfo * info = OCI_TypeInfoGet(d_ora->ocilib_cn, tname, OCI_TIF_TYPE);
    OCI_Object * obj = OCI_ObjectCreate(d_ora->ocilib_cn, info);
    return obj;
}

OCI_Object* objBindQore(OracleData * d, const QoreHashNode * h, ExceptionSink * xsink)
{
//     QoreNodeAsStringHelper str(h, 1, xsink);
//     printf("obj hash= %s\n", str->getBuffer());

    if (!ntyCheckType(ORACLE_OBJECT, h)) {
        xsink->raiseException("BIND-ORACLE-OBJECT-ERROR",
                              "Hash is not passed with bindOracleObject()");
        return 0;
    }

    const QoreHashNode * th = reinterpret_cast<const QoreHashNode*>(h->getKeyValue("^values^"));
    const char * tname = reinterpret_cast<const QoreStringNode*>(h->getKeyValue("^oratype^"))->getBuffer();
    
    OCI_TypeInfo * info = OCI_TypeInfoGet(d->ocilib_cn, tname, OCI_TIF_TYPE);
    OCI_Object * obj = OCI_ObjectCreate(d->ocilib_cn, info);
    
    int n = OCI_TypeInfoGetColumnCount(info);
    for (int i = 1; i <= n; ++i) {
        
        OCI_Column *col = OCI_TypeInfoGetColumn(info, i);
        
        const char * cname = OCI_GetColumnName(col);
//         printf("Binding attribute: %s (%d/%d)\n", cname, i, n);
        if (! th->existsKey(cname)) {
            xsink->raiseException("BIND-ORACLE-OBJECT-ERROR", "Key %s (case sensitive) does not exists in the object hash", cname);
            return obj;
        }
        /*const*/ QoreString * key = new QoreString(cname);
        bool e;
        const AbstractQoreNode * val = th->getKeyValueExistence(key, e, xsink);
        
//         qore_type_t ntype = val->getType();
        
        // TODO/FIXME: faster?
        if (dynamic_cast<const QoreNullNode*>(val) || dynamic_cast<const QoreNothingNode*>(val)) {
            OCI_ObjectSetNull(obj, cname);
            continue;
        }
        
        switch (col->ocode)
        {
            case SQLT_LNG: // long
            case SQLT_AFC:
            case SQLT_AVC:
            case SQLT_STR:
            case SQLT_CHR: {
                // strings
                QoreStringNodeValueHelper str(val);
                OCI_ObjectSetString(obj, cname, str->getBuffer());
                break;
            }

            case SQLT_NUM:
                if (col->scale == -127 && col->prec > 0)
                    // float
                    OCI_ObjectSetDouble(obj, cname, val->getAsFloat());
                else
                    // int
                    OCI_ObjectSetInt(obj, cname, val->getAsInt());
                break;

            case SQLT_INT:
                OCI_ObjectSetInt(obj, cname, val->getAsInt());
                break;

            case SQLT_FLT:
#if OCI_VERSION_COMPILE >= OCI_10_1
            case SQLT_BFLOAT:
            case SQLT_IBFLOAT:
            case SQLT_BDOUBLE:
            case SQLT_IBDOUBLE:
#endif
                // float
                OCI_ObjectSetDouble(obj, cname, val->getAsFloat());
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
                    OCI_Timestamp *dt = OCI_TimestampCreate(d->ocilib_cn, OCI_TIMESTAMP);
                    OCI_TimestampConstruct(dt,
                                           dn->getYear(), dn->getMonth(), dn->getDay(),
                                           dn->getHour(), dn->getMinute(), dn->getSecond(),
                                           (dn->getMillisecond() * 1000000),
                                           0 // no TZ here
                                           );
                    OCI_ObjectSetTimestamp(obj, cname, dt);
                    OCI_TimestampFree(dt);
                }
                else if (col->type == OCI_CDT_DATETIME) {
                    OCI_Date * dt = OCI_DateCreate(d->ocilib_cn);
                    OCI_DateSetDateTime(dt,
                                        dn->getYear(), dn->getMonth(), dn->getDay(),
                                        dn->getHour(), dn->getMinute(), dn->getSecond()
                                       );
                    OCI_ObjectSetDate(obj, cname, dt);
                    OCI_DateFree(dt);
                }
                // intervals
                else if (col->type == OCI_CDT_INTERVAL) {
                    OCI_Interval * dt;
                    if (col->ocode == SQLT_INTERVAL_YM) {
                        dt = OCI_IntervalCreate(d->ocilib_cn, OCI_INTERVAL_YM);
                        OCI_IntervalSetYearMonth(dt, dn->getYear(), dn->getMonth());
                        OCI_ObjectSetInterval(obj, cname, dt);
                    }
                    else  {
                        // SQLT_INTERVAL_DS
                        dt = OCI_IntervalCreate(d->ocilib_cn, OCI_INTERVAL_DS);
                        OCI_IntervalSetDaySecond(dt,
                                                 dn->getDay(),
                                                 dn->getHour(), dn->getMinute(), dn->getSecond(),
                                                 (dn->getMillisecond() * 1000000)
                                                );
                        OCI_ObjectSetInterval(obj, cname, dt);
                    }
                    OCI_IntervalFree(dt);
                }
                else
                    assert(0); // TODO/FIXME: xsink?
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
//                 OCI_Lob * l = OCI_ObjectGetLob(obj, cname);
                OCI_Lob * l;
                qore_type_t ntype = val->getType();
                if (ntype == NT_BINARY) {
                    l = OCI_LobCreate(d->ocilib_cn, OCI_BLOB);
                    const BinaryNode * bn = reinterpret_cast<const BinaryNode*>(val);
                    unsigned int size = bn->size();
                    OCI_LobWrite2(l, (void*)bn->getPtr(), &size, &size);
                }
                else {
                    // clobs
                    l = OCI_LobCreate(d->ocilib_cn, OCI_CLOB);
                    QoreStringNodeValueHelper str(val);
                    unsigned int size = str->length();
                    unsigned int sizelen = str->strlen();
                    OCI_LobWrite2(l, (void*)str->getBuffer(), &size, &sizelen);
                }
                OCI_ObjectSetLob(obj, cname, l);
                OCI_LobFree(l);
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
                if (!n) {
                    xsink->raiseException("BIND-OBJECT-ERROR", "Object type: '%s' has to be passed as an Oracle Object", col->typinf->name);
                    return obj;
                }
                const char * t = ntyHashType(n);
                if (!strcmp(t, ORACLE_OBJECT)) {
                    OCI_Object * o = objBindQore(d, n, xsink);
                    OCI_ObjectSetObject(obj, cname, o);
                    OCI_ObjectFree(o);
                }
                else if (!strcmp(t, ORACLE_COLLECTION)) {
                    OCI_Coll * o = collBindQore(d, n, xsink);
                    OCI_ObjectSetColl(obj, cname, o);
                    OCI_CollFree(o);
                }
                else
                    assert(0); // TODO/FIXME
                break;
            }

            default:
                xsink->raiseException("BIND-OBJECT-ERROR", "unknown datatype to bind as an attribute: %s", col->typinf->name);
                return obj;
        } // switch

        delete key;
    }
    
    return obj;
}

AbstractQoreNode* objToQore(OCI_Object * obj, Datasource *ds, ExceptionSink *xsink)
{
    QoreHashNode *rv = new QoreHashNode();

    int n = OCI_TypeInfoGetColumnCount(obj->typinf);
    for (int i = 1; i <= n; ++i) {
        
        OCI_Column *col = OCI_TypeInfoGetColumn(obj->typinf, i);
        
        const char * cname = OCI_GetColumnName(col);
        
        if (OCI_ObjectIsNull(obj, cname)) {
            rv->setKeyValue(cname, null(), xsink);
            continue;
        }

        switch (col->ocode)
        {
            case SQLT_LNG: // long
            case SQLT_AFC:
            case SQLT_AVC:
            case SQLT_STR:
            case SQLT_CHR: {
                // strings
                rv->setKeyValue(cname, new QoreStringNode(OCI_ObjectGetString(obj, cname)), xsink);
                break;
            }

            case SQLT_NUM:
                if (col->scale == -127 && col->prec > 0)
                    // float
                    rv->setKeyValue(cname, new QoreFloatNode(OCI_ObjectGetDouble(obj, cname)), xsink);
                else
                    // int
                    rv->setKeyValue(cname, new QoreBigIntNode(OCI_ObjectGetBigInt(obj, cname)), xsink);
                break;

            case SQLT_INT:
                rv->setKeyValue(cname, new QoreBigIntNode(OCI_ObjectGetBigInt(obj, cname)), xsink);
                break;

            case SQLT_FLT:
#if OCI_VERSION_COMPILE >= OCI_10_1
            case SQLT_BFLOAT:
            case SQLT_IBFLOAT:
            case SQLT_BDOUBLE:
            case SQLT_IBDOUBLE:
#endif
                // float
                rv->setKeyValue(cname, new QoreFloatNode(OCI_ObjectGetDouble(obj, cname)), xsink);
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
                    OCI_Timestamp * dt = OCI_ObjectGetTimeStamp(obj, cname);
                    // only SQLT_TIMESTAMP gets the default TZ
//                     assert(0);
                    rv->setKeyValue(cname, get_oracle_timestamp(col->ocode != SQLT_TIMESTAMP, ds, dt->handle, xsink), xsink);
                }
                // pure DATE like
                else if (col->type == OCI_CDT_DATETIME) {
                    OCI_Date * dt = OCI_ObjectGetDate(obj, cname);
                    int y, m, d, h, mi, s;
                    OCI_DateGetDateTime(dt, &y, &m, &d, &h, &mi, &s);
                    DateTimeNode * dn = new DateTimeNode(y, m, d, h, mi, s);
                    rv->setKeyValue(cname, dn, xsink);
                }
                // intervals
                else if (col->type == OCI_CDT_INTERVAL) {
                    OCI_Interval * dt = OCI_ObjectGetInterval(obj, cname);
                    if (col->ocode == SQLT_INTERVAL_YM) {
                        int y, m;
                        OCI_IntervalGetYearMonth(dt, &y, &m);
                        rv->setKeyValue(cname, new DateTimeNode(y, m, 0, 0, 0, 0, 0, true), xsink);
                    }
                    else  {
                        // SQLT_INTERVAL_DS
                        int d, h, mi, s, fs;
                        OCI_IntervalGetDaySecond(dt, &d, &h, &mi, &s, &fs);
#ifdef _QORE_HAS_TIME_ZONES
                        rv->setKeyValue(cname, DateTimeNode::makeRelative(0, 0, d, h, mi, s, fs / 1000), xsink);
#else
                        rv->setKeyValue(cname, new DateTimeNode(0, 0,  d, h, mi, s, fs / 1000000, true), xsink);
#endif
                    }
                }
                else
                    assert(0); // TODO/FIXME: xsink?
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
                OCI_Lob * l = OCI_ObjectGetLob(obj, cname);
                // The returned value is in bytes for BLOBS and characters for CLOBS/NCLOBs
                uint len = OCI_LobGetLength(l);
                void *buf = malloc(len);
                OCI_LobRead2(l, buf, &len, &len);

                if (OCI_LobGetType(l) == OCI_BLOB) {
                    SimpleRefHolder<BinaryNode> b(new BinaryNode());
                    b->append(buf, len);
                    rv->setKeyValue(cname, b.release(), xsink);
                }
                else {
                    // clobs
                    QoreStringNodeHolder str(new QoreStringNode(ds->getQoreEncoding()));
                    str->concat((const char*)buf, len);
                    rv->setKeyValue(cname, str.release(), xsink);

                }
                free(buf);
                OCI_LobFree(l);
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
                    rv->setKeyValue(cname, collToQore(OCI_ObjectGetColl(obj, cname), ds, xsink), xsink);
                } else {
                    // object
                    rv->setKeyValue(cname, objToQore(OCI_ObjectGetObject(obj, cname), ds, xsink), xsink);
                }
                break;

            default:
                xsink->raiseException("BIND-OBJECT-ERROR", "unknown datatype to fetch as an attribute: %s", col->typinf->name);
                return 0;
        } // switch

    }
    
    return rv;
}


OCI_Coll* collBindQore(OracleData * d, const QoreHashNode * h, ExceptionSink * xsink)
{
//     QoreNodeAsStringHelper str(h, 1, xsink);
//     printf("hash= %s\n", str->getBuffer());

    if (!ntyCheckType(ORACLE_COLLECTION, h)) {
        xsink->raiseException("BIND-ORACLE-COLLECTION-ERROR",
                              "List is not passed with bindOracleCollection()");
        return 0;
    }
    
    const QoreListNode * th = reinterpret_cast<const QoreListNode*>(h->getKeyValue("^values^"));
    const char * tname = reinterpret_cast<const QoreStringNode*>(h->getKeyValue("^oratype^"))->getBuffer();
    
    OCI_TypeInfo * info = OCI_TypeInfoGet(d->ocilib_cn, tname, OCI_TIF_TYPE);
//     printf("onfo: %d\n", info->nb_cols);
    OCI_Coll * obj = OCI_CollCreate(info);
    OCI_Column *col = OCI_TypeInfoGetColumn(info, 1);
        
//     const char * cname = OCI_GetColumnName(col);
//     printf("Binding attribute: %s\n", cname);
    OCI_Elem * e = OCI_ElemCreate(info);
    
    for (uint i = 0; i < th->size(); ++i) {
        const AbstractQoreNode * val = th->retrieve_entry(i);
        assert(e);
        if (dynamic_cast<const QoreNullNode*>(val) || dynamic_cast<const QoreNothingNode*>(val)) {
//             printf("val str: NULL\n");
            OCI_ElemSetNull(e);
            OCI_CollAppend(obj, e);
            continue;
        }

        switch (col->ocode)
        {
            case SQLT_LNG: // long
            case SQLT_AFC:
            case SQLT_AVC:
            case SQLT_STR:
            case SQLT_CHR: {
                // strings
                QoreStringNodeValueHelper str(val);
//                 printf("val str: %s\n", str->getBuffer());
                OCI_ElemSetString(e, str->getBuffer());
//                 printf("OCI_ElemSetString %s\n", OCI_ElemGetString(e));
                break;
            }

            case SQLT_NUM:
                if (col->scale == -127 && col->prec > 0)
                    // float
                    OCI_ElemSetDouble(e, val->getAsFloat());
                else
                    // int
                    OCI_ElemSetInt(e, val->getAsInt());
                break;

            case SQLT_INT:
                OCI_ElemSetInt(e, val->getAsInt());
                break;

            case SQLT_FLT:
#if OCI_VERSION_COMPILE >= OCI_10_1
            case SQLT_BFLOAT:
            case SQLT_IBFLOAT:
            case SQLT_BDOUBLE:
            case SQLT_IBDOUBLE:
#endif
                // float
                OCI_ElemSetDouble(e, val->getAsFloat());
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
                    OCI_Timestamp *dt = OCI_TimestampCreate(d->ocilib_cn, OCI_TIMESTAMP);
                    OCI_TimestampConstruct(dt,
                                           dn->getYear(), dn->getMonth(), dn->getDay(),
                                           dn->getHour(), dn->getMinute(), dn->getSecond(),
                                           (dn->getMillisecond() * 1000000),
                                           0 // no TZ here
                                           );
                    OCI_ElemSetTimestamp(e, dt);
                    OCI_TimestampFree(dt);
                }
                else if (col->type == OCI_CDT_DATETIME) {
                    OCI_Date * dt = OCI_DateCreate(d->ocilib_cn);
                    OCI_DateSetDateTime(dt,
                                        dn->getYear(), dn->getMonth(), dn->getDay(),
                                        dn->getHour(), dn->getMinute(), dn->getSecond()
                                       );
                    OCI_ElemSetDate(e, dt);
                    OCI_DateFree(dt);
                }
                // intervals
                else if (col->type == OCI_CDT_INTERVAL) {
                    OCI_Interval * dt;
                    if (col->ocode == SQLT_INTERVAL_YM) {
                        dt = OCI_IntervalCreate(d->ocilib_cn, OCI_INTERVAL_YM);
                        OCI_IntervalSetYearMonth(dt, dn->getYear(), dn->getMonth());
                        OCI_ElemSetInterval(e, dt);
                    }
                    else  {
                        // SQLT_INTERVAL_DS
                        dt = OCI_IntervalCreate(d->ocilib_cn, OCI_INTERVAL_DS);
                        OCI_IntervalSetDaySecond(dt,
                                                 dn->getDay(),
                                                 dn->getHour(), dn->getMinute(), dn->getSecond(),
                                                 (dn->getMillisecond() * 1000000)
                                                );
                        OCI_ElemSetInterval(e, dt);
                    }
                    OCI_IntervalFree(dt);
                }
                else
                    assert(0); // TODO/FIXME: xsink?
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
                OCI_Lob * l;
                qore_type_t ntype = val->getType();
                if (ntype == NT_BINARY) {
                    l = OCI_LobCreate(d->ocilib_cn, OCI_BLOB);
                    const BinaryNode * bn = reinterpret_cast<const BinaryNode*>(val);
                    unsigned int size = bn->size();
                    OCI_LobWrite2(l, (void*)bn->getPtr(), &size, &size);
                }
                else {
                    // clobs
                    l = OCI_LobCreate(d->ocilib_cn, OCI_CLOB);
                    QoreStringNodeValueHelper str(val);
                    unsigned int size = str->length();
                    unsigned int sizelen = str->strlen();
                    OCI_LobWrite2(l, (void*)str->getBuffer(), &size, &sizelen);
                }
                OCI_ElemSetLob(e, l);
                OCI_LobFree(l);
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
                if (!n) {
                    xsink->raiseException("BIND-COLLECTION-ERROR", "Object type: '%s' has to be passed as an Oracle Collection", col->typinf->name);
                    return obj;
                }
                const char * t = ntyHashType(n);
                if (!strcmp(t, ORACLE_OBJECT)) {
                    OCI_Object * o = objBindQore(d, n, xsink);
                    OCI_ElemSetObject(e, o);
                    OCI_ObjectFree(o);
                }
                else if (!strcmp(t, ORACLE_COLLECTION)) {
                    OCI_Coll * o = collBindQore(d, n, xsink);
                    OCI_ElemSetColl(e, o);
                    OCI_CollFree(o);
                }
                else
                    assert(0); // TODO/FIXME

                break;
            }

            default:
                xsink->raiseException("BIND-COLLECTION-ERROR", "unknown datatype to bind as an attribute: %s", col->typinf->name);
        } // switch
        
        if (*xsink)
            return obj;

        OCI_CollAppend(obj, e);
    }
    
    OCI_ElemFree(e);

    return obj;
}

AbstractQoreNode* collToQore(OCI_Coll * obj, Datasource *ds, ExceptionSink *xsink)
{
    QoreListNode *rv = new QoreListNode();
    
    OCI_Elem * e;
    
    int count = OCI_CollGetSize(obj);
    OCI_Column *col = OCI_TypeInfoGetColumn(obj->typinf, 1);

    for (int i = 1; i <= count; ++i) {

        e = OCI_CollGetAt(obj, i);

        if (OCI_ElemIsNull(e)) {
            rv->set_entry(rv->size(), null(), xsink);
            continue;
        }

        switch (col->ocode)
        {
            case SQLT_LNG: // long
            case SQLT_AFC:
            case SQLT_AVC:
            case SQLT_STR:
            case SQLT_CHR: {
                // strings
                rv->set_entry(rv->size(), new QoreStringNode(OCI_ElemGetString(e)), xsink);
                break;
            }

            case SQLT_NUM:
                if (col->scale == -127 && col->prec > 0)
                    // float
                    rv->set_entry(rv->size(), new QoreFloatNode(OCI_ElemGetDouble(e)), xsink);
                else
                    // int
                    rv->set_entry(rv->size(), new QoreBigIntNode(OCI_ElemGetBigInt(e)), xsink);
                break;

            case SQLT_INT:
                rv->set_entry(rv->size(), new QoreBigIntNode(OCI_ElemGetBigInt(e)), xsink);
                break;

            case SQLT_FLT:
#if OCI_VERSION_COMPILE >= OCI_10_1
            case SQLT_BFLOAT:
            case SQLT_IBFLOAT:
            case SQLT_BDOUBLE:
            case SQLT_IBDOUBLE:
#endif
                // float
                rv->set_entry(rv->size(), new QoreFloatNode(OCI_ElemGetDouble(e)), xsink);
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
                    OCI_Timestamp * dt = OCI_ElemGetTimeStamp(e);
                    // only SQLT_TIMESTAMP gets the default TZ
                    rv->set_entry(rv->size(), get_oracle_timestamp(col->ocode != SQLT_TIMESTAMP, ds, dt->handle, xsink), xsink);
                }
                // pure DATE like
                else if (col->type == OCI_CDT_DATETIME) {
                    OCI_Date * dt = OCI_ElemGetDate(e);
                    int y, m, d, h, mi, s;
                    OCI_DateGetDateTime(dt, &y, &m, &d, &h, &mi, &s);
                    DateTimeNode * dn = new DateTimeNode(y, m, d, h, mi, s);
                    rv->set_entry(rv->size(), dn, xsink);
                }
                // intervals
                else if (col->type == OCI_CDT_INTERVAL) {
                    OCI_Interval * dt = OCI_ElemGetInterval(e);
                    if (col->ocode == SQLT_INTERVAL_YM) {
                        int y, m;
                        OCI_IntervalGetYearMonth(dt, &y, &m);
                        rv->set_entry(rv->size(), new DateTimeNode(y, m, 0, 0, 0, 0, 0, true), xsink);
                    }
                    else  {
                        // SQLT_INTERVAL_DS
                        int d, h, mi, s, fs;
                        OCI_IntervalGetDaySecond(dt, &d, &h, &mi, &s, &fs);
#ifdef _QORE_HAS_TIME_ZONES
                        rv->set_entry(rv->size(), DateTimeNode::makeRelative(0, 0, d, h, mi, s, fs / 1000), xsink);
#else
                        rv->set_entry(rv->size(), new DateTimeNode(0, 0,  d, h, mi, s, fs / 1000000, true), xsink);
#endif
                    }
                }
                else
                    assert(0); // TODO/FIXME: xsink?
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
                OCI_Lob * l = OCI_ElemGetLob(e);
                // The returned value is in bytes for BLOBS and characters for CLOBS/NCLOBs
                uint len = OCI_LobGetLength(l);
                void *buf = malloc(len);
                OCI_LobRead2(l, buf, &len, &len);

                if (OCI_LobGetType(l) == OCI_BLOB) {
                    SimpleRefHolder<BinaryNode> b(new BinaryNode());
                    b->append(buf, len);
                    rv->set_entry(rv->size(), b.release(), xsink);
                }
                else {
                    // clobs
                    QoreStringNodeHolder str(new QoreStringNode(ds->getQoreEncoding()));
                    str->concat((const char*)buf, len);
                    rv->set_entry(rv->size(), str.release(), xsink);
                }
                free(buf);
                OCI_LobFree(l);
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
                    rv->set_entry(rv->size(), collToQore(OCI_ElemGetColl(e), ds, xsink), xsink);
                } else {
                    // object
                    rv->set_entry(rv->size(), objToQore(OCI_ElemGetObject(e), ds, xsink), xsink);
                }
                break;

            default:
                xsink->raiseException("BIND-COLLECTION-ERROR", "unknown datatype to fetch as an attribute: %d (define SQLT_...)", col->ocode);
                return 0;
        } // switch
        
    }

    return rv;
}

OCI_Coll* collPlaceholderQore(OracleData * d_ora, const char * tname, ExceptionSink *xsink)
{
    OCI_TypeInfo * info = OCI_TypeInfoGet(d_ora->ocilib_cn, tname, OCI_TIF_TYPE);
    OCI_Coll * obj = OCI_CollCreate(info);
    return obj;
}
