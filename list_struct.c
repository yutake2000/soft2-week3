/*
  struct Listを使うバージョン
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

const int maxlen = 1000;

typedef struct node Node;

struct node 
{
  char *str;
  Node *next;
};

typedef struct list
{
  Node *begin;
} List;


void push_front(List *list, const char *str)
{

  Node *p = (Node *)malloc(sizeof(Node));
  char *s = (char *)malloc(strlen(str) + 1);
  strcpy(s, str);

  *p = (Node){.str = s , .next = list->begin};

  list->begin = p; 
}


void pop_front(List *list)
{
  assert(list->begin != NULL);
  Node *p = list->begin->next;

  free(list->begin->str);
  free(list->begin);

  list->begin = p;
}

void push_back(List *list, const char *str)
{
  if (list->begin == NULL) { 
    push_front(list, str);
    return;
  }

  Node *p = list->begin;
  while (p->next != NULL) {
    p = p->next;
  }

  Node *q = (Node *)malloc(sizeof(Node));
  char *s = (char *)malloc(strlen(str) + 1);
  strcpy(s, str);

  *q = (Node){.str = s, .next = NULL};
  p->next = q;
}

void pop_back(List *list)
{
  assert(list->begin != NULL);

  // 要素が1つしかない場合は先頭を削除するのと同じ
  if (list->begin->next == NULL) {
    pop_front(list);
    return;
  }

  // 最後から2番目の要素を取得
  Node *p = list->begin;
  while (p->next->next != NULL) {
    p = p->next;
  }

  free(p->next->str);
  free(p->next);
  
  // 最後から2番目の要素を最後の要素に変更
  p->next = NULL;
}


void remove_all(List *list)
{
  while (list->begin != NULL) pop_front(list);
  //while (list->begin != NULL) pop_back(list);
}

int main()
{
  Node *begin = NULL; 
  List *list = (List*)malloc(sizeof(List));

  char buf[maxlen];
  while (fgets(buf, maxlen, stdin)) {
    //push_front(list, buf);
    push_back(list, buf);
  }

  pop_front(list);
  pop_back(list);

  remove_all(list);

  // assert check
  //pop_front(list);
  //pop_back(list);

  for (const Node *p = list->begin; p != NULL; p = p->next) {
    printf("%s", p->str);
  }
  
  return EXIT_SUCCESS;
}
