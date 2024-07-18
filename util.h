#ifndef UTIL_H
#define UTIL_H


#define INITIAL_SIZE 64
#define GENERIC_LIST(t) struct { int size; int capacity; t* data;  }
#define LIST_DEFAULT(t) { 0, INITIAL_SIZE, malloc(INITIAL_SIZE * sizeof(t)) }
#define LIST_INIT(l) (l)->size = 0; (l)->capacity = INITIAL_SIZE; \
  (l)->data = malloc(INITIAL_SIZE * sizeof l->data);
#define LIST_APPEND(l, val)     \
  if ((l)->size == (l)->capacity) \
    (l)->data = reallocarray((l)->data, (l)->capacity *= 2, sizeof val); \
  (l)->data[(l)->size++] = val;
#define LIST_DESTROY(l) free((l)->data); free((l));


void test_list() {
  GENERIC_LIST(char) str = LIST_DEFAULT(char*);
  LIST_APPEND(&str, 'a');
  LIST_APPEND(&str, 'b');
  printf("%d\n", str.size);
  for (int i = 0; i < str.size; i++) {
    printf("%c ", str.data[i]);
  }
  printf("\n");
  LIST_APPEND(&str, 'c');
  printf("%d\n", str.capacity);
  printf("%d\n", str.size);
  for (int i = 0; i < str.size; i++) {
    printf("%c ", str.data[i]);
  }
  printf("\n");

  GENERIC_LIST(int) list = LIST_DEFAULT(int);
  LIST_APPEND(&list, 1);
  LIST_APPEND(&list, 2);
  LIST_APPEND(&list, 3);
  LIST_APPEND(&list, 4);
  LIST_APPEND(&list, 5);
  for (int i = 0; i < list.size; i++) {
    printf("%d ", list.data[i]);
  }
  printf("\n");
}
  


const int M = 1e5;

long long ipow(int base, int exp) {
  long long res = 1;
  while (1) {
    if (exp % 2) {
      res *= base;
    }
    exp /= 2;
    if (!exp) {
      break;
    }
    base *= base;
    base %= M;
  }
  return res;
}

int hash(const char* str) {
  const int P = 37;
  long long hash = 0;
  for (int i = 0; i < strlen(str); i++) {
    hash += str[i] * ipow(P, i);
    hash %= M;
  }
  return (int) hash;
}


typedef struct ht_entry {
  const char* key;
  int val;
  struct ht_entry* next;
} ht_entry;

typedef struct {
  ht_entry** entries;
} hashtable;

hashtable* ht_new() {
  hashtable* ht = malloc(sizeof(hashtable));
  ht->entries = malloc(M * sizeof(ht_entry*));
  memset(ht->entries, 0, M * sizeof(ht_entry*));
  return ht;
}


void ht_set(hashtable* ht, const char* key, const int val) {
  int hashed = hash(key);
  ht_entry* cur = ht->entries[hashed];
  if (!cur) {
    ht->entries[hashed] = malloc(sizeof(ht_entry*));
    ht->entries[hashed]->key = key;
    ht->entries[hashed]->val = val;
    return;
  }

  while (!cur->next) {
    cur = cur->next;
  }
  cur->next = malloc(sizeof(ht_entry));
  cur->next->key = key;
  cur->next->val = val;
}

ht_entry* ht_get(hashtable* ht, const char* key) {
  int hashed = hash(key);
  ht_entry* cur = ht->entries[hashed];
  while (cur && strcmp(key, cur->key)) {
    cur = cur->next;
  }
  return cur;
}
#endif
