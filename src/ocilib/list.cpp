/*
   +----------------------------------------------------------------------+   
   |                                                                      |
   |                     OCILIB - C Driver for Oracle                     |
   |                                                                      |
   |                      (C Wrapper for Oracle OCI)                      |
   |                                                                      |
   +----------------------------------------------------------------------+
   |                      Website : http://www.ocilib.net                 |
   +----------------------------------------------------------------------+
   |               Copyright (c) 2007-2010 Vincent ROGIER                 |
   +----------------------------------------------------------------------+
   | This library is free software; you can redistribute it and/or        |
   | modify it under the terms of the GNU Lesser General Public           |
   | License as published by the Free Software Foundation; either         |
   | version 2 of the License, or (at your option) any later version.     |
   |                                                                      |
   | This library is distributed in the hope that it will be useful,      |
   | but WITHOUT ANY WARRANTY; without even the implied warranty of       |
   | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    |
   | Lesser General Public License for more details.                      |
   |                                                                      |
   | You should have received a copy of the GNU Lesser General Public     |
   | License along with this library; if not, write to the Free           |
   | Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.   |
   +----------------------------------------------------------------------+
   |          Author: Vincent ROGIER <vince.rogier@ocilib.net>            |
   +----------------------------------------------------------------------+ 
*/

/* ------------------------------------------------------------------------ *
 * $Id: list.c, v 3.7.0 2010-07-20 17:45 Vincent Rogier $
 * ------------------------------------------------------------------------ */

#include "ocilib_internal.h"

/* ************************************************************************ *
 *                             PRIVATE FUNCTIONS
 * ************************************************************************ */

/* ------------------------------------------------------------------------ *
 * OCI_ListCreate
 * ------------------------------------------------------------------------ */

OCI_List * OCI_ListCreate2(OCI_Library *pOCILib, int type)
{
   return new OCI_List(type);
}

/* ------------------------------------------------------------------------ *
 * OCI_ListFree
 * ------------------------------------------------------------------------ */

boolean OCI_ListFree(OCI_Library *pOCILib, OCI_List *list)
{
   assert(list);
   
   OCI_ListClear(pOCILib, list);
   delete list;
    
   return TRUE;
}

/* ------------------------------------------------------------------------ *
 * OCI_ListCreateItem
 * ------------------------------------------------------------------------ */

OCI_Item * OCI_ListCreateItem(OCI_Library *pOCILib, int type, int size)
{
    OCI_Item *item  = NULL;

    /* allocate list item entry */

    item = (OCI_Item *) OCI_MemAlloc2(pOCILib, OCI_IPC_LIST_ITEM, sizeof(*item),
                                     (size_t) 1, TRUE);

    if (item != NULL)
    {
        /* allocate item data buffer */

        item->data = (void *) OCI_MemAlloc2(pOCILib, type, (size_t) size, (size_t) 1, TRUE);

        if (item->data == NULL)
            OCI_FREE(item);
    }

    return item;
}

/* ------------------------------------------------------------------------ *
 * OCI_ListAppend
 * ------------------------------------------------------------------------ */

OCI_Item * OCI_ListAppend(OCI_Library *pOCILib, OCI_List *list, int size)
{
    OCI_Item *item = NULL;
    OCI_Item *temp = NULL;

    OCI_CHECK(list == NULL, NULL);

    item = OCI_ListCreateItem(pOCILib, list->type, size);

    OCI_CHECK(item == NULL, FALSE);

    AutoLocker al(list->mutex);

    temp = list->head;

    while (temp != NULL && temp->next)
    {
        temp = temp->next;
    }

    if (temp != NULL)
        temp->next = item;
    else
        list->head = item;

    list->count++;

    return item;
}

/* ------------------------------------------------------------------------ *
 * OCI_ListClear
 * ------------------------------------------------------------------------ */

boolean OCI_ListClear(OCI_Library *pOCILib, OCI_List *list)
{
    OCI_Item *item = NULL;
    OCI_Item *temp = NULL;

    OCI_CHECK(list == NULL, FALSE);

    AutoLocker al(list->mutex);

    // walk along the list to free item's buffer

    item = list->head;

    while (item != NULL)
    {
        temp  = item;
        item  = item->next;

        /* free data */

        OCI_FREE(temp->data);
        OCI_FREE(temp);
    }

    list->head  = NULL;
    list->count = 0;

    return TRUE;
}

/* ------------------------------------------------------------------------ *
 * OCI_ListForEach
 * ------------------------------------------------------------------------ */

boolean OCI_ListForEach(OCI_Library *pOCILib, OCI_List *list, boolean (*proc)(void *))
{
    OCI_Item *item = NULL;

    OCI_CHECK(list == NULL, FALSE);

    AutoLocker al(list->mutex);

    item = list->head;

    /* for each item in the list, execute the given callback */

    while (item != NULL)
    {
        proc(item->data);
        item = item->next;
    }

    return TRUE;
}

/* ------------------------------------------------------------------------ *
 * OCI_ListRemove
 * ------------------------------------------------------------------------ */

boolean OCI_ListRemove(OCI_Library *pOCILib, OCI_List *list, void *data)
{
    OCI_Item *item = NULL;
    OCI_Item *temp = NULL;

    OCI_CHECK(list == NULL, FALSE);
    OCI_CHECK(data == NULL, FALSE);

    AutoLocker al(list->mutex);

    item = list->head;

    while (item != NULL)
    {
        if (item->data == data)
        {
            if (temp) temp->next = item->next;

            /* if item was the first entry, readjust the first list
               entry to next element */

            if (item == list->head) list->head = item->next;

            OCI_FREE(item);

            break;
        }

        temp = item;
        item = item->next;
    }

    list->count--;

    return TRUE;
}
