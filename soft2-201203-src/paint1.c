/*
  rect, circleコマンドを実装

  どちらもlineと同じように整数の引数を必要とするため、lineの処理で引数を読み込んでいた部分を関数化した。(read_int_arguments)
  また、キャンバス外に書き込もうとするとエラーになってしまうので、キャンバスを書き換える部分はdraw_dotという関数で行うようにした。
  これによってdraw_dotの内部でのみ範囲外かどうかを判定すればよくなった。
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h> // for error catch
#include <assert.h>
#include <math.h>

// Structure for canvas
typedef struct
{
  int width;
  int height;
  char **canvas;
  char pen;
} Canvas;

// 最大履歴と現在位置の情報は持たない
typedef struct command Command;
struct command {
  char *str;
  size_t bufsize; // このコマンドの長さ + 1 ('\0')
  Command *next;
};

// コマンドリストの先頭へのポインタをメンバに持つ構造体としてHistoryを考える。
// 履歴がない時点ではbegin = NULL となる。
typedef struct {
  Command *begin;
  size_t bufsize; // コマンドの長さの最大値
  size_t size; // コマンドの数
} History;

// functions for Canvas type
Canvas *init_canvas(int width, int height, char pen);
void reset_canvas(Canvas *c);
void print_canvas(FILE *fp, Canvas *c);
void free_canvas(Canvas *c);

// display functions
void rewind_screen(FILE *fp,unsigned int line);
void clear_command(FILE *fp);
void clear_screen(FILE *fp);

// enum for interpret_command results
typedef enum res{ EXIT, NORMAL, COMMAND, UNKNOWN, ERROR} Result;

int max(const int a, const int b);
int draw_dot(Canvas *c, const int x, const int y); //(x, y)がキャンバス内なら点を打つ。外なら1を返すのみ。
void draw_line(Canvas *c, const int x0, const int y0, const int x1, const int y1);
void draw_rect(Canvas *c, const int x0, const int y0, const int width, const int height);
void draw_circle(Canvas *c, const int x0, const int y0, const int r);

int* read_int_arguments(const int count); // countの数だけコマンドの引数を読み込みその配列を返す
Result interpret_command(const char *command, History *his, Canvas *c);
void save_history(const char *filename, History *his);
int load_history(const char *filename, History *his); //返り値はResult


int main(int argc, char **argv)
{

  History *his = (History*)malloc(sizeof(History));
  his->bufsize = 1000;

  int width;
  int height;
  if (argc != 3){
    fprintf(stderr,"usage: %s <width> <height>\n",argv[0]);
    return EXIT_FAILURE;
  } else{
    char *e;
    long w = strtol(argv[1],&e,10);
    if (*e != '\0'){
      fprintf(stderr, "%s: irregular character found %s\n", argv[1],e);
      return EXIT_FAILURE;
    }
    long h = strtol(argv[2],&e,10);
    if (*e != '\0'){
      fprintf(stderr, "%s: irregular character found %s\n", argv[2],e);
      return EXIT_FAILURE;
    }
    width = (int)w;
    height = (int)h;    
  }
  char pen = '*';

  FILE *fp;
  char buf[his->bufsize];
  fp = stdout;
  Canvas *c = init_canvas(width,height, pen);

  fprintf(fp,"\n"); // required especially for windows env
  while (1) {

    print_canvas(fp,c);
    printf("%zu > ", his->size);
    if(fgets(buf, his->bufsize, stdin) == NULL) break;

    const Result r = interpret_command(buf, his, c);
    if (r == EXIT) break;   
    if (r == NORMAL) {

      Command *command = (Command*)malloc(sizeof(Command));
      int len = strlen(buf);
      command->bufsize = len + 1;
      command->str = (char*)malloc(command->bufsize);
      strcpy(command->str, buf);
      command->next = NULL;

      his->size++;

      if (his->begin == NULL) {
        his->begin = command;
      } else {
        Command *node = his->begin;
        while (node->next != NULL) {
          node = node->next;
        }
        node->next = command;
      }

    }

    rewind_screen(fp,2); // command results
    clear_command(fp); // command itself
    rewind_screen(fp, height+2); // rewind the screen to command input

  }

  free_canvas(c);
  fclose(fp);

  return 0;
}

Canvas *init_canvas(int width,int height, char pen)
{
  Canvas *new = (Canvas *)malloc(sizeof(Canvas));
  new->width = width;
  new->height = height;
  new->canvas = (char **)malloc(width * sizeof(char *));

  char *tmp = (char *)malloc(width*height*sizeof(char));
  memset(tmp, ' ', width*height*sizeof(char));
  for (int i = 0 ; i < width ; i++){
    new->canvas[i] = tmp + i * height;
  }
  
  new->pen = pen;
  return new;
}

void reset_canvas(Canvas *c)
{
  const int width = c->width;
  const int height = c->height;
  memset(c->canvas[0], ' ', width*height*sizeof(char));
}


void print_canvas(FILE *fp, Canvas *c)
{
  const int height = c->height;
  const int width = c->width;
  char **canvas = c->canvas;
  
  // 上の壁
  fprintf(fp,"+");
  for (int x = 0 ; x < width ; x++)
    fprintf(fp, "-");
  fprintf(fp, "+\n");

  // 外壁と内側
  for (int y = 0 ; y < height ; y++) {
    fprintf(fp,"|");
    for (int x = 0 ; x < width; x++){
      const char c = canvas[x][y];
      fputc(c, fp);
    }
    fprintf(fp,"|\n");
  }
  
  // 下の壁
  fprintf(fp, "+");
  for (int x = 0 ; x < width ; x++)
    fprintf(fp, "-");
  fprintf(fp, "+\n");
  fflush(fp);
}

void free_canvas(Canvas *c)
{
  free(c->canvas[0]); //  for 2-D array free
  free(c->canvas);
  free(c);
}

void rewind_screen(FILE *fp,unsigned int line)
{
  fprintf(fp,"\e[%dA",line);
}

void clear_command(FILE *fp)
{
  fprintf(fp,"\e[2K");
}

void clear_screen(FILE *fp)
{
  fprintf(fp, "\e[2J");
}


int max(const int a, const int b)
{
  return (a > b) ? a : b;
}
void draw_line(Canvas *c, const int x0, const int y0, const int x1, const int y1)
{
  const int width = c->width;
  const int height = c->height;
  char pen = c->pen;
  
  const int n = max(abs(x1 - x0), abs(y1 - y0));
  c->canvas[x0][y0] = pen;
  for (int i = 1; i <= n; i++) {
    const int x = x0 + i * (x1 - x0) / n;
    const int y = y0 + i * (y1 - y0) / n;
    if ( (x >= 0) && (x< width) && (y >= 0) && (y < height))
      c->canvas[x][y] = pen;
  }
}

int draw_dot(Canvas *c, const int x, const int y) {
  if (x < 0 || c->width <= x) return 1;
  if (y < 0 || c->height <= y) return 1;

  c->canvas[x][y] = c->pen;
  return 0;
}

void draw_rect(Canvas *c, const int x0, const int y0, const int width, const int height) {

  // 上下の辺
  for (int x=x0; x<x0+width; x++) {
    draw_dot(c, x, y0);
    draw_dot(c, x, y0+height-1);
  }

  // 左右の辺
  for (int y=y0; y<y0+height; y++) {
    draw_dot(c, x0, y);
    draw_dot(c, x0+width-1, y);
  }

}

void draw_circle(Canvas *c, const int x0, const int y0, const int r) {

  for (int x=0; x<c->width; x++) {
    for (int y=0; y<c->height; y++) {
      double dist = sqrt(pow(x-x0, 2) + pow(y - y0, 2));
      if (r <= dist && dist < r+1) {
        draw_dot(c, x, y);
      }
    }
  }

}

void save_history(const char *filename, History *his)
{
  const char *default_history_file = "history.txt";
  if (filename == NULL)
    filename = default_history_file;
  
  FILE *fp;
  if ((fp = fopen(filename, "w")) == NULL) {
    fprintf(stderr, "error: cannot open %s.\n", filename);
    return;
  }

  for (Command *command = his->begin; command != NULL; command = command->next) {
    fprintf(fp, "%s", command->str);
  }

  fclose(fp);
}

int* read_int_arguments(const int count) {
  int *p = (int*)calloc(count, sizeof(int));
  char **b = (char**)calloc(count, sizeof(char*));

  for (int i = 0 ; i < count; i++){
    b[i] = strtok(NULL, " ");
    if (b[i] == NULL){
      clear_command(stdout);
      printf("usage: rect <x> <y> <width> <height>\n");
      return NULL;
    }
  }

  for (int i = 0 ; i < count ; i++){
    char *e;
    long v = strtol(b[i],&e, 10);
    if (*e != '\0'){
      clear_command(stdout);
      printf("Non-int value is included.\n");
      return NULL;
    }
    p[i] = (int)v;
  }

  free(b);

  return p;
}

Result interpret_command(const char *command, History *his, Canvas *c)
{
  char buf[his->bufsize];
  strcpy(buf, command);

  assert(strlen(buf) != 0);
  buf[strlen(buf) - 1] = 0; // remove the newline character at the end

  const char *s = strtok(buf, " ");

  if (s == NULL) { // 何も入力されなかった場合
    clear_command(stdout);
    printf("none!\n");
    return UNKNOWN;
  }

  // The first token corresponds to command
  if (strcmp(s, "line") == 0) {
    int *args = read_int_arguments(4);
    if (args == NULL) {
      return ERROR;
    }

    draw_line(c, args[0], args[1], args[2], args[3]);

    free(args);
    clear_command(stdout);
    printf("1 line drawn\n");
    return NORMAL;
  }
  
  if (strcmp(s, "rect") == 0) {
    int *args = read_int_arguments(4);
    if (args == NULL) {
      return ERROR;
    }

    draw_rect(c, args[0], args[1], args[2], args[3]);

    free(args);
    clear_command(stdout);
    printf("1 rect drawn\n");
    return NORMAL;
  }

  if (strcmp(s, "circle") == 0) {
    int *args = read_int_arguments(3);
    if (args == NULL) {
      return ERROR;
    }

    draw_circle(c, args[0], args[1], args[2]);

    free(args);
    clear_command(stdout);
    printf("1 circle drawn\n");
    return NORMAL;
  }

  if (strcmp(s, "save") == 0) {
    s = strtok(NULL, " ");
    save_history(s, his);
    clear_command(stdout);
    printf("saved as \"%s\"\n",(s==NULL)?"history.txt":s);
    return COMMAND;
  }

  if (strcmp(s, "undo") == 0) {
    reset_canvas(c);

    if (his->begin == NULL) { // コマンドが1つもなかった場合

      clear_command(stdout);
      printf("none!\n");

    } else if (his->begin->next == NULL) { // コマンドが1つだけだった場合

      free(his->begin->str);
      free(his->begin);
      his->begin = NULL;
      his->size--;

      clear_command(stdout);
      printf("undo!\n");

    } else {

      for (Command *com = his->begin; com != NULL; com = com->next) {
        // 最初から実行し直す
        interpret_command(com->str, his, c);
        rewind_screen(stdout, 1);

        // 実行したのが最後から2番目のコマンドなら、次のコマンドを消去して終了
        if (com->next->next == NULL) {
          free(com->next->str);
          free(com->next);
          com->next = NULL;
          his->size--;
          break;
        }

      }

      clear_command(stdout);
      printf("undo!\n");

    }

    return COMMAND;
  }

  if (strcmp(s, "quit") == 0) {
    return EXIT;
  }
  clear_command(stdout);
  printf("error: unknown command.\n");
  return UNKNOWN;
}
