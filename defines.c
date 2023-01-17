#include "defines.h"

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

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

void print_bit(int val, int num) {
  int i;
  for(i = 0; i < num; i++) {
    printf("%d", val & (1<<(num-1)) ? 1 : 0);
    val <<= 1;
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