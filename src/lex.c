#include "clint.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>


#define LINES_INIT_LEN 100

static file_t *fp;
static char *ch;
static int max_lines;

static bool parsing_header_name;
static bool parsing_pp_directive;


/* Include code generated by gperf.
 */

struct gperf_s {
  const char *word;
  enum token_e code;
};

static const struct gperf_s *kw_check(const char *str, unsigned len);
static const struct gperf_s *pp_check(const char *str, unsigned len);

#include "kw.c.part"
#include "pp.c.part"


static enum token_e find_kw(const char *word, int len) {
  assert(word);
  assert(len > 0);

  const struct gperf_s *res = kw_check(word, len);
  return res ? res->code : TOK_UNKNOWN;
}


static enum token_e find_pp(const char *word, int len) {
  assert(word);
  assert(len > 0);

  const struct gperf_s *res = pp_check(word, len);
  return res ? res->code : TOK_UNKNOWN;
}


static inline const char *stringify_kind(enum token_e kind) {
  static const char *words[] = {
#define XX(kind, word) word,
    TOK_MAP(XX)
#undef XX
  };

  return words[kind];
}


void lex_init(file_t *file) {
  assert(file);
  assert(file->data);

  fp = file;
  ch = fp->data;

  fp->nlines = 1;
  max_lines = LINES_INIT_LEN;
  fp->lines = xmalloc((max_lines+1) * sizeof(char *));
  fp->lines[1] = fp->data;

  parsing_header_name = false;
  parsing_pp_directive = false;
}


static inline void newline(void) {
  assert(*ch == '\n');

  if (fp->nlines == max_lines) {
    max_lines *= 2;
    fp->lines = xrealloc(fp->lines, (max_lines+1) * sizeof(*fp->lines));
  }

  ++ch;
  fp->lines[++fp->nlines] = ch;
}


static inline void skip_spaces(void) {
  while (isspace(*ch))
    if (*ch == '\n')
      newline();
    else
      ++ch;
}


/* C99 6.4.4.1: integer constant.
 * C99 6.4.4.2: floating constant.
 *
 *   decimal integer: [1-9]{D}*{IS}?           (1)
 *   octal integer: 0[0-7]*{IS}?               (2)
 *   hexa integer: 0[xX]{H}+{IS}?              (3)
 *   decimal floating: {D}+{E}{FS}?            (4)
 *                     {D}*\.{D}+{E}?{FS}?     (5)
 *                     {D}+\.{D}*{E}?{FS}?     (6)
 *   hexa floating: 0[xX]{H}+{P}{FS}?          (7)
 *                  0[xX]{H}*\.{H}+{P}?{FS}?   (8)
 *                  0[xX]{H}+\.{H}*{P}?{FS}?   (9)
 * , where
 *   D: [0-9]
 *   H: [a-fA-F0-9]
 *   IS: ([uU]|[uU]?(l|L|ll|LL)|(l|L|ll|LL)[uU])
 *   FS: [fFlL]
 *   P: ([Pp][+-]?{D}+)
 *   E: ([Ee][+-]?{D}+)
 */
static bool numeric_const(token_t *token) {
  assert(token);

  if (!(isdigit(*ch) || *ch == '.'))
    return false;

  while (isxdigit(*ch)) ++ch;
  if (tolower(*ch) == 'x') ++ch;
  while (isxdigit(*ch)) ++ch;
  if (*ch == '.') ++ch;
  while (isxdigit(*ch)) ++ch;
  if (tolower(*ch) == 'e' || tolower(*ch) == 'p') {
    ++ch;
    if (*ch == '+' || *ch == '-') ++ch;
    while (isdigit(*ch)) ++ch;
  }

  while (isalpha(*ch)) ++ch;

  token->kind = TOK_NUM_CONST;
  return true;
}


/* C99 6.4.4.4: character constant.
 */
static bool char_const(token_t *token) {
  assert(token);

  if (!(*ch == '\'' ||  *ch == 'L' && ch[1] == '\''))
    return false;

  ch += *ch == 'L' ? 2 : 1;
  while (!(*ch == '\'' && ch[-1] != '\\')) ++ch;
  ++ch;

  token->kind = TOK_CHAR_CONST;
  return true;
}


/* C99 6.4.5: string literal.
 */
static bool string_literal(token_t *token) {
  assert(token);

  if(!(*ch == '"' ||  *ch == 'L' && ch[1] == '"'))
    return false;

  ch += *ch == 'L' ? 2 : 1;
  while (!(*ch == '"' && ch[-1] != '\\')) ++ch;
  ++ch;

  token->kind = TOK_STRING;
  return true;
}


/* C99 6.4.3: universal character name.
 */
static bool check_ucn(void) {
  if (!(*ch == '\\' && tolower(ch[1]) == 'u'))
    return false;

  int digits = ch[1] == 'u' ? 4 : 8;
  for (int i = 0; i < digits; ++i)
    if (!isxdigit(ch[i+2]))
      return false;

  return true;
}


/* C99 6.4.1: keyword.
 * C99 6.4.2: identifier.
 *
 *   identifier: ([_a-zA-Z]|{U})(\w|{U})*
 * , where
 *   H: [a-fA-F0-9]{4}
 *   U: (\\u{H})|(\\U{H}{H})
 */
static bool identifier(token_t *token) {
  assert(token);

  if (!(isalpha(*ch) || *ch == '_' || check_ucn()))
    return false;

  char *start = ch;

  do {
    ch += ch[1] == 'u' ? 6
        : ch[1] == 'U' ? 10
                       : 1;
  } while (isalnum(*ch) || *ch == '_' || check_ucn());

  if (parsing_pp_directive) {
    token->kind = find_pp(start, ch-start);

    if (token->kind == PP_INCLUDE)
      parsing_header_name = true;

    parsing_pp_directive = false;
  } else {
    token->kind = find_kw(start, ch-start);
    if (token->kind == TOK_UNKNOWN)
      token->kind = TOK_IDENTIFIER;
  }

  return true;
}


/* C99 6.4.6: punctuator.
 */
static bool punctuator(token_t *token) {
  //#TODO: support for digraphs and trigraphs.

  enum token_e kind;

  switch (*ch) {
    case '[': kind = PN_LSQUARE; break;
    case ']': kind = PN_RSQUARE; break;
    case '(': kind = PN_LPAREN; break;
    case ')': kind = PN_RPAREN; break;
    case '{': kind = PN_LBRACE; break;
    case '}': kind = PN_RBRACE; break;
    case '~': kind = PN_TILDE; break;
    case '?': kind = PN_QUESTION; break;
    case ':': kind = PN_COLON; break;
    case ';': kind = PN_SEMI; break;
    case ',': kind = PN_COMMA; break;

    case '!': kind = ch[1] == '=' ? PN_EXCLAIM : PN_EXCLAIMEQ; break;
    case '/': kind = ch[1] == '=' ? PN_SLASHEQ : PN_SLASH; break;
    case '%': kind = ch[1] == '=' ? PN_PERCENTEQ : PN_PERCENT; break;
    case '^': kind = ch[1] == '=' ? PN_CARETEQ : PN_CARET; break;
    case '=': kind = ch[1] == '=' ? PN_EQEQ : PN_EQ; break;
    case '#': kind = ch[1] == '#' ? PN_HASHHASH : PN_HASH; break;

    case '.':
      kind = ch[1] == '.' && ch[2] == '.' ? PN_ELLIPSIS : PN_PERIOD;
      break;

    case '&':
      kind = ch[1] == '&' ? PN_AMPAMP
           : ch[1] == '=' ? PN_AMPEQ
                          : PN_AMP;
      break;

    case '*':
      kind = ch[1] == '=' ? PN_STAREQ
                          : PN_STAR;
      break;

    case '+':
      kind = ch[1] == '+' ? PN_PLUSPLUS
           : ch[1] == '=' ? PN_PLUSEQ
                          : PN_PLUS;
      break;

    case '-':
      kind = ch[1] == '>' ? PN_ARROW
           : ch[1] == '-' ? PN_MINUSMINUS
           : ch[1] == '=' ? PN_MINUSEQ
                          : PN_MINUS;
      break;

    case '<':
      kind = ch[1] == '<' && ch[2] == '=' ? PN_LELEEQ
           : ch[1] == '<' ? PN_LELE
           : ch[1] == '=' ? PN_LEEQ
                          : PN_LE;
      break;

    case '>':
      kind = ch[1] == '>' && ch[2] == '=' ? PN_GTGTEQ
           : ch[1] == '>' ? PN_GTGT
           : ch[1] == '=' ? PN_GTEQ
                          : PN_GT;
      break;

    case '|':
      kind = ch[1] == '|' ? PN_PIPEPIPE
           : ch[1] == '=' ? PN_PIPEEQ
                          : PN_PIPE;
      break;

    default:
      assert(0);
  }

  ch += strlen(stringify_kind(kind));
  token->kind = kind;
  return true;
}


static bool comment(token_t *token) {
  assert(*ch == '/');
  ++ch;
  assert(*ch == '*' || *ch == '/');

  if (*ch == '*') {
    if (!(*++ch && *++ch)) return false;

    while (*ch && !(ch[-1] == '*' && *ch == '/')) {
      if (*ch == '\n') newline();
      ++ch;
    }
  } else {
    while (*ch && *ch != '\n') ++ch;
    if (*ch == '\n') newline();
  }

  ++ch;
  token->kind = TOK_COMMENT;
  return true;
}


/* C99 6.4.7: header name.
 */
static bool header_name(token_t *token) {
  assert(token);

  if (!(*ch == '<' || *ch == '"'))
    return false;

  int expected = *ch == '<' ? '>' : '"';
  while (*++ch != expected);

  ++ch;
  return true;
}


bool lex_pull(token_t *token) {
  assert(token);

  bool success = false;

  skip_spaces();

  token->start.line = fp->nlines;
  token->start.column = ch - fp->lines[fp->nlines] + 1;

  switch (*ch) {
    // EOB.
    case 0:
      break;

    // Numbers.
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      success = numeric_const(token);
      break;

    // Wide character constant, string literal or identifier.
    case 'L':
      success = ch[1] == '\'' ? char_const(token)
              : ch[1] == '"'  ? string_literal(token)
                              : identifier(token);
      break;

    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': /* 'L' */ case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
    case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
    case 'v': case 'w': case 'x': case 'y': case 'z':
    case '_':
      success = identifier(token);
      break;

    // Character consant.
    case '\'':
      success = char_const(token);
      break;

    // String literal or header name.
    case '"':
      if (parsing_header_name) {
        parsing_header_name = false;
        success = header_name(token);
      } else {
        success = string_literal(token);
      }

      break;

    // Numeric constant or punctuator.
    case '.':
      success = (isdigit(ch[1]) ? numeric_const : punctuator)(token);
      break;

    case '[': case ']': case '(': case ')': case '{': case '}': case '&':
    case '*': case '+': case '-': case '~': case '!': case '%': case '>':
    case '=': case '^': case '|': case '?': case ':': case ';': case ',':
      success = punctuator(token);
      break;

    // Header name or punctuator.
    case '<':
      if (parsing_header_name) {
        parsing_header_name = false;
        success = header_name(token);
      } else {
        success = punctuator(token);
      }

      break;

    // Comment or punctuator.
    case '/':
      if (ch[1] == '/' || ch[1] == '*')
        success = comment(token);
      else
        success = punctuator(token);

      break;

    case '#':
      parsing_pp_directive = true;
      success = punctuator(token);
      break;

    // C99 C99 6.4.3: universal character name.
    case '\\':
      if (check_ucn())
        success = identifier(token);

      //#TODO: error handling
  }

  if (success) {
    token->end.line = fp->nlines;
    token->end.column = ch - fp->lines[fp->nlines];
  }

  return success;
}


const char *lex_to_str(token_t *token) {
  assert(token);
  return stringify_kind(token->kind);
}
