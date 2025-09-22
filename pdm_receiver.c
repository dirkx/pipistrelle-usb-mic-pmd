#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "pico/stdlib.h"

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "pico/time.h"

#include "hardware/pio.h"
#include "hardware/dma.h"

#include "pdm_receiver.pio.h"

static PIO pio;
static uint offset;
static uint sm;

#ifndef DMA_IRQ_PRIORITY
#define DMA_IRQ_PRIORITY PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY
#endif

#ifndef DMA_IRQ_TO_USE
#define DMA_IRQ_TO_USE 1 
#endif

static uint dma_channel_rx;

static void* aligned_malloc(size_t required_bytes, size_t alignment)
{
    void* p1; // original block
    void** p2; // aligned block
    int offset = alignment - 1 + sizeof(void*);
    if ((p1 = (void*)malloc(required_bytes + offset)) == NULL)
    {
       return NULL;
    }
    p2 = (void**)(((size_t)(p1) + offset) & ~(alignment - 1));
    p2[-1] = p1;
    return p2;
}

static void aligned_free(void *p)
{
    free(((void**)p)[-1]);
}

static uint32_t * buffer_rx;
static uint32_t * count_buffer;
static size_t n32words_rx;

void (*_received_cb)(uint8_t * buff, size_t len);

// DMA interrupt handler, called when a DMA channel has transmitted its data
//
uint32_t cnt;
static void dma_irq_handler() {
    dma_irqn_acknowledge_channel(DMA_IRQ_TO_USE, dma_channel_rx);

    uint8_t *p = (uint8_t *)count_buffer;
    for(int i = 0; i < n32words_rx; i++) {
        // We get one count per bytes; as a negative, to complement, 
	// number. So map these all to a positive count if they are
	// not 0
	count_buffer[i] = ~buffer_rx[i];
#if 0
	*p = (*p == 0xFF) ? 0 : (*p + 1); p++;
	*p = (*p == 0xFF) ? 0 : (*p + 1); p++;
	*p = (*p == 0xFF) ? 0 : (*p + 1); p++;
	*p = (*p == 0xFF) ? 0 : (*p + 1); p++;
#else
	*p++ = (*p) + 1;
	*p++ = (*p) + 1;
	*p++ = (*p) + 1;
	*p++ = (*p) + 1;
#endif
	// printf("%d/%d..+3:	%08x - %08x\n", i,i*4,count_buffer[i], buffer_rx[i]);
    };

    _received_cb((uint8_t *)count_buffer,4*n32words_rx);

  // Reset DMA process for next block from PIO FIFO
  dma_channel_set_write_addr(dma_channel_rx,buffer_rx,true);
}

void receiver_clock_start(uint pin_clk, uint64_t sampleBitRate) {
    // clock out
    //
    // On RP2040 valid GPIOs are 21, 23, 24, 25. These GPIOs are connected to the GPOUT0-3 clock
    // generators. On RP2350 valid GPIOs are 13, 15, 21, 23, 24, 25. GPIOs 13 and 21 are
    // connected to the GPOUT0 clock generator. GPIOs 15 and 23 are connected to the GPOUT1
    // clock generator. GPIOs 24 and 25 are connected to the GPOUT2-3 clock generators.
    //
    assert(pin_clk == 13);
    uint32_t freq = clock_get_hz (clk_usb);
    float usb_div = freq / sampleBitRate;

    clock_gpio_init(pin_clk, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_USB, usb_div);
}

void init_pdm_receiver(void (*received_cb)(uint8_t * buff, size_t len), uint pin_dat, uint pin_clk, size_t len, uint Q) {
printf("%s:%d DAT:%u CLK:%u\n",__FILE__,__LINE__, pin_dat, pin_clk);

    hard_assert(Q >= 1);
    hard_assert(Q <= 255); // max that fits in a byte (0 to 255 bits; 256 bits would not fit)
printf("%s:%d\n",__FILE__,__LINE__);
    // len - number of samples of Q bits
    //
    size_t nbits = Q * len;
printf("%s:%d\n",__FILE__,__LINE__);
    n32words_rx = len / 4; // Each count is 8 bits
    buffer_rx = aligned_malloc(4*n32words_rx, 4096); 
printf("%s:%d\n",__FILE__,__LINE__);
    hard_assert(buffer_rx);
printf("%s:%d\n",__FILE__,__LINE__);
    count_buffer = malloc(len); // every count is returned as a byte
    hard_assert(count_buffer);
printf("%s:%d\n",__FILE__,__LINE__);

    bzero(buffer_rx, 4 * n32words_rx);
printf("%s:%d\n",__FILE__,__LINE__);

    _received_cb = received_cb;
    bool pok = pio_claim_free_sm_and_add_program(&receiver_program, &pio, &sm, &offset);
    hard_assert(pok);
printf("%s:%d\n",__FILE__,__LINE__);

    pio_set_gpio_base(pio,0); // using 0 ..31
printf("%s:%d\n",__FILE__,__LINE__);
    receiver_program_init(pio, sm, offset, pin_dat, pin_clk, Q);
printf("%s:%d\n",__FILE__,__LINE__);

    irq_add_shared_handler(dma_get_irq_num(DMA_IRQ_TO_USE), dma_irq_handler, DMA_IRQ_PRIORITY);
printf("%s:%d\n",__FILE__,__LINE__);
    irq_set_enabled(dma_get_irq_num(DMA_IRQ_TO_USE), true);
printf("%s:%d\n",__FILE__,__LINE__);

    dma_channel_config config_rx;
    dma_channel_rx = dma_claim_unused_channel(true);
    config_rx = dma_channel_get_default_config(dma_channel_rx);
printf("%s:%d\n",__FILE__,__LINE__);

    channel_config_set_transfer_data_size(&config_rx, DMA_SIZE_32);
printf("%s:%d\n",__FILE__,__LINE__);

    channel_config_set_read_increment(&config_rx, false); // do not increment; FIFO
    channel_config_set_write_increment(&config_rx, true); 

    // Wire DMA to FIFO
    channel_config_set_dreq(&config_rx, pio_get_dreq(pio, sm, false /* is rx */));

printf("%s:%d\n",__FILE__,__LINE__);
#if 0
    channel_config_set_ring(
        &config_rx, 
        true, // Apply to write addresses.
        7 // bits for wrap around; buffer is 256 bytes; 32 transfer
     ); 
#endif

    printf("Transactions %d for %d bytes of %d bits = %d bits/block\n", n32words_rx, len, Q, nbits);

    // 8-bit read from the uppermost byte of the FIFO, as data is left-justified 
    // so need to add 3. Don't forget the cast!
    dma_channel_configure(
	dma_channel_rx, 
	&config_rx, 
	buffer_rx,  // write addr
	(io_rw_8*)&pio->rxf[sm], // read addr
	n32words_rx, // -- number of 32 bit word transactions
	false // dma started
    );

    dma_irqn_set_channel_enabled(DMA_IRQ_TO_USE, dma_channel_rx, true);

    pio_sm_set_enabled(pio, sm, true);

    dma_channel_start(dma_channel_rx);
}
