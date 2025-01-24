#include <esp_system.h>
#include <stddef.h>
#include <stdint.h>

extern void esp_fill_random(void *buf, size_t len);

void randombytes(uint8_t *output, size_t len);
void PQCLEAN_randombytes(uint8_t *output, size_t len);

void randombytes(uint8_t *output, size_t len) {
    esp_fill_random(output, len); // Use ESP32's hardware RNG
}

void PQCLEAN_randombytes(uint8_t *output, size_t len) {

    randombytes(output, len);
}
