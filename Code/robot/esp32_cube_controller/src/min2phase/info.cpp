/**
 * min2phaseCXX Copyright (C) 2022 Borgo Federico
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 */

#include "min2phase.h"
#include "info.h"

namespace min2phase { namespace info {

    //precomputed binomial coefficient
    int16_t Cnk[PRECOMPUTED_CNK][PRECOMPUTED_CNK] = {0};
    int8_t std2ud[N_MOVES] = {0};
    int16_t ckmv2bit[N_MOVES2+1] = {0};

    /**
     * Initialize binomial coefficient.
     *
     * @return : the matrix with precomputed binomial coefficient.
     */
    void initCnk() {
        uint8_t i, j;

        for (i = 0; i < PRECOMPUTED_CNK; i++) {
            Cnk[i][0] = Cnk[i][i] = 1;

            for (j = 1; j < i; j++)
                Cnk[i][j] = Cnk[i? i-1 : 0][j - 1] + Cnk[i? i-1 : 0][j];
        }
    }

    void initStd2ud() {
        for (int8_t i = 0; i < N_MOVES; i++)
            std2ud[ud2std[i]] = i;

    }

    void initCkmv2bit() {
        uint8_t i, j, ix, jx;

        for (i = 0; i < N_MOVES2; i++) {
            ix = ud2std[i] / 3;
            ckmv2bit[i] = 0;

            for (j = 0; j < N_MOVES2; j++) {
                jx = ud2std[j] / 3;
                ckmv2bit[i] |= ((ix == jx) || ((ix % 3 == jx % 3) && (ix >= jx)) ? 1 : 0) << j;
            }
        }

        ckmv2bit[10] = 0;
    }

    void init(){
        MIN2PHASE_OUTPUT("Info initialization.");
        initCnk();
        initStd2ud();
        initCkmv2bit();
    }
} }
