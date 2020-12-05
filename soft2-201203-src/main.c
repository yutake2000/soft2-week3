#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h> // for error catch
#include <assert.h>
#include <math.h>

typedef struct layer Layer;
struct layer {
  char **board;
  int visible;
  Layer *next;
  Layer *prev;
};

typedef struct {
  Layer *begin;
} Layer_List;

// Structure for canvas
typedef struct
{
  int width;
  int height;
  int layer_index;
  Layer_List *layer_list;
  char pen;
  char pen_default;
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
int load_history(const char *filename, History *his); //エラー時は1を返す

// actualが0ならundoで取り消されていないもののうちの最後、1なら履歴全体で最後のコマンド(なければNULL)を返す
Command *get_last_command(History *his, const int actual);
// command以降の履歴をすべて削除する
void remove_commands(Command *command);
void push_back_history(History *his, char *str);
Command *construct_command(char *str);

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
      push_back_history(his, buf);
    }

    rewind_screen(fp,2); // command results
    clear_command(fp); // command itself
    rewind_screen(fp, height+2); // rewind the screen to command input

  }

  free_canvas(c);
  fclose(fp);

  return 0;
}

void free_layer(Layer *layer) {
  free(layer->board[0]);
  free(layer->board);
  free(layer);
}

void free_layers(Canvas *c) {
  Layer *layer = c->layer_list->begin->next; // 最初のレイヤーは除く

  while(layer != NULL) {
    Layer *temp = layer;
    layer = layer->next;

    free_layer(temp);
  }
}

Layer *get_layer(Canvas *c, int index) {
  Layer *layer = c->layer_list->begin;
  for (int i=0; i<index; i++) {
    layer = layer->next;
  }
  return layer;
}

int remove_layer(Canvas *c, int index) {
  Layer *layer = get_layer(c, index);

  if (index == 0 && layer->next == NULL) {
    printf("Can't remove all layers.\n");
    return 1;
  }

  if (layer->prev != NULL) {
    layer->prev->next = layer->next;
  }

  if (layer->next != NULL) {
    layer->next->prev = layer->prev;
  }

  if (index == 0) {
    c->layer_list->begin = layer->next;
  }

  free_layer(layer);

  return 0;
}

void swap_layers(Canvas *c, int a, int b) {

  /*
  Layer *layer1 = get_layer(c, a);
  Layer *layer2 = get_layer(c, b);

  Layer *prev = layer2->prev;
  Layer *next = layer2->next;

  layer2->prev = layer1->prev;
  layer2->next = layer1->next;

  layer1->prev = prev;
  layer1->next = next;

  if (layer1->prev != NULL) layer1->prev->next = layer1;
  if (layer1->next != NULL) layer1->next->prev = layer1;
  if (layer2->prev != NULL) layer2->prev->next = layer2;
  if (layer2->next != NULL) layer2->next->prev = layer2;
  */

}

Layer *cur_layer(Canvas *c) {
  return get_layer(c, c->layer_index);
}

Layer *get_last_layer(Canvas *c) {
  Layer *layer = c->layer_list->begin;

  assert(layer != NULL);

  while (layer->next != NULL) layer = layer->next;

  return layer;
}

Layer *construct_layer(int width, int height) {
  Layer *layer = (Layer*)malloc(sizeof(Layer));
  layer->next = NULL;
  layer->prev = NULL;
  layer->visible = 1;
  layer->board = (char**)malloc(width * sizeof(char*));

  char *tmp = (char*)malloc(width * height * sizeof(char));
  memset(tmp, ' ', width * height * sizeof(char));
  for (int i = 0 ; i < width ; i++){
    layer->board[i] = tmp + i * height;
  }

  return layer;
}

void add_layer(Canvas *c) {
  Layer *last = get_last_layer(c);
  assert(last != NULL);

  Layer *new = construct_layer(c->width, c->height);

  last->next = new;
  new->prev = last;
}

Canvas *init_canvas(int width,int height, char pen)
{
  Canvas *new = (Canvas *)malloc(sizeof(Canvas));
  new->width = width;
  new->height = height;
  new->layer_list = (Layer_List*)malloc(sizeof(Layer_List));
  new->layer_list->begin = construct_layer(width, height);
  new->layer_index = 0;
  
  new->pen = pen;
  new->pen_default = pen;
  return new;
}

void reset_canvas(Canvas *c)
{
  const int width = c->width;
  const int height = c->height;
  c->pen = c->pen_default;
  free_layers(c);

  memset(get_layer(c, 0)->board[0], ' ', width * height * sizeof(char));
}


void print_canvas(FILE *fp, Canvas *c)
{
  const int height = c->height;
  const int width = c->width;
  char **board = cur_layer(c)->board;
  
  // 上の壁
  fprintf(fp,"+");
  for (int x = 0 ; x < width ; x++)
    fprintf(fp, "-");
  fprintf(fp, "+\n");

  // 外壁と内側
  for (int y = 0 ; y < height ; y++) {
    fprintf(fp,"|");
    for (int x = 0 ; x < width; x++){
      const char c = board[x][y];
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
  free_layers(c);
  Layer *layer = get_layer(c, 0);
  free(layer->board[0]); //  for 2-D array free
  free(layer->board);
  free(layer);
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
  
  const int n = max(abs(x1 - x0), abs(y1 - y0));
  draw_dot(c, x0, y0);
  for (int i = 1; i <= n; i++) {
    const int x = x0 + i * (x1 - x0) / n;
    const int y = y0 + i * (y1 - y0) / n;
    draw_dot(c, x, y);
  }
}

int draw_dot(Canvas *c, const int x, const int y) {
  if (x < 0 || c->width <= x) return 1;
  if (y < 0 || c->height <= y) return 1;

  Layer *layer = cur_layer(c);
  layer->board[x][y] = c->pen;
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

  // undoで取り消した部分は保存しない
  Command *command = his->begin;
  for (int i=0; i < his->size; i++) {
    fprintf(fp, "%s", command->str);
    command = command->next;
  }

  fclose(fp);
}

int load_history(const char *filename, History *his) {
  const char *default_history_file = "history.txt";
  if (filename == NULL)
    filename = default_history_file;

  FILE *fp;
  if ((fp = fopen(filename, "r")) == NULL) {
    clear_command(stdout);
    printf("error: cannot open %s.\n", filename);
    return 1;
  }

  // コマンド履歴を全削除
  remove_commands(his->begin);
  his->begin = NULL;
  his->size = 0;

  char *buf = (char*)malloc(sizeof(char) * his->bufsize);
  while ((fgets(buf, his->bufsize, fp)) != NULL) {
    push_back_history(his, buf);
  }

  return 0;
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

  if (strcmp(s, "load") == 0) {
    s = strtok(NULL, " ");
    int result = load_history(s, his);
    if (result == 1) {
      return ERROR;
    }

    // コマンドをすべて実行する(undoはされていないのでNULLになるまですべて実行してOK)
    reset_canvas(c);
    for (Command *com = his->begin; com != NULL; com = com->next) {
      interpret_command(com->str, his, c);
      rewind_screen(stdout, 1);
    }

    clear_command(stdout);
    printf("loaded \"%s\"\n",(s==NULL)?"history.txt":s);

    return COMMAND;
  }

  if (strcmp(s, "chpen") == 0) {
    s = strtok(NULL, " ");
    if (s == NULL) {
      clear_command(stdout);
      printf("usage: chpen <character>\n");
      return ERROR;
    }

    c->pen = s[0];

    clear_command(stdout);
    printf("changed to '%c'\n", c->pen);
    return NORMAL;
  }

  // 実際にはコマンドを削除せずに、his->sizeを減らす
  if (strcmp(s, "undo") == 0) {
    reset_canvas(c);

    if (his->size == 0) { // コマンドが1つもなかった場合

      clear_command(stdout);
      printf("none!\n");

    } else if (his->size == 1) { // コマンドが1つだけだった場合

      his->size--;

      clear_command(stdout);
      printf("undo!\n");

    } else {

      his->size--;

      Command *com = his->begin;
      // 最初から実行し直す
      for (int i=0; i< his->size; i++) {
        interpret_command(com->str, his, c);
        rewind_screen(stdout, 1);

        com = com->next;
      }

      clear_command(stdout);
      printf("undo!\n");

    }

    return COMMAND;
  }

  if (strcmp(s, "redo") == 0) {

    Command *last = get_last_command(his, 0);
    Command *last_actual = get_last_command(his, 1);

    if (last != last_actual) { // undoされた状態
      his->size++;
      if (last == NULL) {
        last = his->begin;
      } else {
        last = last->next;
      }

      interpret_command(last->str, his, c);
      rewind_screen(stdout, 1);

      clear_command(stdout);
      printf("redo!\n");
    } else {
      clear_command(stdout);
      printf("none!\n");
    }
    return COMMAND;
  }

  if (strcmp(s, "layer") == 0) {

    s = strtok(NULL, " ");

    clear_command(stdout);
    if (strcmp(s, "add") == 0) {
      add_layer(c);
      printf("added!\n");
    } else if (strcmp(s, "ch") == 0 || strcmp(s, "change") == 0) {
      int *index = read_int_arguments(1);
      c->layer_index = *index;
      printf("changed!\n");
    } else if (strcmp(s, "rm") == 0 || strcmp(s, "remove") == 0) {
      int *index = read_int_arguments(1);
      int result = remove_layer(c, *index);
      if (result == 1) {
        return ERROR;
      }
      printf("removed!\n");
    } else if (strcmp(s, "swap") == 0) {
      int *indices = read_int_arguments(2);
      swap_layers(c, indices[0], indices[1]);
      printf("swapped\n");
    } else {
      printf("usage: layer [command = add | change]\n");
      return ERROR;
    }

    return NORMAL;
  }

  if (strcmp(s, "quit") == 0) {
    return EXIT;
  }
  clear_command(stdout);
  printf("error: unknown command.\n");
  return UNKNOWN;
}

Command *get_last_command(History *his, const int actual) {

  if (actual) {

    if (his->begin == NULL) {
      return NULL;
    } else {
      Command *command = his->begin;
      while (command->next != NULL) {
        command = command->next;
      }
      return command;
    }

  } else {

    if (his->size == 0) {
      return NULL;
    } else {
      Command *command = his->begin;
      for (int i=0; i < his->size-1; i++) {
        command = command->next;
      }
      return command;
    }

  }

}

void remove_commands(Command *command) {
  while (command != NULL) {
    Command *temp = command;
    command = command->next;

    free(temp->str);
    free(temp);
  }
}

void push_back_history(History *his, char *str) {
  Command *command = construct_command(str);

  Command *node = get_last_command(his, 0);
  if (node != NULL) {
    remove_commands(node->next); // undoで取り消されたコマンドをすべて削除
    node->next = command;
  } else {
    remove_commands(his->begin);
    his->begin = command;
  }

  his->size++;
}

Command *construct_command(char *str) {

  Command *command = (Command*)malloc(sizeof(Command));
  command->bufsize = strlen(str) + 1;
  command->next = NULL;
  command->str = (char*)malloc(command->bufsize);
  strcpy(command->str, str);

  return command;
}