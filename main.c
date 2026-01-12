//
// Created by marijn on 10/14/25.
//
#include <libpynq.h>
#include <time.h>



#define HEARTBEAT_SM_ADDR        0x52U

#define HEARTBEAT_SM_RMAP_SIZE    0x01U
uint32_t rmap[HEARTBEAT_SM_RMAP_SIZE] = {
    0,        // heart rate
};


#define HEARTBEAT_SM_HEARTRATE_REG    0x00U


display_t display;
FontxFile fx;
__clock_t prev;


void write_to_screen(uint32_t heart_rate) {
    // if (clock() - prev < 100000) { return; }
    // printf("clk %ld, %ld, %ld\n", clock(), prev, clock() - prev);
    // prev = clock();

    // Clear display
    displayFillScreen(&display, RGB_BLACK);

    int y = 20;
    // Crying level
    char heart_string[50];
    snprintf(heart_string, sizeof(heart_string), "heart rate: %lu", (unsigned long)heart_rate);
    displayDrawString(&display, &fx, 10, y, (uint8_t *)heart_string, RGB_GREEN);
}



int main(void) {
    // init
    pynq_init();
    gpio_init();
    gpio_reset();
    switchbox_init();
    switchbox_reset();

    InitFontx(&fx, "../../fonts/ILGH24XB.FNT", "");
    display_init(&display);
    
    // pins
    switchbox_set_pin(IO_AR_SCL, SWB_IIC0_SCL);
    switchbox_set_pin(IO_AR_SDA, SWB_IIC0_SDA);

    // interrupt? (this is does not really constitute interupt ability...)
    gpio_interrupt_init();
    gpio_enable_interrupt(IO_AR6);
    verify_interrupt_request(IO_AR6);
    gpio_ack_interrupt();
    uint64_t irqmsk;
    // NOTE: why cant it call a func, why cant i ack specific irqs, why cant i set the edge direction, why is this just a slightly fancier way of reading a pin????

    // I2C
    iic_init(IIC0);
    iic_reset(IIC0);
    iic_set_slave_mode(IIC0, HEARTBEAT_SM_ADDR, &(rmap[0]), HEARTBEAT_SM_RMAP_SIZE);

    prev = clock();
    uint32_t pulsecnt = 0;

    // loop
    for (;;) {
        // handle I2C
        iic_slave_mode_handler(IIC0);
        sleep_msec(5);
        
        // sample sensor
        irqmsk = gpio_get_interrupt();
        if (irqmsk && (1 << IO_AR6)) {
            pulsecnt++;
            gpio_ack_interrupt();
        }
        
        
        if (clock() - prev > (5 * 500000)) {
            printf("clk %ld, %ld, %ld\n", clock(), prev, clock() - prev);
            rmap[0] = pulsecnt * 12;
            pulsecnt = 0;
            prev = clock();
        }

        write_to_screen(rmap[0]);
    }


    // return
    iic_destroy(IIC0);
    pynq_destroy();
    return EXIT_SUCCESS;
}