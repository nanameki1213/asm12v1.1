#include "defines.h"


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

  cur = new_label(cur, "push", 4, PUSH_ADDR);
  cur = new_label(cur, "pop", 3, POP_ADDR);

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
    if (*p == ' ' || *p == '\r' || *p == '\f' ||
        *p == '\t' || *p == '\v') {
      p++;
      continue;
    }

    if(*p == '\n') {
      cur = new_token(TK_NEWLINE, cur, "\n", 1);
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
    if((startswith(p, "load") | startswith(p, "stre") |
       startswith(p, "jpmi") | startswith(p, "jmpz") |
       startswith(p, "jump")) && !is_alnum(p[4]))
    {
      cur = new_token(TK_ISTR, cur, p, 4);
      p += 4;
      continue;
    }

    if((startswith(p, "add") | startswith(p, "sub") |
       startswith(p, "lda")) && !is_alnum(p[3]))
    {
      cur = new_token(TK_ISTR, cur, p, 3);
      p += 3;
      continue;
    }

    if(startswith(p, "push") && !is_alnum(p[4])) {
      p += 5;
      Token *opd = tokenize(p);
      cur = new_token(TK_ISTR, cur, "load", 4);
      for(; opd->kind != TK_NEWLINE; opd = opd->next) {
        cur = new_token(opd->kind, cur, opd->str, opd->len);
        if(opd->kind == TK_HEX)
          cur->val = opd->val;
        p += opd->len;
      }
      free(opd);
      cur = new_token(TK_NEWLINE, cur, "\n", 1);
      p++;
      Token *call = tokenize(CALL);
      cur->next = call;
      cur = getcurrent(call);
      cur = new_token(TK_IDENT, cur, "push", 4);
      cur = new_token(TK_NEWLINE, cur, "\n", 1);
      continue;
    }

    if(startswith(p, "pop") && !isalnum(p[3])) {
      Token *call = tokenize(CALL);
      cur->next = call;
      cur = getcurrent(call);
      cur = new_token(TK_IDENT, cur, "pop", 3);
      cur = new_token(TK_NEWLINE, cur, "\n", 1);
      cur = new_token(TK_ISTR, cur, "stre", 4);
      p += 4;
      Token *opd = tokenize(p);
      for(; opd->kind != TK_NEWLINE; opd = opd->next) {
        cur = new_token(opd->kind, cur, opd->str, opd->len);
        if(opd->kind == TK_HEX)
          cur->val = opd->val;
        p += opd->len;
      }
      free(opd);
      continue;
      // Token *pop = tokenize(POP);
      // cur->next = pop;
      // cur = getcurrent(pop);
      // p += 3;
      // continue;
    }

    if(startswith(p, "call") && !is_alnum(p[4])) {
      Token *call = tokenize(CALL);
      cur->next = call;
      cur = getcurrent(call);
      p += 4;
      continue;
    }

    if(startswith(p, "ret") && !isalnum(p[3])) {
      cur = new_token(TK_ISTR, cur, "jump", 4);
      cur = new_token(TK_RESERVED, cur, "@", 1);
      cur = new_token(TK_HEX, cur, "0x5", 3);
      cur->val = 5;
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

    if(startswith(p, "0b")) {
      cur = new_token(TK_BIN, cur, p, 0);
      p += 2;
      char *q = p;
      cur->val = strtol(p, &p, 2);
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
      else
        error("存在しない命令: %s\n", cut_str(tok->str, tok->len));

      // アドレッシングモードの指定
      if(consume("@", TK_RESERVED, &tok)) { // 直接アドレス
        printf("10");
      }
      else if(consume("[", TK_RESERVED, &tok)) { // 間接アドレス
        printf("010000000000\n");
        consume("amr", TK_IDENT, &tok);
        consume("]", TK_RESERVED, &tok);
        continue;
      } else if(tok->kind == TK_HEX || tok->kind == TK_BIN) { // 即値
        printf("00");
      } else if(consume("acr", TK_IDENT, &tok)) { // ACR
        printf("110000000000\n");
        continue;
      } else if(consume("pc", TK_IDENT, &tok)) { // PC
        printf("111000000000\n");
        continue;
      } else if(tok->kind == TK_IDENT) { // ラベルによる直接アドレス
        printf("00");
      }
      
      if(consume(",", TK_RESERVED, &tok))
        tok = tok->next;

      // オペランドの指定
      if(tok->kind == TK_HEX || tok->kind == TK_BIN)
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
      tok = tok->next;
      if(tok->kind == TK_NEWLINE)
        size++;
      continue;
    }
    tok = tok->next;
  }
}