#include "xil_printf.h"
#include <ctype.h>


// Base addresses
#define AXI_GPIO_0_BASE_ADDR 0x40000000
#define AXI_GPIO_1_BASE_ADDR 0x40010000
#define TIMER_BASE_ADDR      0x41C00000

// GPIO channel offsets
#define GPIO_DATA_OFFSET   0
#define GPIO_TRI_OFFSET    1

// Green LEDs + buttons
#define GREEN_LEDS_BASE_ADDR (AXI_GPIO_0_BASE_ADDR)
#define PUSH_BTNS_BASE_ADDR  (AXI_GPIO_0_BASE_ADDR + 8)

#define GREEN_LEDS_REG   (unsigned *)(GREEN_LEDS_BASE_ADDR)
#define PUSH_BTNS_REG    (unsigned *)(PUSH_BTNS_BASE_ADDR)

// RGB LEDs (only red used)
#define RGB_LEDS_BASE_ADDR (AXI_GPIO_1_BASE_ADDR)
#define RGB_LEDS_REG   (unsigned *)(RGB_LEDS_BASE_ADDR)


// AXI Timer (Generate Mode)
#define TCSR0   (*(volatile unsigned *)(TIMER_BASE_ADDR + 0x00))
#define TLR0    (*(volatile unsigned *)(TIMER_BASE_ADDR + 0x04))
#define TCR0    (*(volatile unsigned *)(TIMER_BASE_ADDR + 0x08))

#define TCSR_LOAD   (1 << 5)   // load TLR into TCR
#define TCSR_ARHT   (1 << 4)   // auto reload
#define TCSR_UDT    (1 << 1)   // count down
#define TCSR_ENT    (1 << 7)   // enable timer
#define TCSR_T0INT  (1 << 8)   // interrupt flag

#define HALFSEC_COUNT 40623496   // ~0.5s

void timer_init_halfsec() {
    TCSR0 = 0x00000000;         // stop timer
    TLR0  = HALFSEC_COUNT;      // load 0.5s value
    TCSR0 = TCSR_LOAD | TCSR_ARHT | TCSR_UDT; // load into counter
    TCSR0 = TCSR_ARHT | TCSR_UDT | TCSR_ENT;  // start
}

int timer_expired() {
    if (TCSR0 & TCSR_T0INT) {
        TCSR0 |= TCSR_T0INT;  // clear flag
        return 1;
    }
    return 0;
}

void timer_stop() {
    TCSR0 = 0x00000000;  // disable timer
}


// FSM
typedef enum {
    GREEN,
    FLASH_RED_START,
    RED,
    FLASH_RED_END
} State;

State state, next_state;

int flashCounter = 0;
int flashOn = 0;

// Hardware control helpers
void setGreenLED(int on) {
    if (on) *GREEN_LEDS_REG = 0x1;
    else    *GREEN_LEDS_REG = 0x0;
}

void setRedLED(int on) {
    if (on) *RGB_LEDS_REG = 0x924;   // four red RGBs ON
    else    *RGB_LEDS_REG = 0x0;    // all OFF
}

static int anyButton(void) {
    return (*PUSH_BTNS_REG & 0x3) != 0;
}

// FSM tick 
void FSM_tick() {
    switch (state) {
        case GREEN:
            xil_printf("ENTER STATE: GREEN (timer off)\r\n");
            setGreenLED(1);
            setRedLED(0);
            timer_stop();

            if (anyButton()) {
                setGreenLED(0);
                flashCounter = 6;
                flashOn = 1;
                timer_init_halfsec();
                next_state = FLASH_RED_START;
            } else {
                next_state = GREEN;
            }
            break;

        case FLASH_RED_START:
            if (timer_expired()) {
                if (flashOn) {
                    xil_printf("FLASH_RED_START ON\r\n");
                    setRedLED(1);
                } else {
                    xil_printf("FLASH_RED_START OFF\r\n");
                    setRedLED(0);
                    flashCounter--; // one full cycle per OFF
                }
                flashOn = !flashOn;
            }

            if (flashCounter > 0) {
                next_state = FLASH_RED_START;
            } else {
                next_state = RED;
            }
            break;

        case RED: {
            static int redTicks = 0;
            if (redTicks == 0) {
                xil_printf("ENTER STATE: RED (solid)\r\n");
                setRedLED(1);
                redTicks = 8;   // 4s / 0.5s = 8 ticks
                timer_init_halfsec();
            }

            if (timer_expired()) {
                redTicks--;
            }

            if (redTicks > 0) {
                next_state = RED;
            } else {
                flashCounter = 6;
                flashOn = 1;
                next_state = FLASH_RED_END;
            }
            break;
        }

        case FLASH_RED_END:
            if (timer_expired()) {
                if (flashOn) {
                    xil_printf("FLASH_RED_END ON\r\n");
                    setRedLED(1);
                } else {
                    xil_printf("FLASH_RED_END OFF\r\n");
                    setRedLED(0);
                    flashCounter--;
                }
                flashOn = !flashOn;
            }

            if (flashCounter > 0) {
                next_state = FLASH_RED_END;
            } else {
                next_state = GREEN;
            }
            break;
    }

    state = next_state;
}

int main() {
    xil_printf("Starting Bike Crossing FSM (timer-driven)...\r\n");

    // Set directions
    unsigned *greenLEDsTri = GREEN_LEDS_REG + GPIO_TRI_OFFSET;
    unsigned *buttonsTri   = PUSH_BTNS_REG + GPIO_TRI_OFFSET;
    unsigned *rgbLEDsTri   = RGB_LEDS_REG + GPIO_TRI_OFFSET;

    *greenLEDsTri = 0x0;   // LEDs = output
    *rgbLEDsTri   = 0x0;   // RGB LEDs = output
    *buttonsTri   = 0xF;   // push buttons = input

    state = GREEN;

    // Main loop
    while (1) {
        FSM_tick();
    }

    return 0;
}
