#include "defines.h"

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

  printf("ラベル情報:\n");
  for(Label *lab = label; lab; lab = lab->next) {
    printf("%s: 0x%03x\n", cut_str(lab->name, lab->len), lab->addr);
  }

  // for(Label *lab = label; lab; lab = lab->next) {
  //   fprintf(stderr, "lab->name: %s\n", lab->name);
  //   fprintf(stderr, "lab->len: %d\n", lab->len);
  //   fprintf(stderr, "lab->addr: 0x");
  //   print_bit(lab->addr, 10);
  //   putchar('\n');
  // }

  // Token *tok = token;
  // for(; tok; tok = tok->next) {
  //   printf("tok->kind: %d\n", tok->kind);
  //   printf("tok->val: %d\n", tok->val);
  //   printf("tok->len: %d\n", tok->len);
  // }

  printf("コード生成スタート\n\n");

  gen(token);

  return 0;
}