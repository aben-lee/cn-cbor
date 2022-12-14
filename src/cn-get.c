#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "dll-export.h"

#include "cn-cbor.h"

CN_CBOR_EXPORT
cn_cbor* cn_cbor_mapget_int(const cn_cbor* cb, int key)
{
	cn_cbor* cp;
	assert(cb);
	if (cb == NULL) {
		return NULL;
	}
	for (cp = cb->first_child; cp && cp->next; cp = cp->next->next) {
		switch (cp->type) {
			case CN_CBOR_UINT:
				if (cp->v.uint == (unsigned long)key) {
					return cp->next;
				}
				break;
			case CN_CBOR_INT:
				if (cp->v.sint == (long)key) {
					return cp->next;
				}
				break;
			default:;  // skip non-integer keys
		}
	}
	return NULL;
}

CN_CBOR_EXPORT
cn_cbor* cn_cbor_mapget_string(const cn_cbor* cb, const char* key)
{
	cn_cbor* cp = NULL;
	assert(cb != NULL);
	assert(key != NULL);
	if (cb == NULL || key == NULL) {
		return NULL;
	}
	
	size_t keylen = strlen(key);
	for (cp = cb->first_child; cp && cp->next; cp = cp->next->next) {
		switch (cp->type) {
			case CN_CBOR_TEXT:	// fall-through
			case CN_CBOR_BYTES:
				if (keylen != cp->length) {
					continue;
				}
				if (memcmp(key, cp->v.str, keylen) == 0) {
					return cp->next;
				}
			default:;  // skip non-string keys
		}
	}
	return NULL;
}

CN_CBOR_EXPORT
cn_cbor* cn_cbor_index(const cn_cbor* cb, unsigned int idx)
{
	cn_cbor* cp;
	unsigned int i = 0;
	assert(cb);
	if (cb == NULL) {
		return NULL;
	}
	for (cp = cb->first_child; cp; cp = cp->next) {
		if (i == idx) {
			return cp;
		}
		i++;
	}
	return NULL;
}
