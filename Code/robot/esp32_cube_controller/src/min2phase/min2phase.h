/**
 * min2phaseCXX Copyright (C) 2022 Borgo Federico
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 *
 * In this file there are all the functions used to solver a Rubik's cube.
 * You can configure the solver in base the speed you need. You can
 * define the limit of moves to use, the max cubes to search and format the
 * output string.
 * The result of this algorithm is a string that contains the moves necessary
 * to solve the input cube. The output can be formatted. It can add a separator from
 * phase 1 and phase 2, the inverse solution and the number of moves used.
 * This algorithm can also find the optimal solution for a cube. To use it, add the
 * optimal solution flag to the solver input.
 * This algorithm is an implementation of the Kociemba's 2 phase algorithm (http://kociemba.org/)
 * and a fork of the min2phase Java algorithm (https://github.com/cs0x7f/min2phase/).
 */

#ifndef MIN2PHASE
#define MIN2PHASE 1

#include <cstdint>
#include <string>

/**
 * Used to print the info of the solver.
 * 0-generic
 * 1-more details
 *
 * Do not change this if you are not debugging the solver.
 */
//#define MIN2PHASE_DEBUG 0

#ifdef MIN2PHASE_DEBUG
#include <iostream>
///logger
#define MIN2PHASE_OUTPUT(x) std::cout << x << std::endl;
#else
///fake logger
#define MIN2PHASE_OUTPUT(x)
#endif

namespace min2phase{

    /**
     * This is used to indicate if use a dot from the moves
     * used in phase 1 and 2.
     */
    const int8_t USE_SEPARATOR = 0x1;

    /**
     * This is used to find a scramble that generate the input cube from the
     * solved cube instead of generate a scramble that solve the cube.
     */
    const int8_t INVERSE_SOLUTION = 0x2;

    /**
     * This is used to add the number of moves used at the end of
     * the string.
     */
    const int8_t APPEND_LENGTH = 0x4;

    /**
     *  Determines if guaranteeing the solution to be optimal.
     */
    const int8_t OPTIMAL_SOLUTION = 0x8;

    /**
     * This function compute all the variable necessary for te algorithm.
     */
    void init();

    /**
     * Write all the coordinates into a binary file.
     *
     * @param name      : the name and path of the file.
     * @return          : true if the file was written, false if not.
     */
    bool writeFile(const std::string& name);

    /**
     * Load all the coordinates from a binary file.
     *
     * @param name      : the name and path of the file.
     * @return          : true if the file was red, false if not.
     */
    bool loadFile(const std::string& name);

    /**
     * This is used to solve the cube. It has many configuration in base of the
     * power of the machine or the memory. Less the max depth is, more the time
     * to solve the cube is used, is suggested to use from 20 to 31 because it
     * indicates the max number of moves are used to solve the cube. The probe
     * indicate the number of cube that the algorithm is forced to explore.
     * If you need speed i suggest 0 for the probeMin and 100000000 for the max.
     * You can also change the formatting string in output adding it at verbose,
     * for example OutputFormat::USE_SEPARATOR | OutputFormat::INVERSE_SOLUTION.
     * This the order of the colors for the string:
     * U1 -> U9 R1 -> R9 F1 -> F9 D1 -> D9 L1 -> L9 B1 -> B9
     * Solved cube: UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB
     *
     *             |************|
     *             |*U1**U2**U3*|
     *             |************|
     *             |*U4**U5**U6*|
     *             |************|
     *             |*U7**U8**U9*|
     *             |************|
     * ************|************|************|************|
     * *L1**L2**L3*|*F1**F2**F3*|*R1**R2**R3*|*B1**B2**B3*|
     * ************|************|************|************|
     * *L4**L5**L6*|*F4**F5**F6*|*R4**R5**R6*|*B4**B5**B6*|
     * ************|************|************|************|
     * *L7**L8**L9*|*F7**F8**F9*|*R7**R8**R9*|*B7**B8**B9*|
     * ************|************|************|************|
     *             |************|
     *             |*D1**D2**D3*|
     *             |************|
     *             |*D4**D5**D6*|
     *             |************|
     *             |*D7**D8**D9*|
     *             |************|
     *
     * @param facelet  : the order of colors like the cube above.
     * @param maxDepth : the max of moves used to solve the cube, from 20 to 31.
     * @param probeMax : the max of cube explored in phase 2.
     * @param probeMin : the min of cube explored in phase 2.
     * @param verbose  : the format for the output strings.
     * @param usedMoves: the pointer to store the number of moves used.
     * @return         : the moved necessary to solve the cube.
     */
    std::string solve(const std::string& facelets, int8_t maxDepth, int32_t probeMax, int32_t probeMin, int8_t verbose, uint8_t* usedMoves = nullptr);

    /**
     * This is used to init and http server.
     *
     * @warning: this function will abilitate an handler for the zombie process,
     *           do not register another handler with SIGCHLD.
     *
     * @param port      : the port of the server.
     * @pram mReq       : (Optional) the max number of requests.
     * @return          : true if it was done correctly, false if not.
     */
    bool server(uint16_t port, uint16_t mReq = 10000);

    /**
     * This stops the solver server. It also remove the SIGCHLD signal handler.
     *
     * @return          : true if it was done correctly, false if not.
     */
    bool stop();

    /**
     * Remote cube solver. It connects form a remote cube solver server and
     * ask the solution of the cube.
     *
     * @see: the local solver documentation
     *
     * @param ip        : the IP of the server.
     * @param port      : the pot server
     * @param facelet   : the order of colors like the cube above.
     * @param maxDepth  : the max of moves used to solve the cube, from 20 to 31.
     * @param probeMax  : the max of cube explored in phase 2.
     * @param probeMin  : the min of cube explored in phase 2.
     * @param verbose   : the format for the output strings.
     * @param usedMoves : the pointer to store the number of moves used.
     * @param time      : the string pointer to store the time used to solve the cube.
     * @return          : the moved necessary to solve the cube.
     */
    std::string webSearch(const std::string& ip, int32_t port,
                          const std::string &facelets, int8_t maxDepth,
                          int32_t probeMax, int32_t probeMin,
                          int8_t verbose, uint8_t* usedMoves = nullptr,
                          std::string *time = nullptr);
}

#endif //MIN2PHASE
