
#ifndef BENCODE_H_
#define BENCODE_H_
#pragma once

typedef struct
{
    const char *str;
    const char *start;
    void *parent;
    int val;
    int len;
} bencode_t;

/**
* Initialise a bencode object.
* @param be The bencode object
* @param str Buffer we expect input from
* @param len Length of buffer
*/
void bencode_init(
    bencode_t * be,
    const char *str,
    int len
);

/**
* @return 1 if the bencode object is an int; otherwise 0.
*/
int bencode_is_int(
    const bencode_t * be
);

/**
* @return 1 if the bencode object is a string; otherwise 0.
*/
int bencode_is_string(
    const bencode_t * be
);

/**
* @return 1 if the bencode object is a list; otherwise 0.
*/
int bencode_is_list(
    const bencode_t * be
);

/**
* @return 1 if the bencode object is a dict; otherwise 0.
*/
int bencode_is_dict(
    const bencode_t * be
);

/**
* Obtain value from integer bencode object.
* @param val Long int we are writing the result to
* @return 1 on success, otherwise 0
*/
int bencode_int_value(
    bencode_t * be,
    long int *val
);

/**
* @return 1 if there is another item on this dict; otherwise 0.
*/
int bencode_dict_has_next(
    bencode_t * be
);

/**
* Get the next item within this dictionary.
* @param be_item Next item.
* @param key Const pointer to key string of next item.
* @param klen Length of the key of next item.
* @return 1 on success; otherwise 0.
*/
int bencode_dict_get_next(
    bencode_t * be,
    bencode_t * be_item,
    const char **key,
    int *klen
);

/**
* Get the string value from this bencode object.
* The buffer returned is stored on the stack.
* @param be The bencode object.
* @param str Const pointer to the buffer.
* @param slen Length of the buffer we are outputting.
* @return 1 on success; otherwise 0
*/
int bencode_string_value(
    bencode_t * be,
    const char **str,
    int *len
);

/**
* Tell if there is another item within this list.
* @param be The bencode object
* @return 1 if another item exists on the list; 0 otherwise; -1 on invalid processing
*/
int bencode_list_has_next(
    bencode_t * be
);

/**
* Get the next item within this list.
* @param be The bencode object
* @param be_item The next bencode object that we are going to initiate.
* @return return 0 on end; 1 on have next; -1 on error
*/
int bencode_list_get_next(
    bencode_t * be,
    bencode_t * be_item
);

/**
 * Copy bencode object into other bencode object
 */
void bencode_clone(
    bencode_t * be,
    bencode_t * output
);

/**
* Get the start and end position of this dictionary
* @param be Bencode object
* @param start Starting string
* @param len Length of the dictionary 
* @return 1 on success
*/
int bencode_dict_get_start_and_len(
    bencode_t * be,
    const char **start,
    int *len
);

#endif /* BENCODE_H_ */
