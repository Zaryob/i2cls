#ifndef I2C_VENDOR_H
#define I2C_VENDOR_H

#include <stddef.h>

/* Numeric values are internal; use the catalog's string IDs in saved data. */
typedef enum {
    VENDOR_UNKNOWN = 0,
    VENDOR_BOSCH,
    VENDOR_TDK,
    VENDOR_ST,
    VENDOR_ADI,
    VENDOR_MAXIM,
    VENDOR_TI,
    VENDOR_SOLOMON,
    VENDOR_SINO_WEALTH,
    VENDOR_SENSIRION,
    VENDOR_ASAIR,
    VENDOR_BROADCOM,
    VENDOR_MICROCHIP,
    VENDOR_COUNT
} i2c_vendor_id_t;

typedef struct {
    i2c_vendor_id_t key;
    const char *id;
    const char *name;
    const char *aliases[8]; /* NULL-terminated names, not chip or driver aliases. */
} i2c_vendor_info_t;

size_t i2c_vendor_count(void);
const i2c_vendor_info_t *i2c_vendor_get(i2c_vendor_id_t key);
const i2c_vendor_info_t *i2c_vendor_at(size_t index);
const i2c_vendor_info_t *i2c_vendor_find(const char *query);

#endif
