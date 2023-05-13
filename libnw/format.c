// SPDX-License-Identifier: Unlicense

// Base on https://github.com/cavaliercoder/sysinv

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "libnw.h"

// Macros for printing nodes to JSON
#define NODE_JS_DELIM_NL		"\n"	// New line for JSON output
#define NODE_JS_DELIM_INDENT	"  "	// Tab token for JSON output

// Macros for printing nodes to YAML
#define NODE_YAML_DELIM_NL		"\n"	// New line token for YAML output
#define NODE_YAML_DELIM_INDENT	"    "	// Tab token for YAML output

// Macros for printing nodes to LUA
#define NODE_LUA_DELIM_NL		"\n"	// New line token for LUA output
#define NODE_LUA_DELIM_INDENT	"  "	// Tab token for LUA output

static int indent_depth = 0;

static void fprintcx(FILE* file, LPCSTR s, int count)
{
	int i;
	for (i = 0; i < count; i++)
		fputs(s, file);
}

PNODE NWL_NodeAlloc(LPCSTR name, INT flags)
{
	PNODE node = NULL;
	SIZE_T size;
	SIZE_T name_len = strlen(name) + 1;

	// Calculate required size
	size = sizeof(NODE) + (sizeof(CHAR) * name_len);

	// Allocate
	node = (PNODE)calloc(1, size);
	if (!node)
	{
		fprintf(stderr, "Failed to allocate memory for new node\n");
		exit(ERROR_OUTOFMEMORY);
	}
	node->Children = (PNODE_LINK)calloc(1, sizeof(NODE_LINK));
	node->Attributes = (PNODE_ATT_LINK)calloc(1, sizeof(NODE_ATT_LINK));

	// Copy node name
	node->Name = (LPSTR)(node + 1);
	strcpy_s(node->Name, name_len, name);

	// Set flags
	node->Flags = flags;

	return node;
}

VOID NWL_NodeFree(PNODE node, INT deep)
{
	PNODE_ATT_LINK att;
	PNODE_LINK child;

	// Free attributes
	for (att = &node->Attributes[0]; att->LinkedAttribute; att++)
		free(att->LinkedAttribute);

	// Free children
	if (deep > 0)
	{
		for (child = &node->Children[0]; NULL != child->LinkedNode; child++)
			NWL_NodeFree(child->LinkedNode, deep);
	}

	free(node->Attributes);
	free(node->Children);
	free(node);
}

INT NWL_NodeDepth(PNODE node)
{
	PNODE parent;
	int count = 0;
	for (parent = node; parent->Parent; parent = parent->Parent)
		count++;
	return count;
}

INT NWL_NodeChildCount(PNODE node)
{
	int count = 0;
	PNODE_LINK link = node->Children;
	while (node->Children[count].LinkedNode)
		count++;
	return count;
}

INT NWL_NodeAppendChild(PNODE parent, PNODE child)
{
	int i, old_count, new_count;
	PNODE_LINK new_links;

	// Count old children
	old_count = NWL_NodeChildCount(parent);
	if (NULL == child)
		return old_count;

	new_count = old_count + 1;

	// Allocate new link list
	new_links = (PNODE_LINK)calloc(1ULL + new_count, sizeof(NODE_LINK));
	if (!new_links)
	{
		fprintf(stderr, "Failed to allocate memory for appending node\n");
		exit(ERROR_OUTOFMEMORY);
	}

	// Copy old child links
	for (i = 0; i < old_count; i++)
		new_links[i].LinkedNode = parent->Children[i].LinkedNode;

	// Copy new children
	new_links[new_count - 1].LinkedNode = child;

	// Release old list
	free(parent->Children);
	parent->Children = new_links;

	// Update parent pointer
	child->Parent = parent;

	return new_count;
}

PNODE NWL_NodeAppendNew(PNODE parent, LPCSTR name, INT flags)
{
	PNODE node = NWL_NodeAlloc(name, flags);
	NWL_NodeAppendChild(parent, node);
	return node;
}

PNODE NWL_NodeGetChild(PNODE parent, LPCSTR name)
{
	PNODE_LINK link;
	for (link = &parent->Children[0]; link->LinkedNode != NULL; link++)
	{
		if (strcmp(link->LinkedNode->Name, name) == 0)
			return link->LinkedNode;
	}
	return NULL;
}

static PNODE_ATT NWL_NodeAllocAttr(LPCSTR key, LPCSTR value, int flags)
{
	PNODE_ATT att = NULL;
	LPCSTR nvalue = NULL;
	SIZE_T size;
	SIZE_T key_len, nvalue_len;

	if (NULL == key)
		return att;

	nvalue = value ? value : "";

	key_len = strlen(key) + 1;
	nvalue_len = strlen(nvalue) + 1;
	size = sizeof(NODE_ATT) + key_len + nvalue_len;

	att = (PNODE_ATT)calloc(1, size);
	if (!att)
	{
		fprintf(stderr, "Failed to allocate memory in NWL_NodeAllocAttr\n");
		exit(ERROR_OUTOFMEMORY);
	}

	att->Key = (LPSTR)(att + 1);
	strcpy_s(att->Key, key_len, key);

	att->Value = att->Key + key_len;
	strcpy_s(att->Value, nvalue_len, nvalue);

	att->Flags = flags;

	return att;
}

INT NWL_NodeAttrCount(PNODE node)
{
	int count = 0;
	PNODE_ATT_LINK link = node->Attributes;
	while (node->Attributes[count].LinkedAttribute)
		count++;
	return count;
}

static INT NWL_NodeAttrGetIndex(PNODE node, LPCSTR key)
{
	int i;
	for (i = 0; node->Attributes[i].LinkedAttribute; i++)
	{
		if (strcmp(node->Attributes[i].LinkedAttribute->Key, key) == 0)
			return i;
	}
	return -1;
}

LPSTR NWL_NodeAttrGet(PNODE node, LPCSTR key)
{
	int i = NWL_NodeAttrGetIndex(node, key);
	return (i < 0) ? NULL : node->Attributes[i].LinkedAttribute->Value;
}

PNODE_ATT NWL_NodeAttrSet(PNODE node, LPCSTR key, LPCSTR value, INT flags)
{
	int i, old_count, new_count, new_index;
	PNODE_ATT att;
	PNODE_ATT_LINK link = NULL;
	PNODE_ATT_LINK new_link = NULL;
	PNODE_ATT_LINK new_links = NULL;

	if (!NWLC->HumanSize && (flags & NAFLG_FMT_HUMAN_SIZE))
		flags |= NAFLG_FMT_NUMERIC;
	// Count old attributes
	old_count = NWL_NodeAttrCount(node);

	// Search for existing attribute
	new_index = NWL_NodeAttrGetIndex(node, key);
	if (new_index > -1)
		new_link = &node->Attributes[new_index];

	if (new_link)
	{
		// Replace attribute link with new value if value differs
		if (strcmp(new_link->LinkedAttribute->Value, value) != 0)
		{
			free(new_link->LinkedAttribute);
			new_link->LinkedAttribute = NWL_NodeAllocAttr(key, value, flags);
		}
		else
		{
			// Only update flags if value is identical
			new_link->LinkedAttribute->Flags = flags;
		}
		att = new_link->LinkedAttribute;
	}
	else
	{
		// Reallocate link list and add new attribute
		new_index = old_count;
		new_count = old_count + 1;

		// Allocate new link list
		new_links = (PNODE_ATT_LINK)calloc(1ULL + new_count, sizeof(NODE_ATT_LINK));
		if (!new_links)
		{
			fprintf(stderr, "Failed to allocate memory in node_att_set\n");
			exit(ERROR_OUTOFMEMORY);
		}

		// Copy old child links
		for (i = 0; i < old_count; i++)
			new_links[i].LinkedAttribute = node->Attributes[i].LinkedAttribute;

		// Copy new attribute
		new_links[new_index].LinkedAttribute = NWL_NodeAllocAttr(key, value, flags);

		// Release old list
		free(node->Attributes);
		node->Attributes = new_links;

		att = new_links[new_index].LinkedAttribute;
	}

	return att;
}

PNODE_ATT
NWL_NodeAttrSetf(PNODE node, LPCSTR key, INT flags, LPCSTR _Printf_format_string_ format, ...)
{
	int sz;
	char* buf = NULL;
	PNODE_ATT att = NULL;
	va_list ap;
	va_start(ap, format);
	sz = _vscprintf(format, ap) + 1;
	if (sz <= 0)
	{
		va_end(ap);
		fprintf(stderr, "Failed to calculate string length in NWL_NodeAttrSetf\n");
		exit(ERROR_INVALID_DATA);
	}
	buf = calloc(sizeof(CHAR), sz);
	if (!buf)
	{
		va_end(ap);
		fprintf(stderr, "Failed to allocate memory in NWL_NodeAttrSetf\n");
		exit(ERROR_OUTOFMEMORY);
	}
	vsnprintf(buf, sz, format, ap);
	va_end(ap);
	att = NWL_NodeAttrSet(node, key, buf, flags);
	free(buf);
	return att;
}

static SIZE_T json_escape_content(LPCSTR input, LPSTR buffer, DWORD bufferSize)
{
	LPCSTR cIn = input;
	LPSTR cOut = buffer;

	while (*cIn != '\0')
	{
		// truncate
		if (2UL + (cOut - buffer)>= bufferSize)
			break;
		switch (*cIn)
		{
		case '"':
			memcpy(cOut, "\\\"", 2);
			cOut += 2;
			break;

		case '\\':
			memcpy(cOut, "\\\\", 2);
			cOut += 2;
			break;

		case '\r':
			break;

		case '\n':
			memcpy(cOut, "\\n", 2);
			cOut += 2;
			break;

		default:
			memcpy(cOut, cIn, 1);
			cOut += 1;
			break;
		}

		cIn++;
	}
	*cOut = '\0';

	return cOut - buffer;
}

INT NWL_NodeToJson(PNODE node, FILE* file)
{
	int i = 0;
	int nodes = 1;
	int atts = NWL_NodeAttrCount(node);
	int children = NWL_NodeChildCount(node);
	int plural = 0;
	int indent = indent_depth;

	// Print header
	fprintcx(file, NODE_JS_DELIM_INDENT, indent);
	if (indent_depth > 0 && (node->Flags & NFLG_TABLE_ROW) == 0)
		fprintf(file, "\"%s\": ", node->Name);

	if ((node->Flags & NFLG_TABLE) == 0)
		fputs("{", file);
	else
		fputs("[", file);

	// Print attributes
	if (atts > 0 && (node->Flags & NFLG_TABLE) == 0)
	{
		for (i = 0; i < atts; i++)
		{
			if (*node->Attributes[i].LinkedAttribute->Value != '\0')
			{
				if (plural)
					fputs(",", file);

				// Print attribute name
				fputs(NODE_JS_DELIM_NL, file);
				fprintcx(file, NODE_JS_DELIM_INDENT, indent + 1);
				fprintf(file, "\"%s\": ", node->Attributes[i].LinkedAttribute->Key);

				// Print value
				if (node->Attributes[i].LinkedAttribute->Flags & NAFLG_FMT_NUMERIC)
					fputs(node->Attributes[i].LinkedAttribute->Value, file);
				else
				{
					json_escape_content(node->Attributes[i].LinkedAttribute->Value, NWLC->NwBuf, NWINFO_BUFSZ);
					fprintf(file, "\"%s\"", NWLC->NwBuf);
				}
				plural = 1;
			}
		}
	}

	// Print children
	if (children > 0)
	{
		indent_depth++;
		for (i = 0; i < children; i++)
		{
			if (plural)
				fputs(",", file);

			fputs(NODE_JS_DELIM_NL, file);
			nodes += NWL_NodeToJson(node->Children[i].LinkedNode, file);
			plural = 1;
		}
		indent_depth--;
	}

	if (atts > 0 || children > 0)
	{
		fputs(NODE_JS_DELIM_NL, file);
		fprintcx(file, NODE_JS_DELIM_INDENT, indent);
	}
	if ((node->Flags & NFLG_TABLE) == 0)
		fputs("}", file);
	else
		fputs("]", file);
	return nodes;
}

INT NWL_NodeToYaml(PNODE node, FILE* file)
{
	int i = 0;
	int count = 1;
	int atts = NWL_NodeAttrCount(node);
	int children = NWL_NodeChildCount(node);
	PNODE_ATT att = NULL;
	PNODE child = NULL;
	CHAR* attVal = NULL;

	if (!node->Parent)
		fputs("---"NODE_YAML_DELIM_NL, file);

	fprintcx(file, NODE_YAML_DELIM_INDENT, indent_depth);

	if (NFLG_TABLE_ROW & node->Flags)
		fputs("- ", file);

	fprintf(file, "%s:", node->Name);

	// Print attributes
	if (atts > 0)
	{
		fputs(NODE_YAML_DELIM_NL, file);
		for (i = 0; i < atts; i++)
		{
			att = node->Attributes[i].LinkedAttribute;
			attVal = (att->Value && *att->Value != '\0') ? att->Value : "~";

			fprintcx(file, NODE_YAML_DELIM_INDENT, indent_depth + 1);
			if (att->Flags & NAFLG_FMT_NEED_QUOTE)
				fprintf(file, "%s: '%s'%s", att->Key, attVal, NODE_YAML_DELIM_NL);
			else
				fprintf(file, "%s: %s%s", att->Key, attVal, NODE_YAML_DELIM_NL);
		}
	}

	// Print children
	if (children > 0)
	{
		if (atts == 0)
			fputs(NODE_YAML_DELIM_NL, file);
		indent_depth++;
		for (i = 0; i < children; i++)
		{
			child = node->Children[i].LinkedNode;
			NWL_NodeToYaml(child, file);
		}
		indent_depth--;
	}
	else if (atts == 0)
	{
		fputs(" ~"NODE_YAML_DELIM_NL, file);
	}

	return count;
}

INT NWL_NodeToLua(PNODE node, FILE* file)
{
	int i = 0;
	int nodes = 1;
	int atts = NWL_NodeAttrCount(node);
	int children = NWL_NodeChildCount(node);
	int plural = 0;
	int indent = indent_depth;

	if (!node->Parent)
		fputs("#!lua"NODE_LUA_DELIM_NL"_NWINFO = ", file);

	// Print header
	fprintcx(file, NODE_LUA_DELIM_INDENT, indent);
	if (indent_depth > 0 && (node->Flags & NFLG_TABLE_ROW) == 0)
		fprintf(file, "[\"%s\"] = ", node->Name);

	//if ((node->Flags & NFLG_TABLE) == 0)
	fputs("{", file);

	// Print attributes
	if (atts > 0 && (node->Flags & NFLG_TABLE) == 0)
	{
		for (i = 0; i < atts; i++)
		{
			if (*node->Attributes[i].LinkedAttribute->Value != '\0')
			{
				if (plural)
					fputs(",", file);

				// Print attribute name
				fputs(NODE_LUA_DELIM_NL, file);
				fprintcx(file, NODE_LUA_DELIM_INDENT, indent + 1);
				fprintf(file, "[\"%s\"] = ", node->Attributes[i].LinkedAttribute->Key);

				// Print value
				json_escape_content(node->Attributes[i].LinkedAttribute->Value, NWLC->NwBuf, NWINFO_BUFSZ);
				fprintf(file, "\"%s\"", NWLC->NwBuf);
				plural = 1;
			}
		}
	}

	// Print children
	if (children > 0)
	{
		indent_depth++;
		for (i = 0; i < children; i++)
		{
			if (plural)
				fputs(",", file);

			fputs(NODE_LUA_DELIM_NL, file);
			nodes += NWL_NodeToLua(node->Children[i].LinkedNode, file);
			plural = 1;
		}
		indent_depth--;
	}

	if (atts > 0 || children > 0)
	{
		fputs(NODE_LUA_DELIM_NL, file);
		fprintcx(file, NODE_LUA_DELIM_INDENT, indent);
	}
	//if ((node->Flags & NFLG_TABLE) == 0)
	fputs("}", file);
	return nodes;
}
