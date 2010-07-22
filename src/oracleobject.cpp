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

OCI_Object* objPlaceholderQore(OracleData * d_ora, const char * tname, ExceptionSink *xsink)
{
    OCI_TypeInfo * info = OCI_TypeInfoGet(d_ora->ocilib_cn, tname, OCI_TIF_TYPE);
    OCI_Object * obj = OCI_ObjectCreate(d_ora->ocilib_cn, info);
    return obj;
}

OCI_Object* objBindQore(OracleData * d, const QoreHashNode * h, ExceptionSink * xsink)
{
     QoreNodeAsStringHelper str(h, FMT_NONE, xsink);
     printf("hash= %s\n", str->getBuffer());
QoreNodeAsStringHelper str1(h->getKeyValue("^oratype^"), FMT_NONE, xsink);
printf("sart\n");
printf("oratype= %s \n", str1->getBuffer());
printf("end\n");
    // TODO/FIXME: chech if it's really object-like hash
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
            xsink->raiseException("BIND-ORACLE-OBJECT-ERROR", "Key %s does not exists in the object hash", cname);
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
                // float
                OCI_ObjectSetDouble(obj, cname, val->getAsFloat());
                break;
#endif

            case SQLT_DAT:
            case SQLT_ODT:
            case SQLT_DATE: {
                // date
                const DateTimeNode * d = reinterpret_cast<const DateTimeNode*>(val);
                OCI_Date *dt = OCI_ObjectGetDate(obj, cname);
                // TODO/FIXME: tzone? propably OCI_Date from string...
                OCI_DateSetDateTime(dt, d->getYear(), d->getMonth(), d->getDay(),
                                        d->getHour(), d->getMinute(), d->getSecond());
//                 OCI_ObjectSetdate(obj, cname, dt);
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


// #if OCI_VERSION_COMPILE >= OCI_9_0
// 
//             case SQLT_TIMESTAMP:
// 
// //             return MT("TIMESTAMP");
// 
//             case SQLT_TIMESTAMP_TZ:
// 
// //             return MT("TIMESTAMP WITH TIME ZONE");
// 
//             case SQLT_TIMESTAMP_LTZ:
// 
// //             return MT("TIMESTAMP WITH LOCAL TIME ZONE");
// 
//             case SQLT_INTERVAL_YM:
// 
// //             return MT("INTERVAL YEAR TO MONTH");
// 
//             case SQLT_INTERVAL_DS:
// 
// //             return MT("INTERVAL DAY TO SECOND");
// 
// #endif

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
                // object
                OCI_ObjectSetObject(obj, cname, objBindQore(d, n, xsink));
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
                // float
                rv->setKeyValue(cname, new QoreFloatNode(OCI_ObjectGetDouble(obj, cname)), xsink);
                break;
#endif

            case SQLT_DAT:
            case SQLT_TIMESTAMP:
            case SQLT_DATE: {
                OCI_Timestamp * dt = OCI_ObjectGetTimeStamp(obj, cname);
                rv->setKeyValue(cname, get_oracle_timestamp(false, ds, dt->handle, xsink), xsink);
                break;
            }

            case SQLT_TIMESTAMP_TZ:
            case SQLT_TIMESTAMP_LTZ: {
                OCI_Timestamp * dt = OCI_ObjectGetTimeStamp(obj, cname);
                rv->setKeyValue(cname, get_oracle_timestamp(true, ds, dt->handle, xsink), xsink);
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


// #if OCI_VERSION_COMPILE >= OCI_9_0
// 
//             case SQLT_TIMESTAMP:
// 
// //             return MT("TIMESTAMP");
// 
//             case SQLT_TIMESTAMP_TZ:
// 
// //             return MT("TIMESTAMP WITH TIME ZONE");
// 
//             case SQLT_TIMESTAMP_LTZ:
// 
// //             return MT("TIMESTAMP WITH LOCAL TIME ZONE");
// 
//             case SQLT_INTERVAL_YM:
// 
// //             return MT("INTERVAL YEAR TO MONTH");
// 
//             case SQLT_INTERVAL_DS:
// 
// //             return MT("INTERVAL DAY TO SECOND");
// 
// #endif

//             case SQLT_REF:
                // ref

#if OCI_VERSION_COMPILE >= OCI_9_0
            case SQLT_PNTY:
#endif
            case SQLT_NTY:
                rv->setKeyValue(cname, objToQore(OCI_ObjectGetObject(obj, cname), ds, xsink), xsink);
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
//     QoreNodeAsStringHelper str(h, FMT_NONE, xsink);
//     printf("hash= %s\n", str->getBuffer());

    // TODO/FIXME: chech if it's really collection-like list
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
                // float
                OCI_ElemSetDouble(e, val->getAsFloat());
                break;
#endif

//             case SQLT_DAT:
//             case SQLT_ODT:
//             case SQLT_DATE: {
//                 // date
//                 const DateTimeNode * d = reinterpret_cast<const DateTimeNode*>(val);
//                 OCI_Date *dt = OCI_ObjectGetDate(obj, cname);
//                 // TODO/FIXME: tzone? propably OCI_Date from string...
//                 OCI_DateSetDateTime(dt, d->getYear(), d->getMonth(), d->getDay(),
//                                         d->getHour(), d->getMinute(), d->getSecond());
// //                 OCI_ObjectSetdate(obj, cname, dt);
//                 break;
//             }
                

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


// #if OCI_VERSION_COMPILE >= OCI_9_0
// 
//             case SQLT_TIMESTAMP:
// 
// //             return MT("TIMESTAMP");
// 
//             case SQLT_TIMESTAMP_TZ:
// 
// //             return MT("TIMESTAMP WITH TIME ZONE");
// 
//             case SQLT_TIMESTAMP_LTZ:
// 
// //             return MT("TIMESTAMP WITH LOCAL TIME ZONE");
// 
//             case SQLT_INTERVAL_YM:
// 
// //             return MT("INTERVAL YEAR TO MONTH");
// 
//             case SQLT_INTERVAL_DS:
// 
// //             return MT("INTERVAL DAY TO SECOND");
// 
// #endif

//             case SQLT_REF:
                // ref

#if OCI_VERSION_COMPILE >= OCI_9_0
            case SQLT_PNTY:
#endif
            case SQLT_NTY: {
                const QoreHashNode * n = dynamic_cast<const QoreHashNode *>(val);
                if (!n) {
                    xsink->raiseException("BIND-COLLECTION-ERROR", "Object type: '%s' has to be passed as an Oracle Object", col->typinf->name);
                    return obj;
                }
                // object
                OCI_ElemSetObject(e, objBindQore(d, n, xsink));
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
                // float
                rv->set_entry(rv->size(), new QoreFloatNode(OCI_ElemGetDouble(e)), xsink);
                break;
#endif

//             case SQLT_DAT:
//             case SQLT_TIMESTAMP:
//             case SQLT_DATE: {
//                 OCI_Timestamp * dt = OCI_ObjectGetTimeStamp(obj, cname);
//                 rv->set_entry(rv->size(), get_oracle_timestamp(false, ds, dt->handle, xsink), xsink);
//                 break;
//             }
// 
//             case SQLT_TIMESTAMP_TZ:
//             case SQLT_TIMESTAMP_LTZ: {
//                 OCI_Timestamp * dt = OCI_ObjectGetTimeStamp(obj, cname);
//                 rv->set_entry(rv->size(), get_oracle_timestamp(true, ds, dt->handle, xsink), xsink);
//                 break;
//             }
                

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


// #if OCI_VERSION_COMPILE >= OCI_9_0
// 
//             case SQLT_TIMESTAMP:
// 
// //             return MT("TIMESTAMP");
// 
//             case SQLT_TIMESTAMP_TZ:
// 
// //             return MT("TIMESTAMP WITH TIME ZONE");
// 
//             case SQLT_TIMESTAMP_LTZ:
// 
// //             return MT("TIMESTAMP WITH LOCAL TIME ZONE");
// 
//             case SQLT_INTERVAL_YM:
// 
// //             return MT("INTERVAL YEAR TO MONTH");
// 
//             case SQLT_INTERVAL_DS:
// 
// //             return MT("INTERVAL DAY TO SECOND");
// 
// #endif

//             case SQLT_REF:
                // ref

#if OCI_VERSION_COMPILE >= OCI_9_0
            case SQLT_PNTY:
#endif
            case SQLT_NTY:
                rv->set_entry(rv->size(), objToQore(OCI_ElemGetObject(e), ds, xsink), xsink);
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
