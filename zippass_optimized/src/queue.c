// Copyright 2023 Archibald Emmanuel Carrion Claeys

#include <stdlib.h>
#include <string.h>
#include "queue.h"

queue_t* queue_create() {
  queue_t* queue = malloc(sizeof(queue_t));
  if (queue == NULL) {
    return NULL;
  }
  queue->front = NULL;
  queue->rear = NULL;
  queue->size = 0;
  queue->finished = 0;
  return queue;
}

void queue_destroy(queue_t* queue) {
  while (!is_empty(queue)) {
    dequeue(queue);
  }
  free(queue);
}

int is_empty(queue_t* queue) {
  return queue->size == 0;
}

void enqueue(queue_t* queue, char* data) {
  queue_node_t* node = malloc(sizeof(queue_node_t));
  if (node == NULL) {
    return;
  }
  node->data = data;
  node->next = NULL;
  if (is_empty(queue)) {
    queue->front = node;
  } else {
    queue->rear->next = node;
  }
  queue->rear = node;
  queue->size++;
}

char* dequeue(queue_t* queue) {
  if (is_empty(queue)) {
    return NULL;
  }
  queue_node_t* node = queue->front;
  char* data = node->data;
  queue->front = node->next;
  if (queue->front == NULL) {
    queue->rear = NULL;
  }
  free(node);
  queue->size--;
  return data;
}

void queue_empty(queue_t* queue) {
  while (!is_empty(queue)) {
    char* data = dequeue(queue);
    free(data);
  }
}
