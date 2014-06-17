/*
    +-----------------------------------------------------------------------------------------+
    |                                                                                         |
    |                               OCILIB - C Driver for Oracle                              |
    |                                                                                         |
    |                                (C Wrapper for Oracle OCI)                               |
    |                                                                                         |
    |                              Website : http://www.ocilib.net                            |
    |                                                                                         |
    |             Copyright (c) 2007-2012 Vincent ROGIER <vince.rogier@ocilib.net>            |
    |                                                                                         |
    +-----------------------------------------------------------------------------------------+
    |                                                                                         |
    |             This library is free software; you can redistribute it and/or               |
    |             modify it under the terms of the GNU Lesser General Public                  |
    |             License as published by the Free Software Foundation; either                |
    |             version 2 of the License, or (at your option) any later version.            |
    |                                                                                         |
    |             This library is distributed in the hope that it will be useful,             |
    |             but WITHOUT ANY WARRANTY; without even the implied warranty of              |
    |             MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU           |
    |             Lesser General Public License for more details.                             |
    |                                                                                         |
    |             You should have received a copy of the GNU Lesser General Public            |
    |             License along with this library; if not, write to the Free                  |
    |             Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.          |
    |                                                                                         |
    +-----------------------------------------------------------------------------------------+
*/

/* --------------------------------------------------------------------------------------------- *
 * $Id: agent.c, Vincent Rogier $
 * --------------------------------------------------------------------------------------------- */

#include "ocilib_internal.h"

/* ********************************************************************************************* *
 *                             PRIVATE FUNCTIONS
 * ********************************************************************************************* */

/* --------------------------------------------------------------------------------------------- *
 * OCI_AgentInit
 * --------------------------------------------------------------------------------------------- */
/*
OCI_Agent * OCI_AgentInit
(
    OCI_Library *pOCILib,
    OCI_Connection *con,
    OCI_Agent     **pagent,
    OCIAQAgent     *handle,
    const mtext    *name,
    const mtext    *address
)
{
    OCI_Agent *agent = NULL;
    boolean res      = TRUE;

    OCI_CHECK(pagent == NULL, NULL);

    // allocate agent structure

    if (*pagent == NULL)
    {
        *pagent = (OCI_Agent *) OCI_MemAlloc2(pOCILib, OCI_IPC_AGENT, sizeof(*agent),  (size_t) 1, TRUE);
    }

    if (*pagent != NULL)
    {
        agent = *pagent;

        // reinit

        OCI_FREE(agent->name);
        OCI_FREE(agent->address);

        agent->con    = con;
        agent->handle = handle;

        if (handle == NULL)
        {
            agent->hstate = OCI_OBJECT_ALLOCATED;

            res = (OCI_SUCCESS == OCI_DescriptorAlloc2(pOCILib,
            										  (dvoid * ) pOCILib->env,
                                                      (dvoid **) &agent->handle,
                                                      OCI_DTYPE_AQAGENT,
                                                      (size_t) 0, (dvoid **) NULL));
        }
        else
        {
            agent->hstate = OCI_OBJECT_FETCHED_CLEAN;
        }

        // set name attribute if provided

        if ((res == TRUE) && (name != NULL) && (name[0] != 0))
        {
            res = OCI_AgentSetName(pOCILib, agent, name);
        }

        // set address attribute if provided

        if ((res == TRUE) && (address != NULL) && (address[0] != 0))
        {
            res = OCI_AgentSetAddress(pOCILib, agent, address);
        }
    }
    else
    {
        res = FALSE;
    }

    // check for failure

    if (res == FALSE)
    {
        OCI_AgentFree(pOCILib, agent);
        agent = NULL;
    }

    return agent;
}
*/

/* ********************************************************************************************* *
 *                            PUBLIC FUNCTIONS
 * ********************************************************************************************* */

/* --------------------------------------------------------------------------------------------- *
 * OCI_AgentCreate
 * --------------------------------------------------------------------------------------------- */
 /*
OCI_Agent * OCI_API OCI_AgentCreate
(
    OCI_Library *pOCILib,
    OCI_Connection *con,
    const mtext    *name,
    const mtext    *address
)
{
    OCI_Agent *agent = NULL;

    OCI_CHECK_INITIALIZED2(pOCILib, NULL);

    OCI_CHECK_PTR(pOCILib, OCI_IPC_CONNECTION, con, NULL);

    agent = OCI_AgentInit(pOCILib, con, &agent, NULL, name, address);

    OCI_RESULT(pOCILib, agent != NULL);

    return agent;
}
 */
/* --------------------------------------------------------------------------------------------- *
 * OCI_AgentFree
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_AgentFree
(
    OCI_Library *pOCILib,
    OCI_Agent *agent
)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_AGENT, agent, FALSE);

    if (agent->hstate == OCI_OBJECT_ALLOCATED)
    {
        OCI_DescriptorFree2(pOCILib, (dvoid *) agent->handle, OCI_DTYPE_AQAGENT);
    }

    OCI_FREE(agent->address);
    OCI_FREE(agent->name);

    OCI_FREE(agent);

    OCI_RESULT(pOCILib, res);

    return res;
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_AgentGetName
 * --------------------------------------------------------------------------------------------- */
/*
const mtext * OCI_API OCI_AgentGetName
(
	OCI_Library *pOCILib,
    OCI_Agent *agent
)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_AGENT, agent, NULL);

    if (agent->name == NULL)
    {
        res = OCI_StringGetFromAttrHandle(pOCILib, agent->con, agent->handle,  OCI_DTYPE_AQAGENT,
                                          OCI_ATTR_AGENT_NAME,  &agent->name);
    }

    OCI_RESULT(pOCILib, res);

    return agent->name;
}
*/
/* --------------------------------------------------------------------------------------------- *
 * OCI_AgentSetName
 * --------------------------------------------------------------------------------------------- */
 /*
boolean OCI_API OCI_AgentSetName
(
    OCI_Library *pOCILib,
    OCI_Agent   *agent,
    const mtext *name,
    ExceptionSink* xsink
)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_AGENT, agent, FALSE);

    res =  OCI_StringSetToAttrHandle(pOCILib, agent->con, agent->handle,  OCI_DTYPE_AQAGENT,
                                     OCI_ATTR_AGENT_NAME, &agent->name, name, xsink);

    OCI_RESULT(pOCILib, res);

    return res;
}
 */
/* --------------------------------------------------------------------------------------------- *
 * OCI_AgentGetAddress
 * --------------------------------------------------------------------------------------------- */
/*
const mtext * OCI_API OCI_AgentGetAddress
(
    OCI_Library *pOCILib,
    OCI_Agent *agent
)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_AGENT, agent, NULL);

    if (agent->name == NULL)
    {
        res = OCI_StringGetFromAttrHandle(pOCILib, agent->con, agent->handle, OCI_DTYPE_AQAGENT,
                                          OCI_ATTR_AGENT_ADDRESS, &agent->address);
    }

    OCI_RESULT(pOCILib, res);

    return agent->address;
}
*/
/* --------------------------------------------------------------------------------------------- *
 * OCI_AgentSetAddress
 * --------------------------------------------------------------------------------------------- */
 /*
boolean OCI_API OCI_AgentSetAddress
(
    OCI_Library *pOCILib,
    OCI_Agent   *agent,
    const mtext *address,
    ExceptionSink* xsink
)
{
    boolean res = TRUE;

    OCI_CHECK_PTR(pOCILib, OCI_IPC_AGENT, agent, FALSE);

    res = OCI_StringSetToAttrHandle(pOCILib, agent->con, agent->handle, OCI_DTYPE_AQAGENT,
                                    OCI_ATTR_AGENT_ADDRESS, &agent->address, address, xsink);

    OCI_RESULT(pOCILib, res);

    return res;
}
 */
