typedef struct layer Layer;
struct layer {
  int **board;
  int **color; // 30-37
  int **bgcolor; // 40-47
  int visible; // 表示する場合は1
  int clipped; // 下のレイヤーにクリッピングされる場合は1
  Layer *next;
  Layer *prev;
};

typedef struct {
  Layer *begin;
  size_t size;
} Layer_List;

typedef struct {
  int **board;
  int **color;
  int **bgcolor;
  int width;
  int height;
} Clipboard;

// Structure for canvas
typedef struct
{
  int width;
  int height;
  int width_default;
  int height_default;
  int aspect;
  int layer_index;
  Layer_List *layer_list;
  char pen;
  char pen_default;
  int pen_size;
  int color; // 0-7
  Clipboard *clipboard; // コピーしたものを保存する
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
void print_history(Canvas *c, History *his);
void free_canvas(Canvas *c);

// display functions
void rewind_screen(unsigned int line);
void forward_screen(unsigned int line);
void clear_line();
void clear_screen(); // カーソル以降をすべて消去する

// enum for interpret_command results
typedef enum res{ EXIT, NORMAL, COMMAND, UNKNOWN, ERROR} Result;

int max(const int a, const int b);
int in_board(int x, int y, Canvas *c);
int draw_dot(Canvas *c, const int x, const int y); //(x, y)がキャンバス内なら点を打つ。外なら1を返すのみ。
void draw_line(Canvas *c, const int x0, const int y0, const int x1, const int y1);
void draw_rect(Canvas *c, const int x0, const int y0, const int width, const int height, int fill);
void draw_circle(Canvas *c, const int x0, const int y0, const int r, int fill);
void copy_to_clipboard(Canvas *c, Clipboard *clip, int x0, int y0, int w, int h);
void paste_from_clipboard(Canvas *c, Clipboard *clip, int x0, int y0);
Clipboard *construct_clipboard();
void free_clipboard(Clipboard *clip);

int *read_int_arguments(const int count); // countの数だけコマンドの引数を読み込みその配列を返す。
int *read_int_arguments_flex(int *len); // 可変長引数を読み込み配列を返す。長さをlenに書き込む。
Result interpret_command(const char *command, History *his, Canvas *c);
void save_history(const char *filename, History *his);
int load_history(const char *filename, History *his); //エラー時は1を返す。

// actualが0ならundoで取り消されていないもののうちの最後、1なら履歴全体で最後のコマンド(なければNULL)を返す。
Command *get_last_command(History *his, const int actual);
// command以降の履歴をすべて削除する
void remove_commands(Command *command);
void push_back_history(History *his, char *str);
Command *construct_command(char *str);

Layer *get_layer(Canvas *c, int index);
Layer *get_cur_layer(Canvas *c);
Layer *get_last_layer(Canvas *c);
int reverse_layer(Canvas *c, int index, char *mode);
int hide_layer(Canvas *c, int index);
int show_layer(Canvas *c, int index);
int change_layer(Canvas *c, int index);
// layer_listの最後に空のレイヤーを追加する。
void add_layer(Canvas *c);
// layerは空のレイヤーか、remove_layerでリストから切り離されているもの。
int insert_layer(Canvas *c, int index, Layer *layer);
// layer_listからlayerを切り離す。
// メモリも解放する場合はfreeing=1にする。
int remove_layer(Canvas *c, int index, int freeing);
// a番目のレイヤーをb番目に移動する。
int move_layer(Canvas *c, int a, int b);
int **get_2darray(int width, int height);
// 空のレイヤーを作る。
Layer *construct_layer(int width, int height);
int resize_layer(Canvas *c, int index, int width, int height);
int copy_layer(Canvas *c, int index);
void copy_board(int width, int height, int **board, int **color, int **bgcolor, Layer *layer);
int merge_layers(Canvas *c, int len, int indices[]);
// 下のレイヤーに結合する
int merge_layer(Canvas *c, int index);
// レイヤーaをレイヤーbにクリッピング(b = -1で解除)。
int clip_layer(Canvas *c, int a, int b);
void free_2darray(int **array);
void free_board(Layer *layer);
void free_layer(Layer *layer);
void free_all_layers(Canvas *c);