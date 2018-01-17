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
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <signal.h>
#include <assert.h>
#include <strings.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "hashtable.h"

typedef size_t (*hash_func_cb)(void *key);
typedef int (*cmp_keys_cb)(void *key1, void *key2);
typedef void *(*key_for_data_cb)(void *data);
typedef void (*iterator_cb)(void *data);
typedef void (*extractor_cb)(void *data);

typedef struct HashNode
{
   size_t hash;
   void *key;
   void *data;
   struct HashNode *next;
} *HashNodePtr;

/* A structure which contains information regarding the state of the event handler */
struct HashTable
{
   int hashSize;				    	                        /* The hash size of the event handler */
   HASH_hash_func_cb hashFunc;
   HASH_cmp_keys_cb keyCmp;
   HASH_key_for_data_cb keyForData;

   HashNodePtr buckets[1];				                      /* List of pointers to event callbacks */
};

struct HashTable *HASH_create_table(int hashSize, HASH_hash_func_cb hashFunc,
      HASH_cmp_keys_cb keyCmp, HASH_key_for_data_cb keyForData)
{
   struct HashTable *res = NULL;
   int i;

   res = (struct HashTable*)malloc(
         sizeof(struct HashTable) + hashSize * sizeof(HashNodePtr));
   if (!res)
      return NULL;

   res->hashSize = hashSize;
   res->hashFunc = hashFunc;
   res->keyCmp = keyCmp;
   res->keyForData = keyForData;
   for (i = 0; i < res->hashSize; i++)
      res->buckets[i] = NULL;

   return res;
}

static struct HashNode **HASH_search_internal(struct HashTable *table, void *key)
{
   size_t hash;
   struct HashNode **curr;

   hash = (*table->hashFunc)(key);
   for( curr = &table->buckets[hash % table->hashSize];
         *curr; curr = &((*curr)->next) ) {
      if ((*curr)->key && (*table->keyCmp)((*curr)->key, key))
         return curr;
   }

   return NULL;
}

static void *HASH_remove_key_internal(struct HashTable *table,
      void *key)
{
   struct HashNode **nodePtr = HASH_search_internal(table, key);
   struct HashNode *node;
   void *res;

   if (!nodePtr)
      return NULL;

   node = *nodePtr;
   if (!nodePtr)
      return NULL;

   *nodePtr = node->next;
   res = node->data;
   free(node);

   return res;
}

void *HASH_remove_key(struct HashTable *table, void *key)
{
   if (!table || !key)
      return NULL;

   return HASH_remove_key_internal(table, key);
}

void *HASH_remove_data(struct HashTable *table, void *data)
{
   void *key;

   if (!table || !data)
      return NULL;

   key = (*table->keyForData)(data);
   return HASH_remove_key_internal(table, key);
}

void HASH_free_table(struct HashTable *table)
{
   int i;

   if (!table)
      return;

   for(i = 0; i < table->hashSize; i++)
      while(table->buckets[i])
         HASH_remove_key(table, table->buckets[i]->key);

   free(table);
}

void *HASH_find_key(struct HashTable *table, void *key)
{
   size_t hash;
   struct HashNode *curr;

   if (!table || !key)
      return NULL;

   hash = (*table->hashFunc)(key);
   for( curr = table->buckets[hash % table->hashSize];
         curr; curr = curr->next) {
      if (curr->key && (*table->keyCmp)(curr->key, key))
         return curr->data;
   }

   return NULL;
}

void *HASH_find_data(struct HashTable *table, void *data)
{
   void *key;

   if (!table || !data)
      return NULL;

   key = (*table->keyForData)(data);
   return HASH_find_key(table, key);
}

int HASH_add_data(struct HashTable *table, void *data)
{
   struct HashNode *curr;
   void *key;

   if (!data)
      return -1;

   key = (*table->keyForData)(data);
   if (!key)
      return -2;

   if (HASH_find_key(table, key))
      return -3;

   curr = (struct HashNode*)malloc(sizeof(struct HashNode));
   if (!curr)
      return -4;

   curr->key = key;
   curr->data = data;
   curr->hash = (*table->hashFunc)(key);
   curr->next = table->buckets[curr->hash % table->hashSize];
   table->buckets[curr->hash % table->hashSize] = curr;

   return 0;
}

void HASH_iterate_table(struct HashTable *table, HASH_iterator_cb iterator)
{
   int i;
   if (!table)
      return;

   struct HashNode **ptr;
   struct HashNode *goner;
   for(i = 0; i < table->hashSize; i++)
   {
      ptr = &table->buckets[i];
      while(ptr && *ptr)
      {
         if ((*iterator)((*ptr)->data)) {
            goner = *ptr;
            *ptr = goner->next;
            goner->next = NULL;
            free(goner);
         }
         else
            ptr = &(*ptr)->next;
      }
   }
}

void HASH_extract(struct HashTable *table, HASH_extractor_cb extractor)
{
   int i;

   if (!table)
      return;

   for(i = 0; i < table->hashSize; i++)
      while(table->buckets[i])
         (*extractor)(HASH_remove_key(table, table->buckets[i]->key));
}

