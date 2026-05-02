#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Global variable shared with ISR - missing volatile */
uint8_t g_rx_ready = 0;

void UART_IRQHandler(void) {
    /* CRITICAL: malloc in ISR */
    char *data = (char *)malloc(128);
    
    if (data != NULL) {
        /* HIGH: printf in ISR */
        printf("UART Interrupt\n");
        
        g_rx_ready = 1;
        free(data);
    }
}

int process_data(uint8_t *buf, size_t len) {
    /* HIGH: memcpy without bounds check on len (assuming buf size is fixed) */
    static uint8_t local_cache[64];
    memcpy(local_cache, buf, len);
    
    /* MEDIUM: Deep nesting */
    if (len > 0) {
        if (buf[0] == 0xAA) {
            if (buf[1] == 0xBB) {
                if (buf[2] == 0xCC) {
                    /* Do something */
                }
            }
        }
    }
    
    /* HIGH: Unchecked return value */
    external_call();
    
    return 0;
}

void blocking_io(void) {
    /* HIGH: Blocking I/O without timeout */
    while (HAL_UART_GetState(&huart1) != HAL_UART_STATE_READY) {
        /* Infinite loop if UART hangs */
    }
}
