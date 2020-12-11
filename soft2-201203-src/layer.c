Layer *get_layer(Canvas *c, int index) {
  size_t size = c->layer_list->size;
  if (index >= size || index < 0) {
    return NULL;
  }

  Layer *layer = c->layer_list->begin;
  for (int i=0; i<index; i++) {
    layer = layer->next;
  }
  return layer;
}

Layer *get_cur_layer(Canvas *c) {
  return get_layer(c, c->layer_index);
}

Layer *get_last_layer(Canvas *c) {
  Layer *layer = c->layer_list->begin;

  assert(layer != NULL);

  while (layer->next != NULL) layer = layer->next;

  return layer;
}

int reverse_layer(Canvas *c, int index, char *mode) {

  size_t size = c->layer_list->size;
  if (index >= size) {
    printf("out of bounds!\n");
    return 1;
  }

  Layer *layer = get_layer(c, index);
  Clipboard *clip = construct_clipboard();
  int w = c->width;
  int h = c->height;
  copy_to_clipboard(c, clip, 0, 0, w, h);
  if (strcmp(mode, "vertical") == 0) {
    for (int x=0; x<w; x++) {
      for (int y=0; y<h; y++) {
        layer->board[x][y] = clip->board[x][h-1-y];
        layer->color[x][y] = clip->color[x][h-1-y];
        layer->bgcolor[x][y] = clip->bgcolor[x][h-1-y];
      }
    }
  } else if (strcmp(mode, "horizontal") == 0) {
    for (int x=0; x<w; x++) {
      for (int y=0; y<h; y++) {
        layer->board[x][y] = clip->board[w-1-x][y];
        layer->color[x][y] = clip->color[w-1-x][y];
        layer->bgcolor[x][y] = clip->bgcolor[w-1-x][y];
      }
    }
  } else if (strcmp(mode, "diagonal") == 0) {
    if (w != h) {
      printf("Not square!\n");
      return 1;
    }

    for (int x=0; x<w; x++) {
      for (int y=0; y<h; y++) {
        int diff = x - y;
        int xsum = (w-1) + diff;
        int ysum = (h-1) - diff;
        layer->board[x][y] = clip->board[xsum-x][ysum-y];
        layer->color[x][y] = clip->color[xsum-x][ysum-y];
        layer->bgcolor[x][y] = clip->bgcolor[xsum-x][ysum-y];
      }
    }
  } else {
    printf("No such mode!\n");
    free_clipboard(clip);
    return 1;
  }

  free_clipboard(clip);
  return 0;
}

int hide_layer(Canvas *c, int index) {

  size_t size = c->layer_list->size;
  if (index >= size) {
    printf("out of bounds!\n");
    return 1;
  }

  get_layer(c, index)->visible = 0;
  return 0;
}

int show_layer(Canvas *c, int index) {
  size_t size = c->layer_list->size;
  if (index >= size || index < 0) {
    printf("out of bounds!\n");
    return 1;
  }

  get_layer(c, index)->visible = 1;
  return 0;
}

int change_layer(Canvas *c, int index) {
  size_t size = c->layer_list->size;
  if (index >= size || index < 0) {
    printf("out of bounds!\n");
    return 1;
  }

  c->layer_index = index;

  return 0;
}

int **make_2darray(int width, int height) {
  int *tmp = (int*)malloc(width * height * sizeof(int));
  memset(tmp, 0, width * height * sizeof(int));
  int **array = malloc(width * sizeof(int*));
  for (int i = 0 ; i < width ; i++){
    array[i] = tmp + i * height;
  }
  return array;
}

int **copy_2darray(int width, int height, int **array) {
  int **new_array = make_2darray(width, height);
  for (int x=0; x<width; x++) {
    for (int y=0; y<height; y++) {
      new_array[x][y] = array[x][y];
    }
  }

  return new_array;
}

void copy_board(int width, int height, int **board, int **color, int **bgcolor, Layer *layer) {
  for (int x=0; x<width; x++) {
    for (int y=0; y<height; y++) {
      board[x][y] = layer->board[x][y];
      color[x][y] = layer->color[x][y];
      bgcolor[x][y] = layer->bgcolor[x][y];
    }
  }
}

Layer *construct_layer(int width, int height) {
  Layer *layer = (Layer*)malloc(sizeof(Layer));
  layer->next = NULL;
  layer->prev = NULL;
  layer->visible = 1;
  layer->clipped = 0;
  layer->board = make_2darray(width, height);
  layer->color = make_2darray(width, height);
  layer->bgcolor = make_2darray(width, height);

  return layer;
}

int resize_layer(Canvas *c, int index, int width, int height) {

  int old_width = c->width;
  int old_height = c->height;

  int **new_board = make_2darray(width, height);
  int **new_color = make_2darray(width, height);
  int **new_bgcolor = make_2darray(width, height);

  Layer *layer = get_layer(c, index);

  for (int x=0; x<width; x++) {
    for (int y=0; y<height; y++) {

      if (x >= old_width || y >= old_height) {
        new_board[x][y] = 0;
        new_color[x][y] = 0;
        new_bgcolor[x][y] = 0;
      } else {
        new_board[x][y] = layer->board[x][y];
        new_color[x][y] = layer->color[x][y];
        new_bgcolor[x][y] = layer->bgcolor[x][y];
      }

    }
  }

  free(layer->board);
  free(layer->color);
  free(layer->bgcolor);

  layer->board = new_board;
  layer->color = new_color;
  layer->bgcolor = new_bgcolor;

  return 0;
}

int copy_layer(Canvas *c, int index) {

  size_t size = c->layer_list->size;
  if (index >= size || index < 0) {
    printf("out of bounds!\n");
    return 1;
  }

  const int width = c->width;
  const int height = c->height;

  Layer *layer = get_layer(c, index);
  Layer *new_layer = construct_layer(width, height);

  for (int x=0; x<width; x++) {
    for (int y=0; y<height; y++) {
      new_layer->board[x][y] = layer->board[x][y];
      new_layer->color[x][y] = layer->color[x][y];
      new_layer->bgcolor[x][y] = layer->bgcolor[x][y];
    }
  }

  new_layer->visible = 1;
  new_layer->clipped = layer->clipped;

  insert_layer(c, c->layer_list->size, new_layer);

  return 0;
}

int merge_layers(Canvas *c, int len, int indices[]) {

  // バブルソート
  for (int i=len; i>0; i--) {
    for (int j=0; j<i-1; j++) {
      if (indices[j] > indices[j+1]) {
        int temp = indices[j];
        indices[j] = indices[j+1];
        indices[j+1] = temp;
      }
    }
  }

  size_t size = c->layer_list->size;
  if (indices[len-1] >= size || indices[0] < 0) {
    printf("out of bounds!\n");
    return 1;
  }

  // 一番下のレイヤーに上のレイヤーを上書きしていく
  Layer *base_layer = get_layer(c, indices[0]);
  for (int x=0; x<c->width; x++) {
    for (int y=0; y<c->height; y++) {
      for (int i=1; i<len; i++) {
        Layer *layer = get_layer(c, indices[i]);
        if (layer->board[x][y] != 0) {
          base_layer->board[x][y] = layer->board[x][y];
          base_layer->color[x][y] = layer->color[x][y];
          if (layer->bgcolor[x][y] != 0) {
            base_layer->bgcolor[x][y] = layer->bgcolor[x][y];
          }
        }
      }
    }
  }

  // 一番下以外のレイヤーを削除
  for (int i=1; i<len; i++) {
    int index = indices[i] - (i-1); // 下のレイヤーが削除されるたびに上のレイヤーの番号が下がっていく
    remove_layer(c, index, 1); // freeもする
  }

  return 0;
}

int merge_layer(Canvas *c, int index) {

  size_t size = c->layer_list->size;
  if (index > size || index < 0) {
    printf("out of bounds!\n");
    return 1;
  }

  // 一番下のレイヤーに上のレイヤーを上書きしていく
  Layer *layer = get_layer(c, index);
  Layer *base_layer = layer->prev;

  if (base_layer == NULL) {
    printf("none!\n");
    return 1;
  }

  // 自身がクリッピングされていて、かつすぐ下のレイヤーがクリッピング元である場合
  // それ以外の場合はクリッピングを無視して結合する
  // - 両方クリッピングされていない場合
  // - 下のレイヤーのみクリッピングされている場合
  // - 両方がさらに下のレイヤーにクリッピングされている場合
  int clipped = (layer->clipped && !base_layer->clipped);

  for (int x=0; x<c->width; x++) {
    for (int y=0; y<c->height; y++) {

      // 何も書かれていない
      if (layer->board[x][y] == 0)
        continue;

      // クリッピング元に何も書かれていない
      if (clipped && base_layer->board[x][y] == 0)
        continue;

      base_layer->board[x][y] = layer->board[x][y];
      base_layer->color[x][y] = layer->color[x][y];

      if (layer->bgcolor[x][y] != 0) {
        base_layer->bgcolor[x][y] = layer->bgcolor[x][y];
      }

    }
  }

  remove_layer(c, index, 1); //freeもする

  return 0;
}

void add_layer(Canvas *c) {
  Layer *last = get_last_layer(c);
  assert(last != NULL);

  Layer *new = construct_layer(c->width, c->height);

  last->next = new;
  new->prev = last;

  c->layer_list->size++;
}

int insert_layer(Canvas *c, int index, Layer *layer) {

  // index = size はOK
  size_t size = c->layer_list->size;
  if (index > size || index < 0) {
    printf("out of bounds!\n");
    return 1;
  }

  if (index == c->layer_list->size) { // 最後に移動する場合
    Layer *last = get_last_layer(c);
    last->next = layer;

    layer->prev = last;
    layer->next = NULL;
  } else {
    Layer *next_layer = get_layer(c, index);

    // 前のレイヤーと接続
    if (index == 0) {
      c->layer_list->begin = layer;
      layer->prev = NULL;
    } else {
      next_layer->prev->next = layer;
      layer->prev = next_layer->prev;
    }

    // 後ろのレイヤーと接続
    next_layer->prev = layer;
    layer->next = next_layer;
  }

  c->layer_list->size++;

  return 0;
}

int remove_layer(Canvas *c, int index, int freeing) {

  size_t size = c->layer_list->size;
  if (index >= size || index < 0) {
    printf("out of bounds!\n");
    return 1;
  }
  
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

  if (freeing) { // 一時的にリストから切り離すときはメモリを解放しない
    free_layer(layer);
  }

  c->layer_list->size--;

  if (c->layer_index >= c->layer_list->size) { // 最後のレイヤーを消した場合はその一つ前のレイヤーを表示する
    change_layer(c, c->layer_list->size - 1);
  }

  return 0;
}

int move_layer(Canvas *c, int a, int b) {

  size_t size = c->layer_list->size;
  if (a >= size || b >= size || a < 0 || b < 0) {
    printf("out of bounds!\n");
    return 1;
  }
  
  Layer *cur_layer = get_cur_layer(c);
  Layer *layer = get_layer(c, a);

  // 一度リストから切り離し挿入する
  remove_layer(c, a, 0); // freeはしない
  insert_layer(c, b, layer);

  // 現在表示してるレイヤーがずれないように修正
  for (int i=0; i<c->layer_list->size; i++) {
    if (get_layer(c, i) == cur_layer) {
      c->layer_index = i;
    }
  }

  return 0;
}

int clip_layer(Canvas *c, int index, int flag) {

  // b = -1の場合はクリッピングを解除するのでエラーにしない
  size_t size = c->layer_list->size;
  if (index >= size || index < 0) {
    printf("out of bounds!\n");
    return 1;
  }

  get_layer(c, index)->clipped = flag;

  return 0;
}

void free_2darray(int **array) {
  if (array == NULL)
    return;
  free(array[0]);
  free(array);
}

void free_board(Layer *layer) {
  free_2darray(layer->board);
  free_2darray(layer->color);
  free_2darray(layer->bgcolor);
}

void free_layer(Layer *layer) {
  free_board(layer);
  free(layer);
}

void free_all_layers(Canvas *c) {
  Layer *layer = c->layer_list->begin;

  while(layer != NULL) {
    Layer *temp = layer;
    layer = layer->next;

    free_layer(temp);
  }
}