/*
* Windows kernel-mode driver for controlling access to various input devices.
* Copyright (C) 2016-2018  Benjamin H�glinger-Stelzer
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#pragma once

#define PID_LIST_TAG    'LPGH'
#define SYSTEM_PID      0x04

#ifndef _KERNEL_MODE
#include <stdlib.h>
#endif

typedef struct _PID_LIST_NODE
{
    ULONG Pid;

    BOOLEAN IsAllowed;

    struct _PID_LIST_NODE* next;

} PID_LIST_NODE, *PPID_LIST_NODE;

PPID_LIST_NODE FORCEINLINE PID_LIST_CREATE()
{
    PPID_LIST_NODE head;

#ifdef _KERNEL_MODE
    head = ExAllocatePoolWithTag(NonPagedPool, sizeof(PID_LIST_NODE), PID_LIST_TAG);
#else
    head = (PPID_LIST_NODE)malloc(sizeof(PID_LIST_NODE));
#endif

    if (head == NULL) {
        return head;
    }

    RtlZeroMemory(head, sizeof(PID_LIST_NODE));

    return head;
}

VOID FORCEINLINE PID_LIST_DESTROY(PID_LIST_NODE ** head)
{
    PPID_LIST_NODE current = *head;
    PPID_LIST_NODE temp_node = NULL;

    if (current == NULL)
        return;

    while (current->next != NULL) {
        temp_node = current;
        current = current->next;
#ifdef _KERNEL_MODE
        ExFreePoolWithTag(temp_node, PID_LIST_TAG);
#else
        free(temp_node);
#endif
    }
}

BOOLEAN FORCEINLINE PID_LIST_PUSH(PID_LIST_NODE ** head, ULONG pid, BOOLEAN allowed)
{
    PPID_LIST_NODE new_node;

    if (*head == NULL)
        return FALSE;

#ifdef _KERNEL_MODE
    new_node = ExAllocatePoolWithTag(NonPagedPool, sizeof(PID_LIST_NODE), PID_LIST_TAG);
#else
    new_node = (PPID_LIST_NODE)malloc(sizeof(PID_LIST_NODE));
#endif

    if (new_node == NULL) {
        return FALSE;
    }

    new_node->Pid = pid;
    new_node->IsAllowed = allowed;
    new_node->next = *head;
    *head = new_node;

    return TRUE;
}

BOOLEAN FORCEINLINE PID_LIST_REMOVE_BY_PID(PID_LIST_NODE ** head, ULONG pid)
{
    PPID_LIST_NODE current = *head;
    PPID_LIST_NODE temp_node = NULL;

    if (current == NULL)
        return FALSE;

    if (pid == SYSTEM_PID)
        return FALSE;

    //
    // Search for PID
    // 
    while (current->next != NULL) {
        if (current->Pid == pid) {
            break;
        }
        current = current->next;
    }

    if (current->next == NULL) {
        //
        // PID wasn't found in the list
        // 
        return FALSE;
    }

    //
    // Re-link list and free disposed node
    // 
    temp_node = current->next;
    current->next = temp_node->next;
#ifdef _KERNEL_MODE
    ExFreePoolWithTag(temp_node, PID_LIST_TAG);
#else
    free(temp_node);
#endif

    return TRUE;
}

BOOLEAN FORCEINLINE PID_LIST_CONTAINS(PID_LIST_NODE ** head, ULONG pid, BOOLEAN* allowed)
{
    PPID_LIST_NODE current = *head;

    if (current == NULL)
        return FALSE;

    //
    // Search for PID
    // 
    while (current->next != NULL) {
        if (current->Pid == pid) {
            if (allowed != NULL) {
                *allowed = current->IsAllowed;
            }
            return TRUE;
        }
        current = current->next;
    }

    return FALSE;
}
