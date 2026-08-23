
    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>

    // Abstraksjon av hardware-register mapping på bare-metal mikrokontroller
    typedef struct {
        volatile uint32_t GPIO_OUT;      // Fysisk GPIO output register
        volatile uint32_t NVIC_ISER;     // Interrupt Set-Enable Register
        volatile uint32_t TIMER_COUNTER; // Hardware Timer Counter
        uint32_t rtos_thread_state;      // 0 = READY, 1 = RUNNING, 2 = SLEEP
    } ZephyrHardwareRegisterMap;

    // Kjører en direkte bare-metal register-interaksjon uten POSIX-kall
    int execute_baremetal_rtos_tick(ZephyrHardwareRegisterMap* hw, uint32_t command_mask) {
        // 1. Simulerer direkte skriving til GPIO-register (f.eks. utløse maskinvaresignal)
        hw->GPIO_OUT ^= command_mask;

        // 2. Oppdaterer timer-teller direkte i minne-mappet register
        hw->TIMER_COUNTER += 1000U; // 1 ms hardware tick

        // 3. Styrer RTOS-trådens tilstand direkte
        if (command_mask & 0x01U) {
            hw->rtos_thread_state = 1; // RUNNING
            hw->NVIC_ISER |= 0x00000001U; // Aktiverer hardware IRQ 0
        } else {
            hw->rtos_thread_state = 2; // SLEEP (Lav-effekt tilstand)
        }

        return (int)(hw->GPIO_OUT ^ hw->TIMER_COUNTER);
    }
    