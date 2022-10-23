/****************************************************************************
**
** Copyright (C) Eageo Information & Exploration Technology Co.,Ltd.,
**
** Use of this file is limited according to the terms specified by
** Eageo Exploration & Information Technology Co.,Ltd.  Details of
** those terms are listed in licence.txt included as part of the distribution
** package ** of this file. This file may not be distributed without including
** the licence.txt file.
**
** Contact aben.lee@foxmail.com if any conditions of this licensing are
** not clear to you.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/


#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "cn-cbor.h"



#ifdef USE_CBOR_CONTEXT
extern cn_cbor_context *context;
#define CBOR_CONTEXT_PARAM , context
#define CBOR_CONTEXT_PARAM_COMMA context,
#else
#define CBOR_CONTEXT_PARAM
#define CBOR_CONTEXT_PARAM_COMMA
#endif

cn_cbor *cn_cbor_parse_json(char *jsonString, int offset, int length)
{
    char ch;
    int ib2;
    cn_cbor *parent = NULL;
    cn_cbor *root = NULL;

    for (; offset < length; offset++) {
        cn_cbor *node = NULL;
        ch = jsonString[offset];
        switch (ch) {
            case '{':
                node = cn_cbor_map_create(CBOR_CONTEXT_PARAM_COMMA NULL);
                break;

            case '[':
                node = cn_cbor_array_create(CBOR_CONTEXT_PARAM_COMMA NULL);
                break;

            case '}':
            case ']':
                if (parent == NULL) {
                    fprintf(stderr, "Parse failure @ '%s'\n", &jsonString[offset]); // to be removed
                    return NULL;
                }
                parent = parent->parent;
                break;

            case ' ':
            case '\r':
            case '\n':
            case ':':
            case ',':
                break;

            case '"':
                for (ib2 = offset + 1; ib2 < length; ib2++) {
                    if (jsonString[ib2] == '"') {
                        break;
                    }
                }
                size_t strlength = ib2-offset;
                char* buf = (char* ) malloc(strlength * sizeof( char));
                memset( buf,0,strlength);//bzero(buf,strlength);
                strncpy(buf,&jsonString[offset+1],strlength-1);
                node = cn_cbor_string_create(
                    buf, CBOR_CONTEXT_PARAM_COMMA NULL);
                offset = ib2;
                break;
            case 't':
                if (strncmp(&jsonString[offset], "true", 4) != 0) {
                    goto error;
                }
                node =
                    cn_cbor_data_create(NULL, 0, CBOR_CONTEXT_PARAM_COMMA NULL);
                node->type = CN_CBOR_TRUE;
                offset += 3;
                break;

            case 'f':
                if (strncmp(&jsonString[offset], "false", 5) != 0) {
                    goto error;
                }
                node =
                    cn_cbor_data_create(NULL, 0, CBOR_CONTEXT_PARAM_COMMA NULL);
                node->type = CN_CBOR_FALSE;
                offset += 4;
                break;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '-':
                node = cn_cbor_int_create(
                    atol(&jsonString[offset]), CBOR_CONTEXT_PARAM_COMMA NULL);
                if (jsonString[offset] == '-') {
                    offset++;
                }
                while (isdigit(jsonString[offset])) {
                    offset++;
                }
                offset--;
                break;

            default:
            error:
                fprintf(stderr, "Parse failure @ '%s'\n", &jsonString[offset]);
                return NULL;
        }

        if ((node != NULL) && (parent != NULL)) {
            node->parent = parent;
            if (parent->last_child != NULL) {
                parent->last_child->next = node;
                parent->last_child = node;
            }
            else {
                parent->first_child = node;
            }
            parent->last_child = node;
            parent->length++;

            if ((node->type == CN_CBOR_MAP) || (node->type == CN_CBOR_ARRAY)) {
                parent = node;
            }
        }
        if (parent == NULL) {
            parent = node;
            if (root == NULL) {
                root = node;
            }
        }
    }

    return root;
}

