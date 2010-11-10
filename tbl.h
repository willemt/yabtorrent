#ifndef TBL_H
#define TBL_H

#include <stddef.h> /* size_t */

#ifdef __cplusplus
extern C {
#endif

enum tbl_error {
  TBL_E_NONE,
  TBL_E_INVALID_DATA,
  TBL_E_CANCELED_BY_USER,
  TBL_E_NO_CALLBACKS,
  TBL_E_UNKNOWN
};

/* parsing is stopped if a callback returns something else than 0 */
struct tbl_callbacks {
  int (*integer)   (void *ctx, long long value);
  int (*string)    (void *ctx, char *value, size_t length);

  int (*list_start)(void *ctx);
  int (*list_end)  (void *ctx);

  int (*dict_start)(void *ctx);
  int (*dict_key)  (void *ctx, char *key, size_t length);
  int (*dict_end)  (void *ctx);
};

int tbl_parse(const char *buf, size_t lenght,
              const struct tbl_callbacks *callbacks, void *ctx);

/* only prototypes; nothing of this is implemented yet */
/*
int tbl_gen_integer(tbl_handle_t *handle, long value);
int tbl_gen_string(tbl_handle_t *handle, const char *str, size_t len);
int tbl_gen_dict_open(tbl_handle_t *handle);
int tbl_gen_dict_close(tbl_handle_t *handle);
int tbl_gen_list_open(tbl_handle_t *handle);
int tbl_gen_list_close(tbl_handle_t *handle);
*/

#ifdef __cplusplus
}
#endif

#endif /* TBL_H */
