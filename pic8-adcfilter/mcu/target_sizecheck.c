/* Cross-compile sizecheck for pic8-adcfilter: a minimal main that exercises
 * both the oversampler and the moving-average filter, so the XC8 build links
 * the real code and reports flash/RAM footprint. No HAL, no config word. */
#include "pic8_adcfilter.h"
#include <stddef.h>     /* NULL */

static uint16_t stub_read(void *ctx) { (void)ctx; return 512u; }

void main(void)
{
    uint16_t r = pic8_adcfilter_oversample(stub_read, NULL, 2);
    uint16_t buf[4];
    pic8_adcfilter_avg_t f;
    pic8_adcfilter_avg_init(&f, buf, 4);
    pic8_adcfilter_avg_push(&f, r);
    while (1) { }
}