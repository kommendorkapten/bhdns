/*
* Copyright (C) 2016 Fredrik Skogman, skogman - at - gmail.com.
* This file is part of libeds.
*
* The contents of this file are subject to the terms of the Common
* Development and Distribution License (the "License"). You may not use this
* file except in compliance with the License. You can obtain a copy of the
* License at http://opensource.org/licenses/CDDL-1.0. See the License for the
* specific language governing permissions and limitations under the License.
* When distributing the software, include this License Header Notice in each
* file and include the License file at http://opensource.org/licenses/CDDL-1.0.
*/

#ifndef __STACK_H__
#define __STACK_H__

#include <stddef.h>

struct stack;

#define STACK_AUTO_EXPAND 0x1

/**
 * Create a stack.
 * @param the initial capacity.
 * @param flags
 * @return the stack or NULL.
 */
struct stack* stack_create(size_t, unsigned int);

/**
 * Push an element on the stack.
 * @param the stack.
 * @param the element to push.
 * @return 0 if element was added to stack, -1 otherwise.
 */
int stack_push(struct stack*, void*);

/**
 * Pop an element from the stack.
 * @param the stack.
 * @return the element, or NULL if the stack is empty.
 */
void* stack_pop(struct stack*);

/**
 * Returns the number of elements on the stack.
 * @param the stack
 * @return number of elements.
 */
size_t stack_size(const struct stack*);

/**
 * Clears the element on the stack
 * @param the stack
 * @return void.
 */
void stack_clear(struct stack*);

/**
 * Destroy the stack and free any memory.
 * @param the stack to destroy.
 * @return void.
 */
void stack_destroy(struct stack*);
#endif /* __STACK_H__ */
