/**
 * min2phaseCXX Copyright (C) 2022 Borgo Federico
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 *
 * This file contains all the general constant of the algorithm.
 * For example the number of moves, corners or edges.
 * It has also precomputed binomial coefficient to increase the
 * speed of the algorithm.
 */

#ifndef MIN2PHASE_INFO_H
#define MIN2PHASE_INFO_H 1

#include <cstdint>

namespace min2phase { namespace info {

    ///Moves

    /**
     * This is the number of moves of the 3x3 cube.
     */
    const int8_t N_MOVES = 18;

    /**
     * This is the number of moves that can be used in phase 2.
     */
    const int8_t N_MOVES2 = 10;

    /**
     * This is the number of basic moves(R, L, F, B, U, D).
     */
    const int8_t N_BASIC_MOVES = 6;

    /**
     * This is the number
     */
    const int8_t N_GROUP_MOVES = 3;

    /**
     * This is the max moves used to solve the cube.
     */
    const int8_t MAX_LENGTH = 31;

    /**
     * This is the max moves used to solve the phase 1.
     */
    const int8_t P1_LENGTH = 12;

    /**
     * This is the max moves used to solve the phase 2.
     */
    const int8_t P2_LENGTH = 18;

    ///Cube info

    /**
     * This is the number of small plates in the cube. 3*3*6 = 54.
     */
    const int8_t N_PLATES = 54;

    /**
     * This is the number of plates in a face of the cube.
     */
    const int8_t N_PLATES_X_FACE = 9;

    /**
     * This is the number of edges in the 3x3 cube.
     */
    const int8_t NUMBER_EDGES = 12;

    /**
     * This is the number of corner in the 3x3 cube.
     */
    const int8_t NUMBER_CORNER = 8;

    /**
     * This is the number of possibly cases of a corner orientation.
     */
    const int8_t CORNE_ORI_CASES = 3;

    /**
     * This is the number of possibly cases of a edge orientation.
     */
    const int8_t EDGE_ORI_CASES = 2;

    /**
     * This is the number of faces in the cube.
     */
    const int8_t FACES = 6;

    ///coords

    /**
     * This is the number of UDSlice coordinate. 12*11*10*9/4!
     */
    const uint16_t N_SLICE = 495;

    /**
     * This is the number of corner orientation coordinate. 3^7
     */
    const uint16_t N_TWIST = 2187;

    /**
     * This is the number of edge orientation. 2^7
     */
    const uint16_t N_FLIP = 2048;

    /**
     * This is the number of corner permutation. 8!
     */
    const uint16_t N_PERM = 40320;

    /**
     * This is the number of edge permutation. 12!
     */
    const uint32_t FULL_E_PERM = 479001600;

    /**
     * This is the number of UDSliceSorted coordinate. 4!
     */
    const uint8_t N_MPERM = 24;

    /**
     * This is the parity of all edges or corners, and position of 4 corners.
     */
    const uint8_t N_COMB = 140;
    const uint8_t P2_PARITY_MOVE = 0xA5;

    //Sym TODO
    const int8_t SYM = 16;
    const int8_t FULL_SYM = 48;
    const int8_t SYM_CLASSES = 8;
    const uint16_t N_TWIST_SYM = 324;
    const uint16_t N_FLIP_SYM = 336;
    const uint16_t N_PERM_SYM = 2768;

    /**
     * The moves used in the cube.
     */
     typedef enum : int8_t {
         UX1 = 0,
         UX2 = 1,
         UX3 = 2,
         RX1 = 3,
         RX2 = 4,
         RX3 = 5,
         FX1 = 6,
         FX2 = 7,
         FX3 = 8,
         DX1 = 9,
         DX2 = 10,
         DX3 = 11,
         LX1 = 12,
         LX2 = 13,
         LX3 = 14,
         BX1 = 15,
         BX2 = 16,
         BX3 = 17
     }Moves;

    /**
     * The facelets of the cube.
     */
    typedef enum : int8_t {
        U1 = 0,
        U2 = 1,
        U3 = 2,
        U4 = 3,
        U5 = 4,
        U6 = 5,
        U7 = 6,
        U8 = 7,
        U9 = 8,
        R1 = 9,
        R2 = 10,
        R3 = 11,
        R4 = 12,
        R5 = 13,
        R6 = 14,
        R7 = 15,
        R8 = 16,
        R9 = 17,
        F1 = 18,
        F2 = 19,
        F3 = 20,
        F4 = 21,
        F5 = 22,
        F6 = 23,
        F7 = 24,
        F8 = 25,
        F9 = 26,
        D1 = 27,
        D2 = 28,
        D3 = 29,
        D4 = 30,
        D5 = 31,
        D6 = 32,
        D7 = 33,
        D8 = 34,
        D9 = 35,
        L1 = 36,
        L2 = 37,
        L3 = 38,
        L4 = 39,
        L5 = 40,
        L6 = 41,
        L7 = 42,
        L8 = 43,
        L9 = 44,
        B1 = 45,
        B2 = 46,
        B3 = 47,
        B4 = 48,
        B5 = 49,
        B6 = 50,
        B7 = 51,
        B8 = 52,
        B9 = 53
    }Facelets;

    /**
     * The faces of the cube.
     */
    typedef enum : int8_t {
        U = 0,
        R = 1,
        F = 2,
        D = 3,
        L = 4,
        B = 5
    }Faces;

    typedef enum : int8_t {
        NO_ERROR = 0, ///There is no error solving the cube.
#define NO_ERROR NO_ERROR
        MALFORMED_STRING = 1, ///The input string is not correct.
#define MALFORMED_STRING MALFORMED_STRING
        MISSING_EDGE = 2, ///An edge is missing in the edge permutation.
#define MISSING_EDGE MISSING_EDGE
        FLIPPED_EDGE = 3, ///An edge is flipped in the wrong way.
#define FLIPPED_EDGE FLIPPED_EDGE
        MISSING_CORNER = 4, ///A corner is missing in the corner permutation.
#define MISSING_CORNER MISSING_CORNER
        TWISTED_CORNER = 5, ///A corner is rotated in the wrong way.
#define TWISTED_CORNER TWISTED_CORNER
        PARITY_ERROR = 6, ///The parity of coordinates is wrong.
#define PARITY_ERROR PARITY_ERROR
        SHORT_DEPTH = 7, ///The depth assigned for the algorithm is too short.
#define SHORT_DEPTH SHORT_DEPTH
        PROBE_LIMIT = 8, ///The limit of cube to explore is too low.
#define PROBE_LIMIT PROBE_LIMIT
        MISSING_COORDS = 9 ///The coordinates are not initialized.
#define MISSING_COORDS MISSING_COORDS
    }Errors;

    /**
     * The max number of precomputed binomial coefficient.
     */
    const int8_t PRECOMPUTED_CNK = 13;

    /**
     * The facelets names for every corner.
     */
    const int8_t cornerFacelet[][CORNE_ORI_CASES]{
            {U9, R1, F3},
            {U7, F1, L3},
            {U1, L1, B3},
            {U3, B1, R3},
            {D3, F9, R7},
            {D1, L9, F7},
            {D7, B9, L7},
            {D9, R9, B7}
    };

    /**
     * The facelets names for every edge.
     */
    const int8_t edgeFacelet[][EDGE_ORI_CASES]{
            {U6, R2},
            {U8, F2},
            {U4, L2},
            {U2, B2},
            {D6, R8},
            {D2, F8},
            {D4, L8},
            {D8, B8},
            {F6, R4},
            {F4, L6},
            {B6, L4},
            {B4, R6}
    };

    const int8_t ud2std[N_MOVES] = {min2phase::info::UX1,
                                     min2phase::info::UX2,
                                     min2phase::info::UX3,
                                     min2phase::info::RX2,
                                     min2phase::info::FX2,
                                     min2phase::info::DX1,
                                     min2phase::info::DX2,
                                     min2phase::info::DX3,
                                     min2phase::info::LX2,
                                     min2phase::info::BX2,
                                     min2phase::info::RX1,
                                     min2phase::info::RX3,
                                     min2phase::info::FX1,
                                     min2phase::info::FX3,
                                     min2phase::info::LX1,
                                     min2phase::info::LX3,
                                     min2phase::info::BX1,
                                     min2phase::info::BX3};


    /**
     * This is used to convert the moves from a symmetry.
     */
    const int8_t urfMove[FACES][N_MOVES]{
            {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,  10, 11, 12, 13, 14, 15, 16, 17},
            {6, 7, 8, 0, 1, 2, 3, 4, 5, 15, 16, 17, 9,  10, 11, 12, 13, 14},
            {3, 4, 5, 6, 7, 8, 0, 1, 2, 12, 13, 14, 15, 16, 17, 9,  10, 11},
            {2, 1, 0, 5, 4, 3, 8, 7, 6, 11, 10, 9,  14, 13, 12, 17, 16, 15},
            {8, 7, 6, 2, 1, 0, 5, 4, 3, 17, 16, 15, 11, 10, 9,  14, 13, 12},
            {5, 4, 3, 8, 7, 6, 2, 1, 0, 14, 13, 12, 17, 16, 15, 11, 10, 9}
    };

    /**
     * In this matrix is stored the precomputed binomial coefficient.
     */
    extern int16_t Cnk[PRECOMPUTED_CNK][PRECOMPUTED_CNK];
    extern int8_t std2ud[N_MOVES];
    extern int16_t ckmv2bit[N_MOVES2 + 1];

    /**
     * Init all the precomputed mathematical computation. For
     * example the binomial coefficient.
     */
    void init();
} }

#endif //MIN2PHASE_INFO_H
