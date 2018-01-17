/*
 * Copyright PolySat, California Polytechnic State University, San Luis Obispo. cubesat@calpoly.edu
 * This file is part of libproc, a PolySat library.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef HASHTABLE_H
#define HASHTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef size_t (*HASH_hash_func_cb)(void *key);
typedef int (*HASH_cmp_keys_cb)(void *key1, void *key2);
typedef void *(*HASH_key_for_data_cb)(void *data);
typedef int (*HASH_iterator_cb)(void *data);
typedef void (*HASH_extractor_cb)(void *data);

struct HashTable;

/**
 * Initializes a HashTable with a given hash size and function pointers.
 *
 * @param hashSize The hash size of the event handler.
 * @param hashFunc A pointer to a function that takes a key and returns an int hash value.
 * @param keyCmp A pointer to a function that takes two keys and returns true if they are identical, false otherwise.
 * @param keyForData A pointer to a function that takes a data pointer and returns a pointer to the key.
 *
 * @return A pointer to the new EventState
 */
struct HashTable *HASH_create_table(int hashSize, HASH_hash_func_cb hashFunc,
            HASH_cmp_keys_cb keyCmp, HASH_key_for_data_cb keyForData);

void HASH_free_table(struct HashTable *table);

int HASH_add_data(struct HashTable *table, void *data);

void *HASH_find_key(struct HashTable *table, void *key);
void *HASH_find_data(struct HashTable *table, void *data);

void *HASH_remove_key(struct HashTable *table, void *key);
void *HASH_remove_data(struct HashTable *table, void *data);

void HASH_iterate_table(struct HashTable *table, HASH_iterator_cb iterator);
void HASH_extract(struct HashTable *table, HASH_extractor_cb extractor);

#ifdef __cplusplus
}
#endif

#endif
