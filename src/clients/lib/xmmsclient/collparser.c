/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003-2006 XMMS2 Team
 *
 *  PLUGINS ARE NOT CONSIDERED TO BE DERIVED WORK !!!
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "xmmsclient/xmmsclient.h"
#include "xmmsclientpriv/xmmsclient.h"
#include "xmmsc/xmmsc_idnumbers.h"


#define XMMS_COLLECTION_PARSER_DEFAULT_NAMESPACE "Collections"

/* Properties to match by default. */
char *coll_autofilter[] = { "artist", "album", "title", NULL };

typedef struct {
	char  shortstr;
	char *longstr;
} xmmsc_coll_prop_t;

xmmsc_coll_prop_t 
xmmsc_coll_prop_short[] = { { 'a', "artist" },
                            { 'l', "album" },
                            { 't', "title" },
                            { 'n', "tracknr" },
                            { 'y', "year" },
                            { 'g', "genre" },
                            { 'u', "url" },
                            { '\0', NULL } };


#define TOKEN_MATCH_CHAR(symbol, type) if (*tmp == (symbol)) { *newpos = tmp + 1; return coll_token_new (type, NULL); }
#define TOKEN_MATCH_STRING(expr, type) if (strncmp (expr, tmp, strlen (expr)) == 0) { *newpos = tmp + strlen (expr); return coll_token_new (type, NULL); }

#define TOKEN_ASSERT(token, tktype) do { \
	if (!token || (token->type != tktype)) { \
		*ret = NULL; \
		return tokens; \
	} \
} while (0)

#define PARSER_TRY(func) do { \
	pos = func (tokens, &coll); \
	if (coll) { \
		*ret = coll; \
		return pos; \
	} \
} while (0)


static int coll_parse_prepare (xmmsc_coll_token_t *tokens);

static xmmsc_coll_token_t *coll_parse_expr (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret);
static xmmsc_coll_token_t *coll_parse_parenexpr (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret);
static xmmsc_coll_token_t *coll_parse_sequence (xmmsc_coll_token_t *token, const char *field, xmmsc_coll_t **ret);
static xmmsc_coll_token_t *coll_parse_idseq (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret);
static xmmsc_coll_token_t *coll_parse_posseq (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret);
static xmmsc_coll_token_t *coll_parse_operation (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret);
static xmmsc_coll_token_t *coll_parse_unaryop (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret);
static xmmsc_coll_token_t *coll_parse_binaryop (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret);
static xmmsc_coll_token_t *coll_parse_notop (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret);
static xmmsc_coll_token_t *coll_parse_andop (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret);
static xmmsc_coll_token_t *coll_parse_orop (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret);
static xmmsc_coll_token_t *coll_parse_andop_append (xmmsc_coll_token_t *tokens, xmmsc_coll_t *operator, xmmsc_coll_t **ret);
static xmmsc_coll_token_t *coll_parse_orop_append (xmmsc_coll_token_t *tokens, xmmsc_coll_t *operator, xmmsc_coll_t **ret);
static xmmsc_coll_token_t *coll_parse_reference (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret);
static xmmsc_coll_token_t *coll_parse_filter (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret);
static xmmsc_coll_token_t *coll_parse_unaryfilter (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret);
static xmmsc_coll_token_t *coll_parse_binaryfilter (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret);
static xmmsc_coll_token_t *coll_parse_autofilter (xmmsc_coll_token_t *token, xmmsc_coll_t **ret);

static xmmsc_coll_token_t *coll_token_new (xmmsc_coll_token_type_t type, char *string);
static void coll_token_free (xmmsc_coll_token_t *token);
static xmmsc_coll_token_t *coll_next_token (xmmsc_coll_token_t *token);
static void coll_append_universe (xmmsc_coll_t *coll);
static char *coll_parse_prop (xmmsc_coll_token_t *token);
static char *coll_parse_strval (xmmsc_coll_token_t *token);

static char *string_substr (char *start, char *end);
static char *string_intadd (char *number, int delta);



/**
 * @defgroup CollectionParser CollectionParser
 * @ingroup Collections
 * @brief Generate a collection structure from a string pattern.
 *
 * The grammar of the default parser is the following:
 * <pre>
 * S         := OPERATION
 * EXPR      := POSSEQ | IDSEQ | FILTER | TOKEN_GROUP_OPEN OPERATION TOKEN_GROUP_CLOSE | UNARYOP
 * PROP      := TOKEN_PROP_LONG | TOKEN_PROP_SHORT
 * INTVAL    := INTEGER | SEQUENCE
 * STRVAL    := STRING | PATTERN
 * POSSEQ    := INTVAL
 * IDSEQ     := TOKEN_SYMBOL_ID INTVAL
 * OPERATION := UNAOP | BINOP
 * UNAOP     := TOKEN_OPSET_NOT EXPR | TOKEN_REFERENCE STRING
 * BINOP     := ANDOP
 * ANDOP     := OROP ANDOP | OROP TOKEN_OPSET_AND ANDOP | OROP
 * OROP      := EXPR TOKEN_OPSET_OR OROP | EXPR
 * FILTER    := UNAFILTER | BINFILTER | STRVAL
 * UNAFILTER := TOKEN_OPFIL_HAS PROP
 * BINFILTER := PROP TOKEN_OPFIL_MATCH STRING | PROP TOKEN_OPFIL_CONTAINS STRVAL |
 *              PROP TOKEN_OPFIL_SMALLER INTEGER | PROP TOKEN_OPFIL_GREATER INTEGER |
 *              TOKEN_OPFIL_MATCH STRING | TOKEN_OPFIL_CONTAINS STRVAL
 * </pre>
 *
 * @{
 */

/**
 * Try to parse the given pattern to produce a collection structure.
 *
 * @param pattern  The string to generate a collection from.
 * @param coll  The pointer to which the collection will be saved.
 * @return TRUE if the parsing succeeded, false otherwise.
 */
int
xmmsc_coll_parse (const char *pattern, xmmsc_coll_t** coll)
{
	return xmmsc_coll_parse_custom (pattern,
	                                xmmsc_coll_default_parse_tokens,
	                                xmmsc_coll_default_parse_build,
	                                coll);
}

/**
 * Try to parse the given pattern to produce a collection structure,
 * using custom token-parsing and collection-building functions.  This
 * can be used to extend the default syntax of the parser.
 *
 * New token ids can be used, starting from
 * XMMS_COLLECTION_TOKEN_CUSTOM upwards.
 *
 * @param pattern  The string to generate a collection from.
 * @param parse_f The parsing function used to generate a list of tokens
 *                from the pattern string.
 * @param build_f The building function that produces the collection
 *                structure from the list of tokens.
 * @param coll  The pointer to which the collection will be saved.
 * @return TRUE if the parsing succeeded, false otherwise.
 */
int
xmmsc_coll_parse_custom (const char *pattern,
                         xmmsc_coll_parse_tokens_f parse_f,
                         xmmsc_coll_parse_build_f build_f,
                         xmmsc_coll_t** coll)
{
	xmmsc_coll_token_t *tokens;
	xmmsc_coll_token_t *k, *last;
	const char *next, *endstr;

	endstr = pattern + strlen (pattern);
	tokens = NULL;
	last = NULL;

	/* Tokenize the string */
	while (pattern < endstr) {
		k = parse_f (pattern, &next);
		if (k == NULL || k->type == XMMS_COLLECTION_TOKEN_INVALID) {
			/* FIXME: Check for invalid token */
		}

		if (!last)
			tokens = k;
		else
			last->next = k;

		last = k;
		pattern = next;
	}

	*coll = build_f (tokens);

	/* free tokens */
	for (k = tokens; k; k = last) {
		last = k->next;
		coll_token_free (k);
	}

	return (*coll != NULL);
}


/* FIXME:
   - syntax should be case-insensitive here and there
   - support in-string quoted parts, e.g.  hello"test here"world
   - optimize sequences (group, merge, create idlist, etc)
*/
/**
 * The default token parser.
 *
 * @param str The string to parse for a token.
 * @param newpos The position in the string after the found token.
 * @return The token found in the string.
 */
xmmsc_coll_token_t*
xmmsc_coll_default_parse_tokens (const char *str, const char **newpos)
{
	int i;
	int escape = 0;
	xmmsc_coll_token_type_t type;
	const char *tmp;
	char *strval;
	char quote;

	while (*str == ' ') str++;
	tmp = str;

	TOKEN_MATCH_CHAR ('(', XMMS_COLLECTION_TOKEN_GROUP_OPEN);
	TOKEN_MATCH_CHAR (')', XMMS_COLLECTION_TOKEN_GROUP_CLOSE);
	TOKEN_MATCH_CHAR ('#', XMMS_COLLECTION_TOKEN_SYMBOL_ID);
	TOKEN_MATCH_CHAR ('+', XMMS_COLLECTION_TOKEN_OPFIL_HAS);
	TOKEN_MATCH_CHAR (':', XMMS_COLLECTION_TOKEN_OPFIL_MATCH);
	TOKEN_MATCH_CHAR ('~', XMMS_COLLECTION_TOKEN_OPFIL_CONTAINS);
	TOKEN_MATCH_STRING ("<=", XMMS_COLLECTION_TOKEN_OPFIL_SMALLEREQ);
	TOKEN_MATCH_STRING (">=", XMMS_COLLECTION_TOKEN_OPFIL_GREATEREQ);
	TOKEN_MATCH_CHAR ('<', XMMS_COLLECTION_TOKEN_OPFIL_SMALLER);
	TOKEN_MATCH_CHAR ('>', XMMS_COLLECTION_TOKEN_OPFIL_GREATER);

	TOKEN_MATCH_STRING ("OR", XMMS_COLLECTION_TOKEN_OPSET_UNION);
	TOKEN_MATCH_STRING ("AND", XMMS_COLLECTION_TOKEN_OPSET_INTERSECTION);
	TOKEN_MATCH_STRING ("NOT", XMMS_COLLECTION_TOKEN_OPSET_COMPLEMENT);

	TOKEN_MATCH_STRING ("in:", XMMS_COLLECTION_TOKEN_REFERENCE);

	/* Starting with double-quote => STRING or PATTERN */
	if (*tmp == '"' || *tmp == '\'') {
		i = 0;
		quote = *tmp;
		type = XMMS_COLLECTION_TOKEN_STRING;

		tmp++;
		strval = x_new0 (char, strlen (tmp));

		while (escape || (*tmp != '\0' && *tmp != quote)) {
			if (!escape && (*tmp == '\\')) {
				escape = 1;
			} else {
				if (escape) {
					escape = 0;
				} else if ((*tmp == '*') || (*tmp == '?')) {
					type = XMMS_COLLECTION_TOKEN_PATTERN;
				}

				/* FIXME: Kinda dirty, and we should escape % and _ then ! */
				switch (*tmp) {
				case '*':  strval[i++] = '%';  break;
				case '?':  strval[i++] = '_';  break;
				default:   strval[i++] = *tmp; break;
				}
			}

			tmp++;
		}

		if (*tmp == quote) tmp++;

		*newpos = tmp;
		return coll_token_new (type, strval);
	}


	i = 0;
	type = XMMS_COLLECTION_TOKEN_INTEGER;
	strval = x_new0 (char, strlen (tmp));
	while (escape || (*tmp != '\0' && *tmp != ' ')) {

		/* Control input chars, escape mechanism, etc */
		if (!escape) {
			if (*tmp == '\\') {
				escape = 1;
				tmp++;
				continue;
			} else if (*tmp == ':' || *tmp == '~' ||
			           *tmp == '<' || *tmp == '>') {
				/* that was a property name, ends with a colon */
				if (tmp - str == 1)
					type = XMMS_COLLECTION_TOKEN_PROP_SHORT;
				else
					type = XMMS_COLLECTION_TOKEN_PROP_LONG;
				break;
			} else if (*tmp == '(' || *tmp == ')') {
				/* boundary char, stop parsing the string */
				break;
			}
		}

		/* Determine the type of data */
		switch (type) {
		/* matches [0-9] => INTEGER */
		case XMMS_COLLECTION_TOKEN_INTEGER:
			if (*tmp == ',' || *tmp == '-') {
				type = XMMS_COLLECTION_TOKEN_SEQUENCE;
				break;
			}

		/* matches [0-9,-] => SEQUENCE */
		case XMMS_COLLECTION_TOKEN_SEQUENCE:
			if (!isdigit(*tmp) && (*tmp != ',') && (*tmp != '-')) {
				type = XMMS_COLLECTION_TOKEN_STRING;
			}

		/* contains [*?] => PATTERN */
		case XMMS_COLLECTION_TOKEN_STRING:
			if (!escape && (*tmp == '*' || *tmp == '?')) {
				type = XMMS_COLLECTION_TOKEN_PATTERN;
			}
			break;

		/* else => STRING */
		case XMMS_COLLECTION_TOKEN_PATTERN:
			break;

		default:
			type = XMMS_COLLECTION_TOKEN_INVALID;  /* shouldn't happen */
			break;
		}

		if (escape) {
			escape = 0;
		}

		switch (*tmp) {
		case '*':  strval[i++] = '%';  break;
		case '?':  strval[i++] = '_';  break;
		default:   strval[i++] = *tmp; break;
		}

		tmp++;
	}

	*newpos = tmp;
	return coll_token_new (type, strval);
}


/**
 * Default collection structure builder.
 *
 * @param tokens The chained list of tokens.
 * @return The corresponding collection structure.
 */
xmmsc_coll_t *
xmmsc_coll_default_parse_build (xmmsc_coll_token_t *tokens)
{
	xmmsc_coll_t *coll;
	coll_parse_prepare (tokens);
	coll_parse_operation (tokens, &coll);
	return coll;
}


/* Pre-process the token list to apply contextual updates to tokens. */
static int
coll_parse_prepare (xmmsc_coll_token_t *tokens)
{
	xmmsc_coll_token_t *prev, *curr;

	prev = NULL;
	curr = tokens;

	for (prev = NULL, curr = tokens; curr; prev = curr, curr = curr->next) {
		if (prev == NULL) {
			continue;
		}

		/* Process depending on type of current token */
		switch (curr->type) {
		case XMMS_COLLECTION_TOKEN_OPFIL_GREATEREQ:
		case XMMS_COLLECTION_TOKEN_OPFIL_SMALLEREQ:
		case XMMS_COLLECTION_TOKEN_OPFIL_GREATER:
		case XMMS_COLLECTION_TOKEN_OPFIL_SMALLER:
			/* '<' and '>' preceded by a prop name, seen as string if spaced */
			if (prev->type == XMMS_COLLECTION_TOKEN_STRING) {
				if (strlen (prev->string) == 1)
					prev->type = XMMS_COLLECTION_TOKEN_PROP_SHORT;
				else
					prev->type = XMMS_COLLECTION_TOKEN_PROP_LONG;
			}
			break;

		default:
			break;
		}

		/* Process depending on type of previous token */
		switch (prev->type) {
		case XMMS_COLLECTION_TOKEN_OPFIL_HAS:
			/* _HAS is followed by a property name */
			if (curr->type == XMMS_COLLECTION_TOKEN_STRING) {
				if (strlen (curr->string) == 1)
					curr->type = XMMS_COLLECTION_TOKEN_PROP_SHORT;
				else
					curr->type = XMMS_COLLECTION_TOKEN_PROP_LONG;
			}
			break;

		/* Warning: must be placed _before_ the MATCH->CONTAINS converter! */
		case XMMS_COLLECTION_TOKEN_OPFIL_CONTAINS:
			/* Fuzzy match the operand to CONTAINS, i.e. surround with '%' */
			if (curr->type == XMMS_COLLECTION_TOKEN_STRING ||
			    curr->type == XMMS_COLLECTION_TOKEN_PATTERN) {
				int i, o;
				char *newstr = x_new0 (char, strlen (curr->string) + 3);
				i = 0;
				o = 0;

				if (curr->string[i] != '%') {
					newstr[o++] = '%';
				}
				while (curr->string[i] != '\0') {
					newstr[o++] = curr->string[i++];
				}
				if (i > 0 && curr->string[i - 1] != '%') {
					newstr[o++] = '%';
				}
				newstr[o] = '\0';

				free (curr->string);
				curr->string = newstr;
			}
			break;

		case XMMS_COLLECTION_TOKEN_OPFIL_MATCH:
			/* If MATCHing a pattern, use CONTAINS instead */
			if (curr->type == XMMS_COLLECTION_TOKEN_PATTERN) {
				prev->type = XMMS_COLLECTION_TOKEN_OPFIL_CONTAINS;
			}
			break;

		case XMMS_COLLECTION_TOKEN_OPFIL_GREATEREQ:
		case XMMS_COLLECTION_TOKEN_OPFIL_SMALLEREQ:
			/* Transform '<=', '>=' into '<', '>' */
			if (curr->type == XMMS_COLLECTION_TOKEN_INTEGER) {
				char *newstr;
				if (prev->type == XMMS_COLLECTION_TOKEN_OPFIL_GREATEREQ)
					newstr = string_intadd (curr->string, -1);
				else
					newstr = string_intadd (curr->string, 1);

				if (newstr != NULL) {
					if (prev->type == XMMS_COLLECTION_TOKEN_OPFIL_GREATEREQ)
						prev->type = XMMS_COLLECTION_TOKEN_OPFIL_GREATER;
					else
						prev->type = XMMS_COLLECTION_TOKEN_OPFIL_SMALLER;

					free (curr->string);
					curr->string = newstr;
				}
			}
			break;

		default:
			break;
		}
	}

	return 1;
}

static xmmsc_coll_token_t *
coll_parse_expr (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret)
{
	xmmsc_coll_t *coll;
	xmmsc_coll_token_t *pos;

	if (tokens == NULL) {
		*ret = NULL;
		return tokens;
	}

	PARSER_TRY (coll_parse_posseq);
	PARSER_TRY (coll_parse_idseq);
	PARSER_TRY (coll_parse_filter);
	PARSER_TRY (coll_parse_parenexpr);
	PARSER_TRY (coll_parse_unaryop);
	
	*ret = NULL;
	return tokens;
}

static xmmsc_coll_token_t *
coll_parse_parenexpr (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret)
{
	xmmsc_coll_token_t *tk;
	xmmsc_coll_t *expr;

	tk = tokens;
	TOKEN_ASSERT (tk, XMMS_COLLECTION_TOKEN_GROUP_OPEN);

	tk = coll_parse_operation (coll_next_token (tk), &expr);

	/* paren mismatch :-/ */
	if (!tk || tk->type != XMMS_COLLECTION_TOKEN_GROUP_CLOSE) {
		if (expr) {
			xmmsc_coll_unref (expr);
		}
		*ret = NULL;
		return tokens;
	}

	*ret = expr;
	return coll_next_token (tk);
}


static xmmsc_coll_token_t *
coll_parse_sequence (xmmsc_coll_token_t *tokens, const char *field,
                     xmmsc_coll_t **ret)
{
	char *start, *end, *seq, *num;
	xmmsc_coll_t *coll, *parent;
	
	if (!tokens || (tokens->type != XMMS_COLLECTION_TOKEN_INTEGER &&
	                tokens->type != XMMS_COLLECTION_TOKEN_SEQUENCE)) {
		*ret = NULL;
		return tokens;
	}

	start = tokens->string;
	end = strchr (start, ',');

	/* Take the union if several element in the sequence */
	if (end != NULL) {
		parent = xmmsc_coll_new (XMMS_COLLECTION_TYPE_UNION);
	} else {
		parent = NULL;
		end = start + strlen (start);
	}

	while (1) {
		seq = strchr (start, '-');

		/* Contains a '-', parse the sequence */
		if (seq != NULL && seq < end) {
			int len_from, len_to;
			xmmsc_coll_t *coll_from, *coll_to;
			char *buf;

			len_from = seq - start;
			len_to = end - seq - 1;

			if (len_from > 0) {
				buf = string_substr (start, seq);
				num = string_intadd (buf, -1);
				coll_from = xmmsc_coll_new (XMMS_COLLECTION_TYPE_GREATER);
				xmmsc_coll_attribute_set (coll_from, "field", field);
				xmmsc_coll_attribute_set (coll_from, "value", num);
				coll_append_universe (coll_from);
				free (buf);
				free (num);
			} else {
				coll_from = xmmsc_coll_universe ();
			}

			if (len_to > 0) {
				buf = string_substr (seq + 1, end);
				num = string_intadd (buf, 1);
				coll_to = xmmsc_coll_new (XMMS_COLLECTION_TYPE_SMALLER);
				xmmsc_coll_attribute_set (coll_to, "field", field);
				xmmsc_coll_attribute_set (coll_to, "value", num);
				xmmsc_coll_add_operand (coll_to, coll_from);
				xmmsc_coll_unref (coll_from);
				free (buf);
				free (num);
			} else {
				coll_to = coll_from;
			}

			coll = coll_to;

		/* Just an integer, match it */
		} else {
			num = string_substr (start, end);
			coll = xmmsc_coll_new (XMMS_COLLECTION_TYPE_MATCH);
			xmmsc_coll_attribute_set (coll, "field", field);
			xmmsc_coll_attribute_set (coll, "value", num);
			coll_append_universe (coll);
			free (num);
		}

		if (parent) {
			xmmsc_coll_add_operand (parent, coll);
		}

		/* Whole string parsed, exit */
		if (*end == '\0') {
			break;
		}

		start = end + 1;
		end = strchr (start, ',');
		if (end == NULL) {
			end = start + strlen (start);
		}
	}

	if (parent) {
		coll = parent;
	}

	*ret = coll;
	return coll_next_token (tokens);
}

static xmmsc_coll_token_t *
coll_parse_idseq (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret)
{
	xmmsc_coll_token_t *tk;

	tk = tokens;
	TOKEN_ASSERT (tk, XMMS_COLLECTION_TOKEN_SYMBOL_ID);

	tk = coll_next_token (tk);
	tk = coll_parse_sequence (tk, "id", ret);

	return (ret == NULL ? tokens : tk);
}

static xmmsc_coll_token_t *
coll_parse_posseq (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret)
{
	/* FIXME: link with position in (active) playlist? */
	return coll_parse_sequence (tokens, "position", ret);
}

static xmmsc_coll_token_t *
coll_parse_operation (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret)
{
	xmmsc_coll_t *coll;
	xmmsc_coll_token_t *pos;

	PARSER_TRY (coll_parse_unaryop);
	PARSER_TRY (coll_parse_binaryop);

	*ret = NULL;
	return tokens;
}

static xmmsc_coll_token_t *
coll_parse_unaryop (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret)
{
	xmmsc_coll_t *coll;
	xmmsc_coll_token_t *pos;

	PARSER_TRY (coll_parse_notop);
	PARSER_TRY (coll_parse_reference);

	*ret = coll;
	return pos;
}

static xmmsc_coll_token_t *
coll_parse_binaryop (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret)
{
	return coll_parse_andop (tokens, ret);
}

static xmmsc_coll_token_t *
coll_parse_notop (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret)
{
	xmmsc_coll_t *coll;
	xmmsc_coll_t *operand;
	xmmsc_coll_token_t *tk = tokens;

	TOKEN_ASSERT (tk, XMMS_COLLECTION_TOKEN_OPSET_COMPLEMENT);

	tk = coll_parse_expr (coll_next_token (tk), &operand);
	if (!operand) {
		*ret = NULL;
		return tokens;
	}

	coll = xmmsc_coll_new (XMMS_COLLECTION_TYPE_COMPLEMENT);
	xmmsc_coll_add_operand (coll, operand);
	xmmsc_coll_unref (operand);

	*ret = coll;
	return tk;
}

static xmmsc_coll_token_t *
coll_parse_andop (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret)
{
	return coll_parse_andop_append (tokens, NULL, ret);
}

static xmmsc_coll_token_t *
coll_parse_orop (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret)
{
	return coll_parse_orop_append (tokens, NULL, ret);
}

static xmmsc_coll_token_t *
coll_parse_andop_append (xmmsc_coll_token_t *tokens, xmmsc_coll_t *operator,
                         xmmsc_coll_t **ret)
{
	xmmsc_coll_t *first, *tmp;
	xmmsc_coll_token_t *tk;

	tk = coll_parse_orop (tokens, &first);
	if (!first) {
		*ret = NULL;
		return tokens;
	}

	/* skip the AND operator if present */
	if (tk && tk->type == XMMS_COLLECTION_TOKEN_OPSET_INTERSECTION) {
		tk = coll_next_token (tk);
	}


	if (!operator) {
		operator = xmmsc_coll_new (XMMS_COLLECTION_TYPE_INTERSECTION);
		xmmsc_coll_add_operand (operator, first);
		tk = coll_parse_andop_append (tk, operator, &tmp);

		/* actually, only one operand, bypass the 'AND' altogether */
		if (tmp == NULL) {
			xmmsc_coll_remove_operand (operator, first);
			xmmsc_coll_unref (operator);
			*ret = first;
		}
		else {
			*ret = operator;
		}
	}
	else {
		xmmsc_coll_add_operand (operator, first);
		tk = coll_parse_andop_append (tk, operator, ret);
		*ret = operator;
	}

	/* tk = coll_parse_andop_append (tk, operator, ret); */
	return tk;
}

static xmmsc_coll_token_t *
coll_parse_orop_append (xmmsc_coll_token_t *tokens, xmmsc_coll_t *operator,
                        xmmsc_coll_t **ret)
{
	xmmsc_coll_t *first;
	xmmsc_coll_token_t *tk;

	tk = coll_parse_expr (tokens, &first);
	if (!first) {
		*ret = NULL;
		return tokens;
	}

	if (tk && tk->type == XMMS_COLLECTION_TOKEN_OPSET_UNION) {
		if (!operator) {
			operator = xmmsc_coll_new (XMMS_COLLECTION_TYPE_UNION);
		}
	}

	if (operator) {
		xmmsc_coll_add_operand (operator, first);

		if (tk && tk->type == XMMS_COLLECTION_TOKEN_OPSET_UNION) {
			tk = coll_parse_orop_append (coll_next_token (tk), operator, ret);
		}

		*ret = operator;
	}
	else {
		*ret = first;
	}

	return tk;
}

static xmmsc_coll_token_t *
coll_parse_reference (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret)
{
	xmmsc_coll_t *coll;
	char *namespace, *reference, *slash;
	xmmsc_coll_token_t *tk = tokens;

	TOKEN_ASSERT (tk, XMMS_COLLECTION_TOKEN_REFERENCE);

	tk = coll_next_token (tk);

	TOKEN_ASSERT (tk, XMMS_COLLECTION_TOKEN_STRING);

	slash = strchr (tk->string, '/');
	if (slash != NULL && slash > tk->string) {
		namespace = string_substr (tk->string, slash);
	}
	else {
		namespace = strdup (XMMS_COLLECTION_PARSER_DEFAULT_NAMESPACE);
	}

	if (slash == NULL) {
		reference = tk->string;
	}
	else {
		reference = slash + 1;
	}

	coll = xmmsc_coll_new (XMMS_COLLECTION_TYPE_REFERENCE);
	xmmsc_coll_attribute_set (coll, "namespace", namespace);
	xmmsc_coll_attribute_set (coll, "reference", reference);

	free (namespace);

	*ret = coll;
	return coll_next_token (tk);
}


static xmmsc_coll_token_t *
coll_parse_filter (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret)
{
	xmmsc_coll_t *coll;
	xmmsc_coll_token_t *pos;

	PARSER_TRY (coll_parse_unaryfilter);
	PARSER_TRY (coll_parse_binaryfilter);
	PARSER_TRY (coll_parse_autofilter);

	*ret = NULL;
	return tokens;
}

static xmmsc_coll_token_t *
coll_parse_unaryfilter (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret)
{
	xmmsc_coll_t *coll;
	char *prop;
	xmmsc_coll_token_t *tk = tokens;

	TOKEN_ASSERT (tk, XMMS_COLLECTION_TOKEN_OPFIL_HAS);

	tk = coll_next_token (tk);
	prop = coll_parse_prop (tk);
	if (!prop) {
		*ret = NULL;
		return tokens;
	}

	coll = xmmsc_coll_new (XMMS_COLLECTION_TYPE_HAS);
	xmmsc_coll_attribute_set (coll, "field", prop);
	coll_append_universe (coll);

	free (prop);

	*ret = coll;
	return coll_next_token (tk);
}

static xmmsc_coll_token_t *
coll_parse_binaryfilter (xmmsc_coll_token_t *tokens, xmmsc_coll_t **ret)
{
	char *prop, *strval;
	xmmsc_coll_t *coll = NULL;
	xmmsc_coll_token_t *operand;
	xmmsc_coll_type_t colltype;
	xmmsc_coll_token_t *tk = tokens;

	if (!tk) {
		*ret = NULL;
		return tokens;
	}

	prop = coll_parse_prop (tk);
	if (!prop) {
		return NULL;
	}

	tk = coll_next_token (tk);
	operand = coll_next_token (tk);
	if (tk && operand) {
		strval = NULL;

		switch (tk->type) {
		case XMMS_COLLECTION_TOKEN_OPFIL_MATCH:
			colltype = XMMS_COLLECTION_TYPE_MATCH;
			if (operand->type == XMMS_COLLECTION_TOKEN_STRING) {
				strval = operand->string;
			}
			break;

		case XMMS_COLLECTION_TOKEN_OPFIL_CONTAINS:
			colltype = XMMS_COLLECTION_TYPE_CONTAINS;
			strval = coll_parse_strval (operand);
			break;

		case XMMS_COLLECTION_TOKEN_OPFIL_SMALLER:
			colltype = XMMS_COLLECTION_TYPE_SMALLER;
			if (operand->type == XMMS_COLLECTION_TOKEN_INTEGER) {
				strval = operand->string;
			}
			break;

		case XMMS_COLLECTION_TOKEN_OPFIL_GREATER:
			colltype = XMMS_COLLECTION_TYPE_GREATER;
			if (operand->type == XMMS_COLLECTION_TOKEN_INTEGER) {
				strval = operand->string;
			}
			break;

		default:
			break;
		}

		if (strval) {
			coll = xmmsc_coll_new (colltype);
			xmmsc_coll_attribute_set (coll, "field", prop);
			xmmsc_coll_attribute_set (coll, "value", strval);
			coll_append_universe (coll);
		}
	}

	free (prop);

	*ret = coll;
	return coll_next_token (operand);
}

static xmmsc_coll_token_t *
coll_parse_autofilter (xmmsc_coll_token_t *token, xmmsc_coll_t **ret)
{
	char *strval;
	xmmsc_coll_type_t colltype;
	xmmsc_coll_t *coll, *operand;
	int i;

	if (token->type == XMMS_COLLECTION_TOKEN_OPFIL_MATCH) {
		colltype = XMMS_COLLECTION_TYPE_MATCH;
		token = coll_next_token (token);
	} else if (token->type == XMMS_COLLECTION_TOKEN_OPFIL_CONTAINS) {
		colltype = XMMS_COLLECTION_TYPE_CONTAINS;
		token = coll_next_token (token);
	} else {
		colltype = XMMS_COLLECTION_TYPE_ERROR;
	}

	strval = coll_parse_strval (token);
	if (!strval) {
		*ret = NULL;
		return token;
	}

	/* No operator at all, guess from argument type */
	if (colltype == XMMS_COLLECTION_TYPE_ERROR) {
		if (token->type == XMMS_COLLECTION_TOKEN_PATTERN)
			colltype = XMMS_COLLECTION_TYPE_CONTAINS;
		else
			colltype = XMMS_COLLECTION_TYPE_MATCH;
	}

	coll = xmmsc_coll_new (XMMS_COLLECTION_TYPE_UNION);

	for (i = 0; coll_autofilter[i] != NULL; i++) {
		operand = xmmsc_coll_new (colltype);
		xmmsc_coll_attribute_set (operand, "field", coll_autofilter[i]);
		xmmsc_coll_attribute_set (operand, "value", strval);
		xmmsc_coll_add_operand (coll, operand);
		coll_append_universe (operand);
		xmmsc_coll_unref (operand);
	}

	*ret = coll;
	return coll_next_token (token);
}


static xmmsc_coll_token_t *
coll_token_new (xmmsc_coll_token_type_t type, char *string)
{
	xmmsc_coll_token_t* token;

	token = x_new0 (xmmsc_coll_token_t, 1);
	token->type = type;
	token->string = string;

	return token;
}

static void
coll_token_free (xmmsc_coll_token_t *token)
{
	if (token->string != NULL) {
		free (token->string);
	}

	free (token);
}

static xmmsc_coll_token_t *
coll_next_token (xmmsc_coll_token_t *token)
{
	return (token ? token->next : NULL);
}

static void
coll_append_universe (xmmsc_coll_t *coll)
{
	xmmsc_coll_t *univ;

	univ = xmmsc_coll_universe ();
	xmmsc_coll_add_operand (coll, univ);
	xmmsc_coll_unref (univ);
}

static char *
coll_parse_prop (xmmsc_coll_token_t *token)
{
	int i;

	if (!token || !token->string) {
		return NULL;
	}

	switch (token->type) {
	case XMMS_COLLECTION_TOKEN_PROP_LONG:
		return strdup (token->string);

	case XMMS_COLLECTION_TOKEN_PROP_SHORT:
		for (i = 0; xmmsc_coll_prop_short[i].longstr; i++) {
			if (*token->string == xmmsc_coll_prop_short[i].shortstr) {
				return strdup (xmmsc_coll_prop_short[i].longstr);
			}
		}
		break;

	default:
		break;
	}

	return NULL;
}

static char *
coll_parse_strval (xmmsc_coll_token_t *token)
{
	if (!token || (token->type != XMMS_COLLECTION_TOKEN_STRING &&
	               token->type != XMMS_COLLECTION_TOKEN_PATTERN)) {
		return NULL;
	}

	return token->string;
}

/* Create a new string from a substring of an existing string, between
 * start and end.
 */
static char *
string_substr (char *start, char *end)
{
	int len;
	char *buf;

	len = end - start;
	buf = x_new0 (char, len + 1);
	strncpy (buf, start, len);
	buf[len] = '\0';

	return buf;
}

/* Given a string containing a number, add the given delta to that
 * number and produce a new string with the result. The returned
 * string must be freed afterwards.
 */
static char *
string_intadd (char *number, int delta)
{
	int n, len, size;
	char *endptr;
	char *buf;

	n = strtol (number, &endptr, 10);

	/* Invalid integer string */
	if (*endptr != '\0') {
		return NULL;
	}

	n += delta;
	len = strlen (number) + 1;
	buf = x_new0 (char, size + 1);
	snprintf (buf, len, "%d", n);

	return buf;
}

/** @} */