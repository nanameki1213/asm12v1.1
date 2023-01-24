#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#define PRINT(x) fprintf(stderr, "%s\n", (x));
#define PUSH "stre @0x201\n load @0x200\n sub 0x1\n stre @0x200\n lda acr \nload @0x201\n stre [amr]\n"
#define POP "load @0x0\n lda acr\n load [amr]\n stre @0x1\n load @0x0\n add 0x1\n stre @0x0\n load @0x1\n stre "
#define CALL "load pc\n add 0x4\n stre @0x205\n jump"

#define PUSH_ADDR 0x0
#define POP_ADDR 0x6

// トークンの種類
typedef enum {
  TK_RESERVED, // 記号
  TK_ISTR,     // 命令記号
  TK_IDENT,    // 識別子
  TK_HEX,      // 16進数トークン
  TK_BIN,      // 2進数トークン
  TK_NEWLINE,  // 改行文字
  TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
  TokenKind kind; // トークンの型
  Token *next;    // 次の入力トークン
  int val;        // kindがTK_NUMの場合、その数値
  char *str;      // トークン文字列
  int len;        // トークンの長さ
};

// 現在着目しているトークン
Token *token;

typedef struct Label Label;

struct Label {
  Label *next;
  char *name;
  int len;
  int addr;
};

Label *label;

int start_addr;

char *user_input;

int size;

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);

char *cut_str(char *str, int len);
void print_bit(int val, int num);

bool is_alnum(char c);
bool consume(char *op, TokenKind kind, Token **tok);
void expect(char *op);
int expect_number();
bool at_eof();

Label *find_label(char *name, int len);
Label *new_label(Label *cur, char *name, int len, int addr);
Label *labeling();

Token *new_token(TokenKind kind, Token *cur, char *str, int len);
bool startswith(char *p, char *q);
Token *getcurrent(Token *cur);
Token *tokenize(char *p);

void gen(Token *tok);

char *read_file(char *path);