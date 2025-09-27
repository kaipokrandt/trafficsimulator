# Bike / Pedestrian Crossing FSM – Arty A7-100 (MicroBlaze)

This project implements a **timer-driven finite state machine (FSM)** on the Arty A7-100 FPGA using a MicroBlaze soft processor. It simulates a pedestrian/cyclist crossing signal with **green** and **red** LEDs, flashing phases, and button input. The program uses **AXI GPIO** and an **AXI Timer (generate mode)** to control LEDs and timing without busy-loops.

> **Status:** *This project is under active development.*  
> The current version uses a **polling approach** for the AXI Timer’s interrupt flag.  
> The **next implementation** will transition to a fully **interrupt-driven MicroBlaze system**, removing polling from the main loop and handling timer events via an ISR for improved efficiency.

## Overview

- **Platform:** Digilent Arty A7-100T FPGA  
- **Processor:** Xilinx MicroBlaze (bare-metal C program)  
- **Peripherals:**  
  - AXI GPIO at `0x4000_0000` for green LEDs & push buttons  
  - AXI GPIO at `0x4001_0000` for RGB LEDs (red channels used)  
  - AXI Timer at `0x41C0_0000` for 0.5-second ticks  

When a push button is pressed, the FSM cycles through:

1. **GREEN** – Green LED solid, timer stopped.  
2. **FLASH_RED_START** – Red LEDs flash (0.5 s on/off) for 3 s.  
3. **RED** – Red LEDs solid for 4 s.  
4. **FLASH_RED_END** – Red LEDs flash again for 3 s.  
5. Return to **GREEN**.

All timing is handled by the AXI Timer’s interrupt flag, polled in software.

## Features

- **Non-blocking timing:** Uses AXI Timer generate mode instead of software delays.  
- **Hardware abstraction:** Helper functions to turn green/red LEDs on or off.  
- **Clean FSM structure:** `FSM_tick()` updates state and actions every main-loop iteration.  
- **Configurable durations:** `HALFSEC_COUNT` defines tick length (here 0.5 s at 81.247 MHz).  
- **Future work:** Transition to **interrupt-driven** timer events for improved responsiveness.

## Hardware Mapping

| Peripheral        | Base Address | Usage                     |
|------------------|--------------|---------------------------|
| AXI GPIO 0        | `0x40000000` | Green LEDs + push buttons |
| AXI GPIO 1        | `0x40010000` | RGB LEDs (red only)       |
| AXI Timer         | `0x41C00000` | 0.5 s tick generator      |

Green LEDs = output.  
RGB LEDs = output (only red bits are set in `setRedLED`).  
Push buttons = input.

## Building & Running

1. **Vivado / Vitis:**  
   - Add the AXI GPIO and AXI Timer IP blocks at the base addresses shown above.  
   - Connect them to the MicroBlaze M_AXI bus.  
   - Export hardware to Vitis (or Xilinx SDK).

2. **Import the C source** into a new Vitis bare-metal application project.

3. **Compile and program** the bitstream and ELF onto the Arty A7.

4. **Observe LEDs:**  
   - Green LED on by default.  
   - Press a push button to start the flash-red → solid-red → flash-red cycle.  
   - After the cycle, the FSM returns to green.

5. **Serial output:**  
   - Connect to the MicroBlaze UART at 115200 bps.  
   - The program prints state transitions and flash events via `xil_printf`.

## Code Structure

- **Timer Control:**  
  - `timer_init_halfsec()` – configures and starts 0.5 s auto-reload timer.  
  - `timer_expired()` – polls the T0INT flag and clears it.  
  - `timer_stop()` – disables timer.

- **LED Control:**  
  - `setGreenLED(int on)` / `setRedLED(int on)` – write to GPIO registers.

- **FSM:**  
  - `FSM_tick()` – state machine logic, called in main loop.  
  - `State` enum defines `GREEN`, `FLASH_RED_START`, `RED`, `FLASH_RED_END`.

## Customizing

- Adjust `HALFSEC_COUNT` if your AXI Timer clock differs from 81.247 MHz.  
- Change `flashCounter` or `redTicks` to modify flash/solid durations.  
- Expand LED patterns or add other signals as needed.

## Roadmap

- **Current:** Polling-based timer events.
- **Next:** Implement **interrupt-driven** timer handling using MicroBlaze ISRs.
- **Future:** Extend to multi-button inputs, configurable timing via switches, or integration with other peripherals.

---

This project demonstrates how to replace dummy delays with an AXI Timer in MicroBlaze applications, making timing predictable and freeing CPU cycles for other tasks. The upcoming interrupt-driven version will further improve responsiveness and design quality.
