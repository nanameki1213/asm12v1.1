#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#define PRINT(x) fprintf(stderr, "%s\n", (x));
#define PUSH "stre @0x1\n load @0x0\n sub 0x1\n lda acr \nload @0x1\n stre [amr]\n"
#define POP "load @0x0\n lda acr\n load [amr]\n stre @0x1\n load @0x0\n add 0x1\n stre @0x0\n load @0x1\n"
#define CALL "load pc\n stre @0x5\n load @0x1\n jump\n"

// トークンの種類
typedef enum {
  TK_RESERVED, // 記号
  TK_ISTR,     // 命令記号
  TK_IDENT,    // 識別子
  TK_HEX,      // 16進数トークン
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

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// 入力プログラム
char *user_input;

// エラー箇所を報告する
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, " "); // pos個の空白を出力
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

char *cut_str(char *str, int len) {
  char *p = calloc(len, sizeof(char));
  memcpy(p, str, len);
  return p;
}

bool is_alnum(char c){
  return ('a'<=c && c<='z') ||
         ('A'<=c && c<='Z') ||
         ('0'<=c && c<='9') ||
         (c=='_');
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op, TokenKind kind, Token **tok) {
  if ((*tok)->kind != kind ||
      strlen(op) != (*tok)->len ||
      memcmp((*tok)->str, op, (*tok)->len))
    return false;
  *tok = (*tok)->next;
  return true;
}

bool cmp_ident(char *id, Token *tok) {
  if(tok->kind != TK_IDENT ||
     strlen(id) != tok->len ||
     memcmp(tok->str, id, tok->len))
    return false;
  return true;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
  if (token->kind != TK_RESERVED ||
      strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    error("'%c'ではありません", op);
  token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
  if (token->kind != TK_HEX)
    error("数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

Label *find_label(char *name, int len) {
  for(Label *lab = label; lab; lab = lab->next) {
    // fprintf(stderr, "in find_label, lab->len: %d\n", lab->len);
    if(lab->len == len && memcmp(lab->name, name, lab->len) == 0) {
      // PRINT("return lab")
      return lab;
    }
  }
  return NULL;
}

Label *new_label(Label *cur, char *name, int len, int addr) {
  Label *lab = calloc(1, sizeof(Label));
  lab->name = name;
  lab->len = len;
  lab->addr = addr;
  cur->next = lab;
  return lab;
}

Label *labeling() {
  Token *tok = token;
  Label head;
  Label *cur = &head;
  cur->next = NULL;
  int cnt = 0;
  while(tok) {
    if(tok->kind == TK_IDENT) {
      Token *tmp = tok;
      tok = tok->next;
      if(consume(":", TK_RESERVED, &tok)) {
        cur = new_label(cur, tmp->str, tmp->len, start_addr + cnt);
      }
      continue;
    }

    if(consume(":", TK_RESERVED, &tok)) {
      error("ラベルに設定できない識別子です\n");
    }
    
    if(tok->kind == TK_ISTR) {
      cnt++;
    }

    tok = tok->next;
  }
  return head.next;
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

bool startswith(char *p, char *q) {
  return memcmp(p, q, strlen(q)) == 0;
}

// curのリンクリストの先頭アドレスを返す
Token *getcurrent(Token *cur) {
  while(cur->next)
    cur = cur->next;
  return cur;
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    // PRINT("in while")
    // 空白文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }

    if(startswith(p, "#")) {
      while(*p != '\n')
        p++;
      continue;
    }

    if (strchr(":@[],.", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }
// cmp set命令を追加する (命令が16個のみでは足りない)
    if(startswith(p, "load") | startswith(p, "stre") |
       startswith(p, "jpmi") | startswith(p, "jmpz") |
       startswith(p, "jump"))
    {
      cur = new_token(TK_ISTR, cur, p, 4);
      p += 4;
      continue;
    }

    if(startswith(p, "add") | startswith(p, "sub") |
       startswith(p, "lda"))
    {
      cur = new_token(TK_ISTR, cur, p, 3);
      p += 3;
      continue;
    }

    if(startswith(p, "push")) {
      Token *push = tokenize(PUSH);
      cur->next = push;
      cur = getcurrent(push);
      p += 4;
      continue;
    }

    if(startswith(p, "pop")) {
      Token *pop = tokenize(POP);
      cur->next = pop;
      cur = getcurrent(pop);
      p += 3;
      continue;
    }

    if (startswith(p, "0x")) {
      cur = new_token(TK_HEX, cur, p, 0);
      char *q = p;
      cur->val = strtol(p, &p, 16);
      cur->len = p - q;
      continue;
    }

    if('a'<=*p && *p<='z') {
        char *q = p;
        while(is_alnum(*q)) q++;
        cur = new_token(TK_IDENT, cur, p, q - p);
        p = q;
        continue;
    }

    error("トークナイズできません");
  }
  return head.next;
}

void print_bit(int val, int num) {
  int i;
  for(i = 0; i < num; i++) {
    printf("%d", val & (1<<(num-1)) ? 1 : 0);
    val <<= 1;
  }
}

void gen(Token *tok) {
  char *istr = calloc(16, sizeof(char));
  while(tok) {
    if(tok->kind == TK_ISTR) {
      // fprintf(stderr, "in gen, tok->str: %s\n", cut_str(tok->str, tok->len));
      memcpy(istr, tok->str, tok->len);
      if(consume("load", TK_ISTR, &tok))
        printf("0000");
      else if(consume("stre", TK_ISTR, &tok))
        printf("0001");
      else if(consume("add", TK_ISTR, &tok))
        printf("0010");
      else if(consume("sub", TK_ISTR, &tok))
        printf("0011");
      else if(consume("jump", TK_ISTR, &tok))
        printf("0100");
      else if(consume("jpmi", TK_ISTR, &tok))
        printf("0101");
      else if(consume("jmpz", TK_ISTR, &tok))
        printf("0110");
      else if(consume("lda", TK_ISTR, &tok))
        printf("0111");

      // アドレッシングモードの指定
      if(consume("@", TK_RESERVED, &tok)) { // 直接アドレス
        printf("10");
      }
      else if(consume("[", TK_RESERVED, &tok)) { // 間接アドレス
        printf("010000000000\n");
        consume("amr", TK_IDENT, &tok);
        consume("]", TK_RESERVED, &tok);
        continue;
      } else if(tok->kind == TK_HEX) { // 即値
        printf("00");
      } else if(consume("acr", TK_IDENT, &tok)) { // ACR
        printf("110000000000\n");
        continue;
      } else if(consume("pc", TK_IDENT, &tok)) { // PC
        printf("111000000000\n");
        continue;
      } else if(tok->kind == TK_IDENT) { // ラベルによる直接アドレス
        printf("10");
      }

      if(consume(",", TK_RESERVED, &tok))
        tok = tok->next;

      // オペランドの指定
      if(tok->kind == TK_HEX)
        print_bit(tok->val, 10);
      else if(tok->kind == TK_IDENT) {
        // fprintf(stderr, "p: %s\n", cut_str(tok->str, tok->len));
        Label *lab = find_label(tok->str, tok->len);
        if(lab != NULL)
          print_bit(lab->addr, 10);
        else
          error("ラベルが見つかりません\n");
      } else
        printf("0000000000");
      
      putchar('\n');
    }
    tok = tok->next;
  }
}

char *read_file(char *path) {
  //ファイルを開く
  FILE *fp = fopen(path, "r");
  if(!fp)
    error("cannot open %s: %s", path, strerror(errno));
  
  //ファイルの長さを調べる
  if(fseek(fp, 0, SEEK_END) == -1)
    error("%s: fseek: %s", path, strerror(errno));
  size_t size = ftell(fp);
  if(fseek(fp, 0, SEEK_SET) == -1)
    error("%s: fseek: %s", path, strerror(errno));
  
  //ファイル内容を読み込む
  char *buf = calloc(1, size + 2);
  fread(buf, size, 1, fp);

  //ファイルが必ず"\n\0"で終わっているようにする
  if(size == 0 || buf[size - 1] != '\n')
    buf[size++] = '\n';
  buf[size] = '\0';
  fclose(fp);
  return buf;
}

int main(int argc, char **argv) {

  if(argc != 3) {
    fprintf(stderr, "invalid argments.\n");
    return -1;
  }

  start_addr = strtol(argv[2], &argv[2], 16);
  printf("start address: ");
  print_bit(start_addr, 10);
  putchar('\n');

  char *filename = argv[1];
  user_input = read_file(filename);
  
  // トークナイズしてパースする
  token = tokenize(user_input);

  // トークンの末尾にEOFを追加
  Token *eof = getcurrent(token);
  eof->next = calloc(1, sizeof(Token));
  eof->next->kind = TK_EOF;

  // ラベルの定義
  label = labeling();

  // for(Label *lab = label; lab; lab = lab->next) {
  //   fprintf(stderr, "lab->name: %s\n", lab->name);
  //   fprintf(stderr, "lab->len: %d\n", lab->len);
  //   fprintf(stderr, "lab->addr: 0x");
  //   print_bit(lab->addr, 10);
  //   putchar('\n');
  // }

  Token *tok = token;
  for(; tok; tok = tok->next) {
    printf("tok->kind: %d\n", tok->kind);
    printf("tok->val: %d\n", tok->val);
    printf("tok->len: %d\n", tok->len);
  }

  printf("コード生成スタート\n\n");

  gen(token);

  return 0;
}