#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "cn-cbor/cn-cbor.h"

#ifdef USE_CBOR_CONTEXT
#define CBOR_CONTEXT_PARAM , NULL
#else
#define CBOR_CONTEXT_PARAM
#endif

#define ERROR(msg, p) fprintf(stderr, "ERROR: " msg " %s\n", (p));
#ifdef USE_CBOR_CONTEXT
extern cn_cbor_context *context;
#define CBOR_CONTEXT_PARAM , context
#define CBOR_CONTEXT_PARAM_COMMA context,
#else
#define CBOR_CONTEXT_PARAM
#define CBOR_CONTEXT_PARAM_COMMA
#endif

cn_cbor *Json2Cbor(char *jsonString, int offset, int length)
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
				bzero(buf,strlength);
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



static unsigned char *load_file(const char *filepath, unsigned char **end)
{
	struct stat st;
	if (stat(filepath, &st) == -1) {
		ERROR("can't find file", filepath);
		return 0;
	}
	int fd = open(filepath, O_RDONLY);
	if (fd == -1) {
		ERROR("can't open file", filepath);
		return 0;
	}
	unsigned char *text = malloc(st.st_size + 1);  // this is not going to be freed
	if (st.st_size != read(fd, text, st.st_size)) {
		ERROR("can't read file", filepath);
		close(fd);
		return 0;
	}
	close(fd);
	text[st.st_size] = '\0';
	*end = text + st.st_size;
	return text;
}

static void dump(const cn_cbor *cb, char *out, char **end, int indent)
{
	if (!cb)
		goto done;
	int i;
	cn_cbor *cp;
	char finchar = ')'; /* most likely */

#define CPY(s, l)      \
	memcpy(out, s, l); \
	out += l;
#define OUT(s) CPY(s, sizeof(s) - 1)
#define PRF(f, a) out += sprintf(out, f, a)

	for (i = 0; i < indent; i++)
		*out++ = ' ';
	switch (cb->type) {
		case CN_CBOR_TEXT_CHUNKED:
			OUT("(_\n");
			goto sequence;
		case CN_CBOR_BYTES_CHUNKED:
			OUT("(_\n\n");
			goto sequence;
		case CN_CBOR_TAG:
			PRF("%ld(\n", cb->v.sint);
			goto sequence;
		case CN_CBOR_ARRAY:
			finchar = ']';
			OUT("[\n");
			goto sequence;
		case CN_CBOR_MAP:
			finchar = '}';
			OUT("{\n");
			goto sequence;
		sequence:
			for (cp = cb->first_child; cp; cp = cp->next) {
				dump(cp, out, &out, indent + 2);
			}
			for (i = 0; i < indent; i++)
				*out++ = ' ';
			*out++ = finchar;
			break;
		case CN_CBOR_BYTES:
			OUT("h'");
			for (i = 0; i < cb->length; i++)
				PRF("%02x", cb->v.str[i] & 0xff);
			*out++ = '\'';
			break;
		case CN_CBOR_TEXT:
			*out++ = '"';
			CPY(cb->v.str, cb->length); /* should escape stuff */
			*out++ = '"';
			break;
		case CN_CBOR_NULL:
			OUT("null");
			break;
		case CN_CBOR_TRUE:
			OUT("true");
			break;
		case CN_CBOR_FALSE:
			OUT("false");
			break;
		case CN_CBOR_UNDEF:
			OUT("simple(23)");
			break;
		case CN_CBOR_INT:
			PRF("%ld", cb->v.sint);
			break;
		case CN_CBOR_UINT:
			PRF("%lu", cb->v.uint);
			break;
		case CN_CBOR_DOUBLE:
			PRF("%e", cb->v.dbl);
			break;
		case CN_CBOR_SIMPLE:
			PRF("simple(%ld)", cb->v.sint);
			break;
		default:
			PRF("???%d???", cb->type);
			break;
	}
	*out++ = '\n';
done:
	*end = out;
}

const char *err_name[] = {
	"CN_CBOR_NO_ERROR",
	"CN_CBOR_ERR_OUT_OF_DATA",
	"CN_CBOR_ERR_NOT_ALL_DATA_CONSUMED",
	"CN_CBOR_ERR_ODD_SIZE_INDEF_MAP",
	"CN_CBOR_ERR_BREAK_OUTSIDE_INDEF",
	"CN_CBOR_ERR_MT_UNDEF_FOR_INDEF",
	"CN_CBOR_ERR_RESERVED_AI",
	"CN_CBOR_ERR_WRONG_NESTING_IN_INDEF_STRING",
	"CN_CBOR_ERR_OUT_OF_MEMORY",
	"CN_CBOR_ERR_FLOAT_NOT_SUPPORTED",
};

static void cn_cbor_decode_test(const unsigned char *buf, int len)
{
	struct cn_cbor_errback back;
	const cn_cbor *ret = cn_cbor_decode(buf, len CBOR_CONTEXT_PARAM, &back);
	if (ret)
		printf("oops 1");
	printf("%s at %d\n", err_name[back.err], back.pos);
}

int main(void)
{
	char buf[100000];
	unsigned char *end;
	char *bufend;
	unsigned char *s = load_file("cases.cbor", &end);
	printf("%zd\n", end - s);
	cn_cbor *cb = cn_cbor_decode(s, end - s CBOR_CONTEXT_PARAM, 0);
	if (cb) {
		dump(cb, buf, &bufend, 0);
		*bufend = 0;
		printf("%s\n", buf);
		cn_cbor_free(cb CBOR_CONTEXT_PARAM);
		cb = 0; /* for leaks testing */
	}
	cn_cbor_decode_test((const unsigned char *)"\xff", 1);		   /* break outside indef */
	cn_cbor_decode_test((const unsigned char *)"\x1f", 1);		   /* mt undef for indef */
	cn_cbor_decode_test((const unsigned char *)"\x00\x00", 2);	   /* not all data consumed */
	cn_cbor_decode_test((const unsigned char *)"\x81", 1);		   /* out of data */
	cn_cbor_decode_test((const unsigned char *)"\x1c", 1);		   /* reserved ai */
	cn_cbor_decode_test((const unsigned char *)"\xbf\x00\xff", 3); /* odd size indef map */
	cn_cbor_decode_test((const unsigned char *)"\x7f\x40\xff", 3); /* wrong nesting in indef string */
	system("leaks test");
}

/* cn-cbor.c:112:    CN_CBOR_FAIL("out of memory"); */
