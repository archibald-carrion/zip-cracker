// Copyright 2023 Archibald Emmanuel Carrion Claeys

#ifndef QUEUE_H
#define QUEUE_H

/// @brief queue_node_t is a struct used to store the data of a node of a queue.
typedef struct queue_node {
  char* data;
  // data is a string that stores the password to try
  struct queue_node* next;
  // next is a pointer to the next node in the queue
} queue_node_t;

typedef struct queue {
  queue_node_t* front;
  queue_node_t* rear;
  unsigned int size;
  int finished;
} queue_t;

/// @brief queue_create creates a queue.
/// @return the function returns a pointer to the created queue.
queue_t* queue_create();

/// @brief queue_destroy destroys a queue.
/// @param queue is a pointer to the queue to be destroyed.
void queue_destroy(queue_t* queue);

/// @brief is_empty checks if a queue is empty.
/// @param queue is a pointer to the queue to be checked.
/// @return the function returns 1 if the queue is empty, otherwise it returns
/// 0.
int is_empty(queue_t* queue);

/// @brief enqueue adds a new node to the queue.
/// @param queue is a pointer to the queue where the node will be added.
/// @param data is a string that stores the password to try.
void enqueue(queue_t* queue, char* data);

/// @brief dequeue removes the first node of the queue.
/// @param queue is a pointer to the queue where the node will be removed.
/// @return the function returns a pointer to the string stored in the removed
char* dequeue(queue_t* queue);

/// @brief queue_empty removes all the nodes of the queue.
/// @param queue is a pointer to the queue to be emptied.
void queue_empty(queue_t* queue);

#endif /* QUEUE_H */
