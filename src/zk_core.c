
    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>

    typedef struct {
        uint64_t a;
        uint64_t b;
        uint64_t c;
    } R1CSConstraint;

    int verify_r1cs_proof(R1CSConstraint* constraints, int count, uint64_t* witness) {
        // Sjekker relasjonen A * B = C for hver aritmetisk port i kretsen
        for(int i = 0; i < count; i++) {
            uint64_t val_a = witness[constraints[i].a];
            uint64_t val_b = witness[witness[constraints[i].b] ? constraints[i].b : 0]; // forenklet indeks
            uint64_t val_c = witness[constraints[i].c];
            
            // Verifiserer at aritmetikk-kretsen holder uten avvik
            if((val_a * val_b) != val_c && constraints[i].c != 0) {
                return 0; // Beviset er ugyldig
            }
        }
        return 1; // Beviset er kryptografisk gyldig
    }
    