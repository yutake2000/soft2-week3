#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h> // for error catch
#include <assert.h>
#include <math.h>

#include "main.h"
#include "layer.c"

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
    printf("Layer %d/%zu | ", c->layer_index + 1, c->layer_list->size);
    if (c->pen == ' ') { // マーカーの場合
      printf("marker \e[48;5;%dm   \e[0m", c->color);
    } else if (c->pen == 0) {
      printf("eraser");
    } else {
      printf("pen \e[38;5;%dm%c%c%c\e[0m", c->color, c->pen, c->pen, c->pen);
    }
    Command *last = get_last_command(his, 0);
    if (last != NULL) {
      printf(" | %s", last->str);
    } else {
      printf("\n");
    }
    printf("%zu > ", his->size);
    if(fgets(buf, his->bufsize, stdin) == NULL) break;

    const Result r = interpret_command(buf, his, c);
    if (r == EXIT) break;   
    if (r == NORMAL) {
      push_back_history(his, buf);
    }

    rewind_screen(2); // command results
    clear_line(); // command itself
    rewind_screen(1);
    clear_line();
    rewind_screen(c->height+2); // rewind the screen to command input

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
  new->width_default = width;
  new->height_default = height;
  new->aspect = 1;
  new->layer_list = (Layer_List*)malloc(sizeof(Layer_List));
  new->layer_list->begin = construct_layer(width, height);
  new->layer_list->size = 1;
  new->layer_index = 0;
  
  new->pen = pen;
  new->pen_default = pen;
  new->color = 0;
  return new;
}

void reset_canvas(Canvas *c)
{
  c->width = c->width_default;
  c->height = c->height_default;
  c->aspect = 1;
  c->pen = c->pen_default;
  c->color = 0;
  free_all_layers(c);
  c->layer_index = 0;

  c->layer_list->begin = construct_layer(c->width, c->height);
  c->layer_list->size = 1;
}

void print_char(char c, int color, int bgcolor, FILE *fp) {

  fprintf(fp, "\e[38;5;%dm", color);
  if (bgcolor != 0) {
    fprintf(fp, "\e[48;5;%dm", bgcolor);
  }
  fputc(c, fp);
  fprintf(fp, "\e[0m");

}

void print_canvas(FILE *fp, Canvas *c)
{
  const int height = c->height;
  const int width = c->width;
  int **board = get_cur_layer(c)->board;
  
  // 上の壁
  clear_line();
  fprintf(fp,"+");
  for (int x = 0 ; x < width * c->aspect; x++)
    fprintf(fp, "-");
  fprintf(fp, "+\n");

  // 外壁と内側
  for (int y = 0 ; y < height ; y++) {
    clear_line();
    fprintf(fp,"|");
    for (int x = 0 ; x < width; x++) {
      char ch = ' ';
      int color = 0;
      int bgcolor = 0;
      int is_current_layer = 0;
      // 番号が大きいレイヤーを上に表示する
      for (int i=0; i<c->layer_list->size; i++) {
        Layer *layer = get_layer(c, i);

        if (!layer->visible || layer->board[x][y] == 0)
          continue;

        // クリップ元のレイヤーに何も書かれていない場合は表示しない
        Layer *clipping_layer = layer;
        while (clipping_layer != NULL && clipping_layer->clipped && clipping_layer->prev != NULL)
          clipping_layer = clipping_layer->prev;

        if (layer->clipped && clipping_layer != NULL &&
          (!clipping_layer->visible || clipping_layer->board[x][y] == 0))
          continue;

        ch = layer->board[x][y];
        color = layer->color[x][y];
        if (layer->bgcolor[x][y] != 0) {
        	bgcolor = layer->bgcolor[x][y];
        }
        is_current_layer = (i == c->layer_index);
      }
      
      for (int i=0; i<c->aspect; i++) {
        print_char(ch, color, bgcolor, fp);
      }
    }
    fprintf(fp,"|\n");
  }
  
  // 下の壁
  clear_line();
  fprintf(fp, "+");
  for (int x = 0 ; x < width * c->aspect; x++)
    fprintf(fp, "-");
  fprintf(fp, "+\n");
  fflush(fp);
}

void free_canvas(Canvas *c)
{
  free_all_layers(c);
  free(c);
}

void rewind_screen(unsigned int line)
{
  printf("\e[%dA",line);
}

void forward_screen(unsigned int line) {
  for (int i=0; i<line; i++) {
    printf("\n");
  }
}

void clear_line()
{
  printf("\e[2K");
}

void clear_screen()
{
  printf("\e[0J");
}

int max(const int a, const int b)
{
  return (a > b) ? a : b;
}

int draw_dot(Canvas *c, const int x, const int y) {
  if (x < 0 || c->width <= x) return 1;
  if (y < 0 || c->height <= y) return 1;

  Layer *layer = get_cur_layer(c);

  if (c->pen == ' ') { // マーカーの場合
    layer->board[x][y] = ' ';
    layer->bgcolor[x][y] = c->color; // \e[40mからが背景色指定
  } else if (c->pen == 0) { // 消しゴムの場合
    layer->board[x][y] = 0;
    layer->color[x][y] = 0;
    layer->bgcolor[x][y] = 0;
  } else {
    layer->board[x][y] = c->pen;
    layer->color[x][y] = c->color; // \e[30mからが文字色指定
  }

  return 0;
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

void draw_rect(Canvas *c, const int x0, const int y0, const int width, const int height, int fill) {

  if (fill) {
    for (int x=x0; x<x0+width; x++) {
      for (int y=y0; y<y0+height; y++) {
        draw_dot(c, x, y);
      }
    }
  } else {
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

}

void draw_circle(Canvas *c, const int x0, const int y0, const int r, int fill) {

  for (int x=0; x<c->width; x++) {
    for (int y=0; y<c->height; y++) {
      double dist = sqrt(pow(x-x0, 2) + pow(y - y0, 2));
      if (fill && dist < r) {
        draw_dot(c, x, y);
      } else if (r-1 <= dist && dist < r) {
        draw_dot(c, x, y);
      }
    }
  }

}

int in_board(int x, int y, Canvas *c) {

  if (x < 0 || c->width <= x) return 0;
  if (y < 0 || c->height <= y) return 0;

  return 1;
}

void copy_and_paste(Canvas *c, int x0, int y0, int w, int h, int x1, int y1) {

  int width = c->width;
  int height = c->height;

  int board[width][height];
  int color[width][height];
  int bgcolor[width][height];

  Layer *layer = get_cur_layer(c);
  for (int x=0; x<width; x++) {
    for (int y=0; y<height; y++) {
      board[x][y] = layer->board[x][y];
      color[x][y] = layer->color[x][y];
      bgcolor[x][y] = layer->bgcolor[x][y];
    }
  }
  
  int dx = x1 - x0;
  int dy = y1 - y0;
  for (int x=x0; x<x0+w; x++) {
    for (int y=y0; y<y0+h; y++) {
      if (!in_board(x, y, c))
        continue;

      int nx = x + dx;
      int ny = y + dy;
      if (!in_board(nx, ny, c))
        continue;

      board[nx][ny] = layer->board[x][y];
      color[nx][ny] = layer->color[x][y];
      bgcolor[nx][ny] = layer->bgcolor[x][y];
    }
  }

  for (int x=0; x<width; x++) {
    for (int y=0; y<height; y++) {
      layer->board[x][y] = board[x][y];
      layer->color[x][y] = color[x][y];
      layer->bgcolor[x][y] = bgcolor[x][y];
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

int *read_int_arguments(const int count) {
  int *p = (int*)calloc(count, sizeof(int));
  char **b = (char**)calloc(count, sizeof(char*));

  for (int i = 0 ; i < count; i++){
    b[i] = strtok(NULL, " ");
    if (b[i] == NULL){
      printf("too few arguments.\n");
      return NULL;
    }
  }

  for (int i = 0 ; i < count ; i++){
    char *e;
    long v = strtol(b[i],&e, 10);
    if (*e != '\0'){
            printf("Non-int value is included.\n");
      return NULL;
    }
    p[i] = (int)v;
  }

  free(b);

  return p;
}

int *read_int_arguments_flex(int *len) {

  char **b = (char**)calloc(16, sizeof(char*));

  int count = 0;
  while(1) {
    b[count] = strtok(NULL, " ");
    if (b[count] == NULL) {
      break;
    }
    count++;
  }

  *len = count;
  int *p = (int*)calloc(count, sizeof(int));

  for (int i = 0 ; i < count ; i++){
    char *e;
    long v = strtol(b[i],&e, 10);
    if (*e != '\0'){
            printf("Non-int value is included.\n");
      return NULL;
    }
    p[i] = (int)v;
  }

  free(b);

  return p;
}

int resize_canvas(Canvas *c, int width, int height) {

  if (width <= 0 || height <= 0)
    return 1;

  int len = c->layer_list->size;
  for (int i=0; i<len; i++) {
    resize_layer(c, i, width, height);
  }

  int diff = height - c->height;

  c->width = width;
  c->height = height;

  if (diff < 0) {
    diff = -diff;
    for (int i=0; i<diff; i++) {
      rewind_screen(1);
      clear_line();
    }
  } else {
    for (int i=0; i<diff; i++) {
      forward_screen(1);
    }
  }

  return 0;
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

int read_layer_index(int default_value) {
  int len;
  int *indices = read_int_arguments_flex(&len);
  int index = default_value;
  if (len >= 1)
    index = indices[0] - 1;
  if (default_value == -1 && len == 0)
    printf("too few arguments.\n");
  return index;
}

Result interpret_command(const char *command, History *his, Canvas *c)
{
  char buf[his->bufsize];
  strcpy(buf, command);

  assert(strlen(buf) != 0);
  buf[strlen(buf) - 1] = 0; // remove the newline character at the end

  char *s = strtok(buf, " ");

  if (s == NULL) {
    s = "redo";
    /*
        printf("none!\n");
    return UNKNOWN;
    */
  }

  clear_line();

  // The first token corresponds to command
  if (strcmp(s, "line") == 0) {
    int *args = read_int_arguments(4);
    if (args == NULL) {
      return ERROR;
    }

    draw_line(c, args[0], args[1], args[2], args[3]);

    free(args);
    printf("1 line drawn\n");
    return NORMAL;
  }
  
  if (strcmp(s, "rect") == 0) {
    int *args = read_int_arguments(4);
    if (args == NULL) {
      return ERROR;
    }

    char *option = strtok(NULL, " ");
    int fill = (option != NULL && strcmp(option, "fill") == 0);

    draw_rect(c, args[0], args[1], args[2], args[3], fill);

    free(args);
    printf("1 rect drawn\n");
    return NORMAL;
  }

  if (strcmp(s, "circle") == 0) {
    int *args = read_int_arguments(3);
    if (args == NULL) {
      return ERROR;
    }

    char *option = strtok(NULL, " ");
    int fill = (option != NULL && strcmp(option, "fill") == 0);
    draw_circle(c, args[0], args[1], args[2], fill);

    free(args);
        printf("1 circle drawn\n");
    return NORMAL;
  }

  if (strcmp(s, "save") == 0) {
    s = strtok(NULL, " ");
    save_history(s, his);
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
      rewind_screen(1);
    }

        printf("loaded \"%s\"\n",(s==NULL)?"history.txt":s);

    return COMMAND;
  }

  if (strcmp(s, "animate") == 0) {
    s = strtok(NULL, " ");
    int result = load_history(s, his);
    if (result == 1) {
      return ERROR;
    }

    reset_canvas(c);
    his->size = 0;

    printf("loaded \"%s\"\n",(s==NULL)?"history.txt":s);

    return COMMAND;
  }

  if (strcmp(s, "chpen") == 0) {
    s = strtok(NULL, " ");
    if (s == NULL) {
            printf("usage: chpen <character>\n");
      return ERROR;
    }

    c->pen = s[0];

    printf("changed!\n");
    return NORMAL;
  }

  if (strcmp(s, "marker") == 0) {
    c->pen = ' ';
    printf("changed!\n");
    return NORMAL;
  }

  if (strcmp(s, "eraser") == 0) {
    c->pen = 0;
    printf("changed!\n");
    return NORMAL;
  }

  if (strcmp(s, "color") == 0) {
    int *color = read_int_arguments(1);
    if (color == NULL) {
      return ERROR;
    }
    c->color = *color;
    printf("changed!\n");
    return NORMAL;
  }

  // 実際にはコマンドを削除せずに、his->sizeを減らす
  if (strcmp(s, "undo") == 0) {
    reset_canvas(c);

    if (his->size == 0) { // コマンドが1つもなかった場合
      printf("none!\n");
    } else if (his->size == 1) { // コマンドが1つだけだった場合
      his->size--;
      printf("undo!\n");
    } else {

      his->size--;

      Command *com = his->begin;
      // 最初から実行し直す
      for (int i=0; i< his->size; i++) {
        interpret_command(com->str, his, c);
        rewind_screen(1);

        com = com->next;
      }

      clear_line();
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
    } else {
      printf("none!\n");
    }
    return COMMAND;
  }

  if (strcmp(s, "layer") == 0 || strcmp(s, "l") == 0) {

    s = strtok(NULL, " ");

    if (strcmp(s, "add") == 0) {
      add_layer(c);
      change_layer(c, c->layer_list->size-1);
      printf("added!\n");
    } else if (strcmp(s, "ch") == 0 || strcmp(s, "change") == 0) {
      int index = read_layer_index(-1);
      if (index == -1)
        return ERROR;

      int result = change_layer(c, index);
      if (result == 1)
        return ERROR;

      printf("changed!\n");
    } else if (strcmp(s, "rm") == 0 || strcmp(s, "remove") == 0) {
      int index = read_layer_index(c->layer_index);

      int result = remove_layer(c, index, 1); // freeもする
      if (result == 1)
        return ERROR;

      printf("removed!\n");
    } else if (strcmp(s, "insert") == 0) {
      int index = read_layer_index(-1);
      if (index == -1)
        return ERROR;

      int result = insert_layer(c, index, construct_layer(c->width, c->height));
      if (result == 1)
        return ERROR;

      printf("inserted\n");
    } else if (strcmp(s, "mv") == 0 || strcmp(s, "move") == 0) {
      int *indices = read_int_arguments(2);
      if (indices == NULL)
        return ERROR;

      int result = move_layer(c, indices[0] - 1, indices[1] - 1);
      if (result == 1)
        return ERROR;

      printf("moved!\n");
    } else if (strcmp(s, "show") == 0) {
      int index = read_layer_index(c->layer_index);

      int result = 0;
      if (index == -1) { // 0を指定した場合はすべてのレイヤーを表示する
        for (int i=0; i<c->layer_list->size; i++) {
          show_layer(c, i);
        }
      } else {
        result = show_layer(c, index);
      }

      if (result == 1)
        return ERROR;

      printf("showed!\n");
    } else if (strcmp(s, "hide") == 0) {
      int index = read_layer_index(c->layer_index);

      int result = 0;
      if (index == -1) { // 0を指定した場合は現在のレイヤー以外を非表示にする
        for (int i=0; i<c->layer_list->size; i++) {
        if (i != c->layer_index)
          hide_layer(c, i);
        }
      } else {
        result = hide_layer(c, index);
      }

      if (result == 1)
        return ERROR;

      printf("hidden!\n");
    } else if (strcmp(s, "cp") == 0 || strcmp(s, "copy") == 0) {
      int index = read_layer_index(c->layer_index);

      int result = copy_layer(c, index);
      if (result == 1)
        return ERROR;

      printf("copied!\n");
    } else if (strcmp(s, "merge") == 0) {
      int index = read_layer_index(c->layer_index);

      int result = merge_layer(c, index);
      if (result == 1)
        return ERROR;

      printf("merged!\n");
    } else if (strcmp(s, "clip") == 0 || strcmp(s, "unclip") == 0) {
      int flag = (strcmp(s, "clip") == 0 ? 1 : 0); // clipなら1

      int index = read_layer_index(c->layer_index);

      int result = clip_layer(c, index, flag);
      if (result == 1)
        return ERROR;

      printf(flag ? "clipped!\n" : "unclipped!\n");
    } else {
      printf("usage: layer [command]\n");
      return ERROR;
    }

    return NORMAL;
  }

  if (strcmp(s, "resize") == 0) {
    int *indices = read_int_arguments(2);
    if (indices == NULL)
      return ERROR;

    int result = resize_canvas(c, indices[0], indices[1]);
    if (result == 1)
      return ERROR;

    printf("resized!\n");
    return NORMAL;
  }

  if (strcmp(s, "aspect") == 0) {
    int *aspect = read_int_arguments(1);
    if (aspect == NULL)
      return ERROR;
    if (aspect[0] < 0) {
      printf("invalid!\n");
      return ERROR;
    }

    c->aspect = aspect[0];
    printf("changed!\n");
    return NORMAL;
  }

  if (strcmp(s, "copy") == 0) {

    int *args = read_int_arguments(6);
    if (args == NULL)
      return ERROR;

    copy_and_paste(c, args[0], args[1], args[2], args[3], args[4], args[5]);

    printf("copied!\n");
    return NORMAL;
  }

  if (strcmp(s, "quit") == 0) {
    return EXIT;
  }
    printf("error: unknown command.\n");
  return UNKNOWN;
}