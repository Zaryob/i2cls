/* Canonical manufacturer metadata; chip signatures live in i2c_db.c. */
#include <strings.h>
#include "i2c_vendor.h"

static const i2c_vendor_info_t vendors[] = {
    { VENDOR_BOSCH, "bosch", "Bosch Sensortec", { "Bosch", NULL } },
    { VENDOR_TDK, "tdk", "TDK InvenSense", { "TDK", "InvenSense", NULL } },
    { VENDOR_ST, "st", "STMicroelectronics", { "STM", "ST Microelectronics", NULL } },
    { VENDOR_ADI, "adi", "Analog Devices", { "Analog", NULL } },
    { VENDOR_MAXIM, "maxim", "Maxim Integrated", { "Maxim", NULL } },
    { VENDOR_TI, "ti", "Texas Instruments", { "Texas", NULL } },
    { VENDOR_SOLOMON, "solomon", "Solomon Systech", { "Solomon", NULL } },
    { VENDOR_SINO_WEALTH, "sino-wealth", "Sino Wealth", { "Sino", NULL } },
    { VENDOR_SENSIRION, "sensirion", "Sensirion", { NULL } },
    { VENDOR_ASAIR, "asair", "ASAIR", { "Aosong", NULL } },
    { VENDOR_BROADCOM, "broadcom", "Broadcom / Avago", { "Broadcom", "Avago", NULL } },
    { VENDOR_MICROCHIP, "microchip", "Microchip", { "Atmel", "Atmel / Microchip", "Microchip / Atmel", NULL } }
};

size_t i2c_vendor_count(void)
{
    return sizeof(vendors) / sizeof(vendors[0]);
}

const i2c_vendor_info_t *i2c_vendor_at(size_t index)
{
    return index < i2c_vendor_count() ? &vendors[index] : NULL;
}

const i2c_vendor_info_t *i2c_vendor_get(i2c_vendor_id_t key)
{
    for (size_t i = 0; i < i2c_vendor_count(); i++)
        if (vendors[i].key == key) return &vendors[i];
    return NULL;
}

const i2c_vendor_info_t *i2c_vendor_find(const char *query)
{
    if (!query || !*query) return NULL;
    for (size_t i = 0; i < i2c_vendor_count(); i++) {
        const i2c_vendor_info_t *vendor = &vendors[i];
        if (!strcasecmp(query, vendor->id) || !strcasecmp(query, vendor->name)) return vendor;
        for (size_t a = 0; a < sizeof(vendor->aliases) / sizeof(vendor->aliases[0]) && vendor->aliases[a]; a++)
            if (!strcasecmp(query, vendor->aliases[a])) return vendor;
    }
    return NULL;
}
