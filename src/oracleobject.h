#ifndef ORACLEOBJECT_H
#define ORACLEOBJECT_H

#include "../config.h"
#include <qore/Qore.h>
#include <map>
#include "ocilib.h"

class OracleData;

/*! A ocilib-like error handler to catch OCILIB raised errors
 */
void ocilib_err_handler(OCI_Error *err);


/* Oracle Named Types - Objects
 */

/*! Bind a special structured qore hash as an Oracle Object
    into the statement - used with %v.
    \param d reference to OracleData with successfully set connection
    \param h qore hash node prepared with f_oracle_object()/bindOracleObject()
    \param xsink exception hanlder
    \retval OCI_Object* always initialized instance. Xsink status has to be checked
                        elsewhere. The instance has to be freed in any case.
 */
OCI_Object* objBindQore(OracleData * d,
                        const QoreHashNode * h,
                        ExceptionSink * xsink);

/*! Bind a an Oracle Object as a placeholder for OUT variables
    into the statement - used with :name.
    Returned object is empty and has to be set with OCI call of OCIObjectBind().
    \param d reference to OracleData with successfully set connection
    \param h qore string node with oracle type name.
    \param xsink exception hanlder
    \retval OCI_Object* always initialized instance. Xsink status has to be checked
                        elsewhere. The instance has to be freed in any case.
 */
OCI_Object* objPlaceholderQore(OracleData * d_ora,
                               const char * tname,
                               ExceptionSink *xsink);

/*! Convert an Oracle Object into Qore Hash node.
    The hash is plain Qore hash - no Object-hash structure is set.
    \param obj Oracle Object instance
    \param ds DBI Datasource instance
    \param xsink exception hanlder
    \retval AbstractQoreNode* Plain Qore hash node
 */
AbstractQoreNode* objToQore(OCI_Object * obj,
                            Datasource *ds,
                            ExceptionSink *xsink);


/* Oracle Named Types - Collections
 */
/*! Bind a special structured qore hash as an Oracle Collection
    into the statement - used with %v.
    \param d reference to OracleData with successfully set connection
    \param h qore hash node prepared with f_oracle_collection()/bindOracleCollection()
    \param xsink exception hanlder
    \retval OCI_Coll* always initialized instance. Xsink status has to be checked
                        elsewhere. The instance has to be freed in any case.
 */
OCI_Coll* collBindQore(OracleData * d,
                       const QoreHashNode * h,
                       ExceptionSink * xsink);

/*! Bind a an Oracle Collection as a placeholder for OUT variables
    into the statement - used with :name.
    Returned collection is empty and has to be set with OCI call of OCIObjectBind().
    \param d reference to OracleData with successfully set connection
    \param h qore string node with oracle type name.
    \param xsink exception hanlder
    \retval OCI_Coll* always initialized instance. Xsink status has to be checked
                        elsewhere. The instance has to be freed in any case.
 */
OCI_Coll* collPlaceholderQore(OracleData * d_ora,
                              const char * tname,
                              ExceptionSink *xsink);

/*! Convert an Oracle Collection into Qore List node.
    The hash is plain Qore list - no Object-hash structure is set.
    \param obj Oracle Collection instance
    \param ds DBI Datasource instance
    \param xsink exception hanlder
    \retval AbstractQoreNode* Plain Qore list node
 */
AbstractQoreNode* collToQore(OCI_Coll * obj,
                             Datasource *ds,
                             ExceptionSink *xsink);


/* Binding functions
 */

/*! Implementation of bindOracleObject(typename, hash)
    Returning hash structure:
    "type" : ORACLE_OBJECT string ("OracleObject")
    "^oratype^" : type name
    "^values^" : a plain hash with column_name : value
 */
DLLEXPORT AbstractQoreNode * f_oracle_object(const QoreListNode *params, ExceptionSink *xsink);

/*! Implementation of placeholderOracleObject(typename)
    Returning hash structure (used in OraBindGroup::parseQuery):
    "type" : ORACLE_OBJECT string ("OracleObject")
    "value" : string node - type name
 */
DLLEXPORT AbstractQoreNode * f_oracle_object_placeholder(const QoreListNode *params, ExceptionSink *xsink);

/*! Implementation of bindOracleCollection(typename, list)
    Returning hash structure:
    "type" : ORACLE_COLLECTION string ("OracleCollection")
    "^oratype^" : type name
    "^values^" : a plain list with column_name : value
 */
DLLEXPORT AbstractQoreNode * f_oracle_collection(const QoreListNode *params, ExceptionSink *xsink);

/*! Implementation of placeholderOracleCollection(typename)
    Returning hash structure (used in OraBindGroup::parseQuery):
    "type" : ORACLE_COLLECTION string ("OracleCollection")
    "value" : string node - type name
 */
DLLEXPORT AbstractQoreNode * f_oracle_collection_placeholder(const QoreListNode *params, ExceptionSink *xsink);


#endif
