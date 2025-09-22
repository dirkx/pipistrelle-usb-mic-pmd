#ifndef _H_RECEIVER
#define _H_RECEIVER

#ifdef __cplusplus
extern "C" {
#endif

void init_pdm_receiver(void(*check)(uint8_t * buff, size_t len),
	int pin_dat, int pin_clk, size_t len, uint Q);
void receiver_clock_start(uint pin_clk, uint64_t sampleBitRate);

#ifdef __cplusplus
}
#endif
#endif

