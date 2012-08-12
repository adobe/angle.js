//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// This file contains the JS specific functions.
#include "compiler/osinclude.h"
#include <map>

#if !defined(ANGLE_OS_JS)
#error Trying to build a JS specific file in a non-JS build.
#endif

static OS_TLSIndex s_NextTLSIndex = 1;
typedef std::map<OS_TLSIndex, void*> TLSValuesMap;
static TLSValuesMap* s_TLSValues = 0;

//
// Thread Local Storage Operations
//
OS_TLSIndex OS_AllocTLSIndex()
{
    return ++s_NextTLSIndex;
}


bool OS_SetTLSValue(OS_TLSIndex nIndex, void *lpvValue)
{
    if (nIndex == OS_INVALID_TLS_INDEX) {
        assert(0 && "OS_SetTLSValue(): Invalid TLS Index");
        return false;
    }
    if (!s_TLSValues)
    	s_TLSValues = new TLSValuesMap();
    s_TLSValues->insert(std::pair<OS_TLSIndex, void*>(nIndex, lpvValue));
    return true;
}


bool OS_FreeTLSIndex(OS_TLSIndex nIndex)
{
    if (nIndex == OS_INVALID_TLS_INDEX) {
        assert(0 && "OS_SetTLSValue(): Invalid TLS Index");
        return false;
    }

    if (!s_TLSValues)
    	return false;
    size_t itemsDeleted = s_TLSValues->erase(nIndex);
    if (!itemsDeleted)
    	return false;
    if (s_TLSValues->empty()) {
    	delete s_TLSValues;
	    s_TLSValues = 0;
	}
	return true;
}

void* OS_GetTLSValue(OS_TLSIndex nIndex)
{
	ASSERT(nIndex != OS_INVALID_TLS_INDEX);
	if (!s_TLSValues)
		return 0;
	TLSValuesMap::iterator iter = s_TLSValues->find(nIndex);
	if (iter == s_TLSValues->end())
		return 0;
	return iter->second;
}
