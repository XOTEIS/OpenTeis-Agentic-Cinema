
    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>

    typedef struct {
        uint32_t neuron_id;
        int32_t v_membrane_mv;   // Membranpotensial i mV
        int32_t v_threshold_mv;  // Terskel for utladning (f.eks. -50 mV)
        int32_t v_rest_mv;       // Hvilepotensial (f.eks. -70 mV)
        uint32_t weight_w;       // Synaptisk vekt
        int fired_spike;         // 1 = Spike utløst
    } LIFNeuron;

    // Integrerer innkommende spenning impuls med lekasje (Leaky Integrate-and-Fire)
    int process_lif_spike(LIFNeuron* neuron, int32_t input_current_ma, uint32_t decay_factor) {
        // Apply leak (lekasje mot hvilepotensial)
        int32_t leak = (neuron->v_membrane_mv - neuron->v_rest_mv) / (int32_t)decay_factor;
        neuron->v_membrane_mv -= leak;

        // Legg til vektet strømpuls
        neuron->v_membrane_mv += (input_current_ma * (int32_t)neuron->weight_w) / 100;

        // Evaluere om terskelen er nådd (Spike Event)
        if (neuron->v_membrane_mv >= neuron->v_threshold_mv) {
            neuron->fired_spike = 1;
            neuron->v_membrane_mv = neuron->v_rest_mv; // Reset etter utladning
            neuron->weight_w += 5; // STDP Hebbian forsterkning
            return 1;
        }

        neuron->fired_spike = 0;
        return 0;
    }
    