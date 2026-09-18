/**
 * min2phaseCXX Copyright (C) 2022 Borgo Federico
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 */

#include "min2phase.h"
#include "coords.h"
#include "platform.h"

namespace min2phase { namespace coords {
    typedef enum : uint8_t {
        MCPP_IDX,
        PCPP_IDX,
        STP_IDX,
        SFP_IDX,
        TFP_IDX,
        FLIP_TYPE,
        TWIST_TYPE,
        EPERM_TYPE
    }CoordType;

    /**
     * The edge perm and corner perm have the same symmetry structure.
     * So their ClassIndexToRepresentArray (see Kociemba's algorithm) are the same.
     * So, when you want the symmetry of edge permutation you do y*16+k, where
     * y is the edge permutation.
     * Otherwise, if you need the symmetry of corner permutation you do
     * y*16+(k^e2c[k]) where y is the corner orientation.
     *
     * This is e2c.
     */
    static const int32_t SYM_E2C_MAGIC = 0x00DDDD00;

    ///ClassIndexToRepresentArrays

    /**
     * This is used to convert the raw symmetry to raw coordinate
     * of edge orientation.
     * Used only in the coordinate computation.
     * This is temporary.
     */
    static uint16_t FlipS2R[info::N_FLIP_SYM];

    /**
     * This is used to convert the raw symmetry to raw coordinate
     * of corner orientation.
     * Used only in the coordinate computation.
     * This is temporary.
     */
    static uint16_t TwistS2R[info::N_TWIST_SYM];

    /**
     * This is used to convert the raw symmetry to raw coordinate of
     * edge and corner orientation.
     * Used only in the coordinate computation.
     * This is temporary.
     */
    static uint16_t EPermS2R[info::N_PERM_SYM];

    ///Symmetries

    /**
     * This is a temporary array used to compute the symmetry state of edge orientation.
     */
    static uint16_t SymStateTwist[info::N_TWIST_SYM];

    /**
     * This is a temporary array used to compute the symmetry state for corner orientation.
     */
    static uint16_t SymStateFlip[info::N_FLIP_SYM];

    /**
     * This is a temporary array used to compute the symmetry state for permutations.
     */
    static uint16_t SymStatePerm [info::N_PERM_SYM];

    /**
     * This method compute the cubes generated from the 18 moves and store them in an array.
     */
    void initMove();

    /**
     * This method compute the cubes generated from the combinations of S_F2, S_U4
     * and S_LR2.
     */
    void initSym();

    /**
     * This method is used to initialize the coords transformation from symmetry to
     * raw permutation coordinate.
     * raw coordinate = transformation array[symmetry]
     */
    void initPermSym2Raw();

    /**
     * This method is used to initialize the coords transformation from symmetry to coordinate.
     * raw coordinate = transformation array[symmetry].
     *
     * @param N_RAW    : the size of raw coordinate.
     * @param Sym2Raw  : the array where is stored the convertor from symmetry to raw.
     * @param Raw2Sym  : the array where is stored the convertor from raw to symmetry,
     * @param SymState : the array containing the symmetry of the coordinate.
     * @param coord    : the type of coordinate, flip, twist or edge perm.
     */
    void initSym2Raw(uint16_t N_RAW, uint16_t Sym2Raw[], uint16_t Raw2Sym[], uint16_t SymState[], const CoordType& coord);

    /**
     * This method compute the corner and edge permutation
     * move table table to increase the speed of the algorithm.
     */
    void initPermsMove();

    /**
     * This method compute the UDSliceSorted move table.
     */
    void initMPermMoveConj();

    /**
     * THis method compute the corner comb move table.
     */
    void initCombPMoveConj();

    /**
     * This method compute the edge orientation move table.
     */
    void initFlipMove();

    /**
     * This method is used to compute the corner orientation move table.
     */
    void initTwistMove();

    /**
     * This methos is used to compute the UDSlice move tavble.
     */
    void initUDSliceMoveConj();

    /**
     * This method is used to create a configuration for the pruning compute.
     */
    void initAllPrun();

    /**
     * This method compute the pruning table for every coordinate.
     *
     * @param PrunTable     : the array to store the pruning table.
     * @param RawMove1      : the matrix of raw moves for phase 2
     * @param RawConj1      : the matrix of table for raw symmetry in phase 2.
     * @param SymMoveVect1  : the matrix of raw moves coordinate for phase 2
     * @param RawMove2      : the matrix of raw moves for phase 1.
     * @param RawConj2      : the matrix of table for raw symmetry in phase 1.
     * @param SymMoveVect2  : the matrix of raw moves coordinate for phase 1 .
     * @param SymState      : the array of symmetries.
     * @param PrunFlag      : the flag used to generate the pruning.
     * @param PrunTableSize : the size of the PrunFlag.
     * @param type          : the type of pruning computed, you determinate if from the type of coordinate.
     */
    void initRawSymPrun(int32_t PrunTable[],
                        uint8_t RawMove1[][info::N_MOVES2], uint8_t RawConj1[][info::SYM], uint16_t SymMoveVect1[][info::N_MOVES2],
                        uint16_t RawMove2[][info::N_MOVES], uint16_t RawConj2[][info::SYM_CLASSES], uint16_t SymMoveVect2[][info::N_MOVES],
                        const uint16_t SymState[], int32_t PrunFlag, int32_t PrunTableSize, const CoordType& type);

    /**
     * This method is used to check if a pruning value has the 0 value.
     *
     * @param val : the pruning value to check.
     * @return    : true if has 0, false if not.
     */
    bool hasZero(int32_t val);

    coords_t* coords = nullptr;

        //init all coordinates
    void init() {
        MIN2PHASE_OUTPUT("Info initialization.")
        coords->urf.setCoords(2531, 1373, 67026819, 1367);
        coords->urfInv.setCoords(2089, 1906, 322752913, 2040);

        initMove();
        initSym();
        initPermSym2Raw();

        initPermsMove();
        initMPermMoveConj();
        initCombPMoveConj();

        initFlipMove();
        initTwistMove();
        initUDSliceMoveConj();

        initAllPrun();

        coords->isInitialized = true;
    }

    bool isInit() {
        return coords && coords->isInitialized;
    }

    //b = S_idx^-1 * a * S_idx for only corner
    void cornConjugate(const CubieCube &a, int8_t idx, CubieCube &b) {
        int8_t oriA, oriB, corn, ori;
        CubieCube sinv, s;

        sinv.copy(coords->CubeSym[coords->SymMultInv[0][idx]]);
        s.copy(coords->CubeSym[idx]);

        for (corn = 0; corn < info::NUMBER_CORNER; corn++) {
            oriA = sinv.corners[a.corners[s.corners[corn] & 7] & 7] >> info::CORNE_ORI_CASES;
            oriB = a.corners[s.corners[corn] & 7] >> info::CORNE_ORI_CASES;
            ori = (oriA < info::CORNE_ORI_CASES) ? oriB : (info::CORNE_ORI_CASES - oriB) %
                                                          info::CORNE_ORI_CASES;
            b.corners[corn] = sinv.corners[a.corners[s.corners[corn] & 7] & 7] & 7 | ori << info::CORNE_ORI_CASES;
        }
    }

    //b = S_idx^-1 * a * S_idx for only edges
    void edgeConjugate(const CubieCube &a, int8_t idx, CubieCube &b) {
        uint8_t ed;
        CubieCube sinv, s;

        sinv.copy(coords->CubeSym[coords->SymMultInv[0][idx]]);
        s.copy(coords->CubeSym[idx]);

        for (ed = 0; ed < info::NUMBER_EDGES; ed++)
            b.edges[ed] =
                    sinv.edges[a.edges[s.edges[ed] >> 1] >> 1] ^ (a.edges[s.edges[ed] >> 1] & 1) ^ (s.edges[ed] & 1);
    }

    //perm from sym
    uint16_t getPermSymInv(uint16_t idx, int8_t sym, bool isCorner) {
        uint16_t idxi = coords->PermInvEdgeSym[idx];

        if (isCorner)
            idxi = ESym2CSym(idxi);

        return idxi & 0xfff0 | coords->SymMult[idxi & 0xf][sym];
    }

    //check if two cubes are the same
    bool isSameCube(const int8_t cube1[], const int8_t cube2[], bool isEdge) {
        const int8_t len = isEdge ? info::NUMBER_EDGES : info::NUMBER_CORNER;
        int8_t i;

        for (i = 0; i < len; i++)
            if (cube1[i] != cube2[i])
                return false;

        return true;
    }

    //y*16+(k^e2c[k])
    int32_t ESym2CSym(uint16_t idx) {
        return idx ^ (SYM_E2C_MAGIC >> ((idx & 0xf) << 1) & 3);
    }

    //set value in pruning table
    void setPruning(int32_t table[], int32_t index, int8_t value) {
        table[index >> 3] ^= value << ((index << 2)%(sizeof(int32_t)*8)); // index << 2 <=> (index & 7) << 2
    }

    //get value in pruning table
    int8_t getPruning(const int32_t table[], int32_t index) {
        return table[index >> 3] >> ((index << 2)%(sizeof(int32_t)*8)) & 0xf; // index << 2 <=> (index & 7) << 2
    }

    bool hasZero(int32_t val) {
        return ((val - 0x11111111) & ~val & 0x88888888) != 0;
    }

    //init basic 18 moves
    void initMove() {
        const int16_t cPermSon[info::N_BASIC_MOVES] = {15120, 21021, 8064, 9, 1230, 224};
        const int16_t cOriSon[info::N_BASIC_MOVES] = {0, 1494, 1236, 0, 412, 137};
        const int32_t ePermSon[info::N_BASIC_MOVES] = {119750400, 323403417, 29441808, 5880, 2949660, 328552};
        const int16_t eOriSon[info::N_BASIC_MOVES] = {0, 0, 550, 0, 0, 137};
        uint8_t i, j;

        for (i = 0; i < info::N_MOVES; i += info::N_GROUP_MOVES)
            coords->moveCube[i].setCoords(cPermSon[i / 3], cOriSon[i / 3], ePermSon[i / 3], eOriSon[i / 3]);

        for (i = 0; i < info::N_MOVES; i += info::N_GROUP_MOVES) {
            for (j = 0; j < info::N_GROUP_MOVES - 1; j++) {
                CubieCube::edgeMult(coords->moveCube[i + j], coords->moveCube[i], coords->moveCube[i + j + 1]);
                CubieCube::cornMult(coords->moveCube[i + j], coords->moveCube[i], coords->moveCube[i + j + 1]);
            }
        }
    }

    //init all sym
    void initSym() {
        uint16_t i;
        uint8_t j, k;

        CubieCube t, c, d, f2, u4, lr2;

        f2.setCoords(28783, 0, 259268407, 0);
        u4.setCoords(15138, 0, 119765538, 7);
        lr2.setCoords(5167, 0, 83473207, 0);

        for (i = 0; i < info::NUMBER_CORNER; i++)
            lr2.corners[i] |= info::CORNE_ORI_CASES << info::CORNE_ORI_CASES;

        for (i = 0; i < info::SYM; i++) {
            coords->CubeSym[i].copy(c);

            CubieCube::cornMultFull(c, u4, d);
            CubieCube::edgeMult(c, u4, d);
            t = d; d = c; c = t;

            if (i % 4 == 3) {
                CubieCube::cornMultFull(c, lr2, d);
                CubieCube::edgeMult(c, lr2, d);
                t = d; d = c; c = t;
            }

            if (i % 8 == 7) {
                CubieCube::cornMultFull(c, f2, d);
                CubieCube::edgeMult(c, f2, d);
                t = d; d = c; c = t;
            }
        }

        for (i = 0; i < info::SYM; i++) {
            for (j = 0; j < info::SYM; j++) {
                CubieCube::cornMultFull(coords->CubeSym[i], coords->CubeSym[j], c);

                for (k = 0; k < info::SYM; k++) {
                    if (isSameCube(coords->CubeSym[k].corners, c.corners, false)) {
                        coords->SymMult[i][j] = (int8_t)k;
                        coords->SymMultInv[k][j] = (int8_t)i;
                        break;
                    }
                }
            }
        }

        for (i = 0; i < info::N_MOVES; i++) {
            for (j = 0; j < info::SYM; j++) {
                cornConjugate(coords->moveCube[i], coords->SymMultInv[0][j], c);

                for (k = 0; k < info::N_MOVES; k++) {
                    if (isSameCube(coords->moveCube[k].corners, c.corners, false)) {
                        coords->SymMove[j][i] = k;
                        coords->SymMoveUD[j][info::std2ud[i]] = info::std2ud[k];
                        break;
                    }
                }

                if (j % 2 == 0)
                    coords->Sym8Move[i << 3 | j >> 1] = coords->SymMove[j][i];
            }

            coords->moveCubeSym[i] = coords->moveCube[i].selfSym();
            j = (uint8_t)i;

            for (int s = 0; s < info::FULL_SYM; s++) {
                if (coords->SymMove[s % info::SYM][j] < i)
                    coords->firstMoveSym[s] |= 1 << i;

                if (s % info::SYM == 15) {
                    j = info::urfMove[2][j];
                }
            }
        }
    }

    //raw coordinate = transformation array[symmetry]
    void initPermSym2Raw() {
        int16_t i;
        CubieCube cc;

        initSym2Raw(info::N_PERM, EPermS2R, coords->EPermR2S, SymStatePerm, EPERM_TYPE);

        for (i = 0; i < info::N_PERM_SYM; i++) {
            cc.setEPerm(EPermS2R[i]);
            coords->Perm2CombP[i] = CubieCube::getComb(cc.edges, 0, true) +
                            CubieCube::getNParity(EPermS2R[i], info::NUMBER_CORNER) * 70;
            cc.inv();
            coords->PermInvEdgeSym[i] = cc.getEPermSym();
        }

        for (i = 0; i < info::N_MPERM; i++) {
            cc.setMPerm(i);
            cc.inv();
            coords->MPermInv[i] = cc.getMPerm();
        }

        initSym2Raw(info::N_FLIP, FlipS2R, coords->FlipR2S, SymStateFlip, FLIP_TYPE);

        initSym2Raw(info::N_TWIST, TwistS2R, coords->TwistR2S, SymStateTwist, TWIST_TYPE);
    }

    //raw coordinate = transformation array[symmetry].
    void initSym2Raw(const uint16_t N_RAW, uint16_t Sym2Raw[], uint16_t Raw2Sym[],
                                            uint16_t SymState[], const CoordType &coord) {
        CubieCube c, d;
        const uint8_t symInc = coord == EPERM_TYPE ? 1 : 2;
        const bool isEdge = coord != TWIST_TYPE;
        uint16_t i, symIdx, idx, count;
        uint8_t j;

        idx = 0;
        count = 0;

        for (i = 0; i < N_RAW; i++)
            Raw2Sym[i] = 0;

        for (i = 0; i < N_RAW; i++) { solver_poll();
            if (Raw2Sym[i] != 0)
                continue;

            switch (coord) {
                case FLIP_TYPE:
                    c.setFlip(i);
                    break;
                case TWIST_TYPE:
                    c.setTwist(i);
                    break;
                case EPERM_TYPE:
                    c.setEPerm(i);
                    break;
                default:
                    break;
            }

            for (j = 0; j < info::SYM; j += symInc) {
                if (isEdge)
                    edgeConjugate(c, j, d);
                else
                    cornConjugate(c, j, d);

                switch (coord) {
                    case FLIP_TYPE:
                        idx = d.getFlip();
                        coords->FlipS2RF[count << 3 | j >> 1] = idx;
                        break;
                    case TWIST_TYPE:
                        idx = d.getTwist();
                        break;
                    case EPERM_TYPE:
                        idx = d.getEPerm();
                        break;
                    default:
                        break;
                }

                if (idx == i)
                    SymState[count] |= 1 << (j / symInc);

                symIdx = (count << 4 | j) / symInc;
                Raw2Sym[idx] = symIdx;
            }
            Sym2Raw[count++] = i;
        }
    }

    //Corner and Edge permutation table
    void initPermsMove() {
        uint16_t i;
        uint8_t j;
        CubieCube c, d;

        for (i = 0; i < info::N_PERM_SYM; i++) {
            c.setCPerm(EPermS2R[i]);
            c.setEPerm(EPermS2R[i]);

            for (j = 0; j < info::N_MOVES2; j++) {
                CubieCube::cornMult(c, coords->moveCube[info::ud2std[j]], d);
                coords->CPermMove[i][j] = d.getCPermSym();

                CubieCube::edgeMult(c, coords->moveCube[info::ud2std[j]], d);
                coords->EPermMove[i][j] = d.getEPermSym();
            }
        }
    }

    //UDSlice sorted move table
    void initMPermMoveConj() {
        uint8_t i, j;
        CubieCube c, d;

        for (i = 0; i < info::N_MPERM; i++) {
            c.setMPerm(i);

            for (j = 0; j < info::N_MOVES2; j++) {
                CubieCube::edgeMult(c, coords->moveCube[info::ud2std[j]], d);
                coords->MPermMove[i][j] = d.getMPerm();
            }

            for (j = 0; j < info::SYM; j++) {
                edgeConjugate(c, coords->SymMultInv[0][j], d);
                coords->MPermConj[i][j] = d.getMPerm();
            }
        }
    }

    //Corner comb move table
    void initCombPMoveConj() {
        uint8_t i, j;
        CubieCube c, d;

        for (i = 0; i < info::N_COMB; i++) {
            c.setCComb(i % 70);

            for (j = 0; j < info::N_MOVES2; j++) {
                CubieCube::cornMult(c, coords->moveCube[info::ud2std[j]], d);
                coords->CCombPMove[i][j] = d.getCComb() + 70 * (info::P2_PARITY_MOVE >> j & 1 ^ (i / 70));
            }

            for (j = 0; j < 16; j++) {
                cornConjugate(c, coords->SymMultInv[0][j], d);
                coords->CCombPConj[i][j] = d.getCComb() + 70 * (i / 70);
            }
        }
    }

    //edge orient move table
    void initFlipMove() {
        uint16_t i;
        uint8_t j;
        CubieCube c, d;

        for (i = 0; i < info::N_FLIP_SYM; i++) {
            c.setFlip(FlipS2R[i]);

            for (j = 0; j < info::N_MOVES; j++) {
                CubieCube::edgeMult(c, coords->moveCube[j], d);
                coords->FlipMove[i][j] = d.getFlipSym();
            }
        }
    }

    //corner orient move table
    void initTwistMove() {
        uint16_t i;
        uint8_t j;
        CubieCube c, d;

        for (i = 0; i < info::N_TWIST_SYM; i++) {
            c.setTwist(TwistS2R[i]);

            for (j = 0; j < info::N_MOVES; j++) {
                CubieCube::cornMult(c, coords->moveCube[j], d);
                coords->TwistMove[i][j] = d.getTwistSym();
            }
        }
    }

    //UDSLice move table
    void initUDSliceMoveConj() {
        uint16_t i, udslice;
        uint8_t j, k;
        CubieCube c, d;

        for (i = 0; i < info::N_SLICE; i++) {
            c.setUDSlice(i);

            for (j = 0; j < info::N_MOVES; j += 3) {
                CubieCube::edgeMult(c, coords->moveCube[j], d);
                coords->UDSliceMove[i][j] = d.getUDSlice();
            }

            for (j = 0; j < info::SYM; j += 2) {
                edgeConjugate(c, coords->SymMultInv[0][j], d);
                coords->UDSliceConj[i][j >> 1] = d.getUDSlice();
            }
        }

        for (i = 0; i < info::N_SLICE; i++) {
            for (j = 0; j < info::N_MOVES; j += 3) {
                udslice = coords->UDSliceMove[i][j];

                for (k = 1; k < 3; k++) {
                    udslice = coords->UDSliceMove[udslice][j];
                    coords->UDSliceMove[i][j + k] = udslice;
                }
            }
        }
    }

    //init pruning
    void initAllPrun() {
        //coords->MCPermPrun
        initRawSymPrun(
                coords->MCPermPrun,
                coords->MPermMove, coords->MPermConj, coords->CPermMove,
                nullptr, nullptr, nullptr,
                SymStatePerm, 0x8ea34,
                info::N_MPERM * info::N_PERM_SYM / 8 + 1,
                MCPP_IDX);

        //PermCombPPrun
        initRawSymPrun(
                coords->EPermCCombPPrun,
                coords->CCombPMove, coords->CCombPConj, coords->EPermMove,
                nullptr, nullptr, nullptr,
                SymStatePerm, 0x7d824,
                info::N_COMB * info::N_PERM_SYM / 8 + 1,
                PCPP_IDX);

        //SliceTwistPrun
        initRawSymPrun(
                coords->UDSliceTwistPrun,
                nullptr, nullptr, nullptr,
                coords->UDSliceMove, coords->UDSliceConj, coords->TwistMove,
                SymStateTwist, 0x69603,
                info::N_SLICE * info::N_TWIST_SYM / 8 + 1,
                STP_IDX);

        //SliceFlipPrun
        initRawSymPrun(
                coords->UDSliceFlipPrun,
                nullptr, nullptr, nullptr,
                coords->UDSliceMove, coords->UDSliceConj, coords->FlipMove,
                SymStateFlip, 0x69603,
                info::N_SLICE * info::N_FLIP_SYM / 8 + 1,
                SFP_IDX);

        //coords->TwistFlipPrun
        initRawSymPrun(
                coords->TwistFlipPrun,
                nullptr, nullptr, nullptr,
                nullptr, nullptr, coords->TwistMove,
                SymStateTwist, 0x19603,
                info::N_FLIP * info::N_TWIST_SYM / 8 + 1,
                TFP_IDX);
    }

    //0 coords->MCPermPrun
    //[N_MPERM][N_MOVES2] [N_MPERM][SYM] [N_PERM_SYM][N_MOVES2] u8 u8 u16 RawMove1, RawConj1, SymMoveVect1
    //1 PermCombPPrun
    //[N_COMB][N_MOVES2] [N_COMB][SYM] [N_PERM_SYM[N_MOVES2] u8 u8 u16 RawMove1, RawConj1, SymMoveVect1
    //2 SliceTwistPrun
    //[N_SLICE][N_MOVES] [N_SLICE][SYM_CLASSES] [N_TWIST_SYM][N_MOVES] u16 u16 u16 RawMove2, RawConj2, SymMoveVect2
    //3 SliceFlipPrun
    //[N_SLICE][N_MOVES] [N_SLICE][SYM_CLASSES] [N_PERM_SYM][N_MOVES] u16 u16 u16 RawMove2, RawConj2, SymMoveVect2
    //4 coords->TwistFlipPrun
    //NO NO [N_TWIST_SYM][N_MOVES] u16 SymMoveVect2
    void initRawSymPrun(int32_t PrunTable[],
                                               uint8_t RawMove1[][info::N_MOVES2],
                                               uint8_t RawConj1[][info::SYM],
                                               uint16_t SymMoveVect1[][info::N_MOVES2],
                                               uint16_t RawMove2[][info::N_MOVES],
                                               uint16_t RawConj2[][info::SYM_CLASSES],
                                               uint16_t SymMoveVect2[][info::N_MOVES],
                                               const uint16_t SymState[], int32_t PrunFlag, int32_t PrunTableSize,
                                               const CoordType &type) {

        const bool IS_TYPE_A = type < STP_IDX;
        const int8_t SYM_SHIFT = PrunFlag & 0xf;
        const int32_t SYM_E2C_MAGIC_P = ((PrunFlag >> 4) & 1) == 1 ? SYM_E2C_MAGIC : 0x00000000;
        const bool IS_PHASE2 = ((PrunFlag >> 5) & 1) == 1;
        const int8_t INV_DEPTH = PrunFlag >> 8 & 0xf;
        const int8_t MAX_DEPTH = PrunFlag >> 12 & 0xf;

        const int8_t SYM_MASK = (1 << SYM_SHIFT) - 1;
        const bool ISTFP = type == TFP_IDX;
        const uint16_t N_RAW = ISTFP ? info::N_FLIP : (type == MCPP_IDX ? info::N_MPERM : (type == PCPP_IDX
                                                                                                  ? info::N_COMB
                                                                                                  : info::N_SLICE));
        const int32_t N_SIZE = N_RAW * (type == MCPP_IDX || type == PCPP_IDX ? info::N_PERM_SYM : type == SFP_IDX
                                                                                                      ? info::N_FLIP_SYM
                                                                                                      : info::N_TWIST_SYM);
        const int8_t N_MOVES = IS_PHASE2 ? info::N_MOVES2 : info::N_MOVES;
        const int32_t NEXT_AXIS_MAGIC = IS_PHASE2 ? 0x42 : 0x92492;

        int32_t mask, val, selArrMask, idx, idxx, i, m, done;
        int8_t check, xorVal, select, fsym, depth, prun;
        uint16_t flip, rawx, symx, symState;
        int16_t raw, symmetry;
        uint8_t j;
        bool inv;


        for (i = 0; i < N_SIZE / 8 + 1; i++)
            PrunTable[i] = 0x11111111;

        setPruning(PrunTable, 0, 0 ^ 1);

        depth = 0;
        done = 1;

        while (depth < MAX_DEPTH) {
            mask = (depth + 1) * 0x11111111 ^ 0xffffffff;

            for (i = 0; i < PrunTableSize; i++) {
                val = PrunTable[i] ^ mask;
                val &= val >> 1;
                PrunTable[i] += val & (val >> 2) & 0x11111111;
            }

            inv = depth > INV_DEPTH;
            select = inv ? (depth + 2) : depth;
            selArrMask = select * 0x11111111;
            check = inv ? depth : (depth + 2);
            depth++;
            xorVal = depth ^ (depth + 1);
            val = 0;

            for (i = 0; i < N_SIZE; i++, val >>= 4) { solver_poll();
                if ((i & 7) == 0) {
                    val = PrunTable[i >> 3];

                    if (!hasZero(val ^ selArrMask)) {
                        i += 7;
                        continue;
                    }
                }

                if ((val & 0xf) != select)
                    continue;

                raw = i % N_RAW;
                symmetry = i / N_RAW;
                flip = 0;
                fsym = 0;

                if (ISTFP) {
                    flip = coords->FlipR2S[raw];
                    fsym = flip & 7;
                    flip >>= 3;
                }

                for (m = 0; m < N_MOVES; m++) {
                    symx = IS_TYPE_A ? SymMoveVect1[symmetry][m] : SymMoveVect2[symmetry][m];

                    if (ISTFP)
                        rawx = coords->FlipS2RF[
                                coords->FlipMove[flip][coords->Sym8Move[m << 3 | fsym]] ^
                                fsym ^ (symx & SYM_MASK)];
                    else
                        rawx = IS_TYPE_A ?
                               RawConj1[RawMove1[raw][m]][symx & SYM_MASK] :
                               RawConj2[RawMove2[raw][m]][symx & SYM_MASK];

                    symx >>= SYM_SHIFT;
                    idx = symx * N_RAW + rawx;
                    prun = getPruning(PrunTable, idx);

                    if (prun != check) {
                        if (prun < depth - 1)
                            m += NEXT_AXIS_MAGIC >> m & 3;
                        continue;
                    }

                    done++;

                    if (inv) {
                        setPruning(PrunTable, i, xorVal);
                        break;
                    }

                    setPruning(PrunTable, idx, xorVal);

                    for (j = 1, symState = SymState[symx]; (symState >>= 1) != 0; j++) {
                        if ((symState & 1) != 1)
                            continue;

                        idxx = symx * N_RAW;

                        if (ISTFP)
                            idxx += coords->FlipS2RF[coords->FlipR2S[rawx] ^ j];
                        else
                            idxx += IS_TYPE_A ? RawConj1[rawx][j ^ (SYM_E2C_MAGIC_P >> (j << 1) & 3)] :
                                    RawConj2[rawx][j ^ (SYM_E2C_MAGIC_P >> (j << 1) & 3)];

                        if (getPruning(PrunTable, idxx) == check) {
                            setPruning(PrunTable, idxx, xorVal);
                            done++;
                        }
                    }
                }
            }
        }
    }

    void CoordCube::calcPrun(bool isPhase1) {
        prun = std::max(
                std::max(
                        getPruning(coords->UDSliceTwistPrun,
                                   twist * info::N_SLICE + coords->UDSliceConj[slice][tsym]),
                        getPruning(coords->UDSliceFlipPrun,
                                   flip * info::N_SLICE + coords->UDSliceConj[slice][fsym])),
                std::max(
                         getPruning(coords->TwistFlipPrun,
                                                          (twistc >> 3) << 11 | coords::coords->FlipS2RF[flipc ^ (twistc & 7)]),
                        getPruning(coords->TwistFlipPrun,
                                                                twist << 11 | coords::coords->FlipS2RF[flip << 3 | (fsym ^ tsym)])));
    }

    bool CoordCube::setWithPrun(const CubieCube &cc, int8_t depth) {
        CubieCube pc;
        twist = cc.getTwistSym();
        flip = cc.getFlipSym();
        tsym = twist & 7;
        twist = twist >> 3;

        prun = getPruning(coords->TwistFlipPrun, twist << 11 | coords->FlipS2RF[flip ^ tsym]);

        if (prun > depth)
            return false;

        fsym = flip & 7;
        flip = flip >> 3;

        slice = cc.getUDSlice();
        prun = std::max(prun, std::max(
                getPruning(coords->UDSliceTwistPrun, twist * info::N_SLICE + coords->UDSliceConj[slice][tsym]),
                getPruning(coords->UDSliceFlipPrun, flip * info::N_SLICE + coords->UDSliceConj[slice][fsym])));

        if (prun > depth)
            return false;

        cornConjugate(cc, 1, pc);
        edgeConjugate(cc, 1, pc);
        twistc = pc.getTwistSym();
        flipc = pc.getFlipSym();

        prun = std::max(prun,
                        getPruning(coords->TwistFlipPrun, (twistc >> 3) << 11 | coords->FlipS2RF[flipc ^ (twistc & 7)]));

        return prun <= depth;
    }

    //move cube pruning
    int8_t CoordCube::doMovePrun(const CoordCube &cc, int8_t m) {
        slice = coords->UDSliceMove[cc.slice][m];

        flip = coords->FlipMove[cc.flip][coords->Sym8Move[m << 3 | cc.fsym]];
        fsym = (flip & 7) ^ cc.fsym;
        flip >>= 3;

        twist = coords->TwistMove[cc.twist][coords->Sym8Move[m << 3 | cc.tsym]];
        tsym = (twist & 7) ^ cc.tsym;
        twist >>= 3;

        prun = std::max(
                std::max(
                        getPruning(coords->UDSliceTwistPrun, twist * info::N_SLICE + coords->UDSliceConj[slice][tsym]),
                        getPruning(coords->UDSliceFlipPrun, flip * info::N_SLICE + coords->UDSliceConj[slice][fsym])),
                getPruning(coords->TwistFlipPrun, twist << 11 | coords->FlipS2RF[flip << 3 | (fsym ^ tsym)]));

        return prun;
    }

    //move cube UDSliceSorted
    int8_t CoordCube::doMovePrunConj(const CoordCube &cc, int8_t m) {
        m = coords->SymMove[3][m];
        flipc = coords->FlipMove[cc.flipc >> 3][coords->Sym8Move[m << 3 | cc.flipc & 7]] ^ (cc.flipc & 7);
        twistc = coords->TwistMove[cc.twistc >> 3][coords->Sym8Move[m << 3 | cc.twistc & 7]] ^ (cc.twistc & 7);

        return getPruning(coords->TwistFlipPrun, (twistc >> 3) << 11 | coords->FlipS2RF[flipc ^ (twistc & 7)]);
    }
} }

