/**
 * min2phaseCXX Copyright (C) 2022 Borgo Federico
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 */

#include "coords.h"
#include "min2phase.h"
#include <memory>
#include <cstring>

namespace min2phase {

    CubieCube::CubieCube(uint16_t cPerm, int16_t cOri, int32_t ePerm, int16_t eOri) {
        setCoords(cPerm, cOri, ePerm, eOri);
    }

    //set che coordinates of the cube
    void CubieCube::setCoords(uint16_t cPerm, int16_t cOri, int32_t ePerm, int16_t eOri) {
        setCPerm(cPerm);
        setTwist(cOri);
        setNPerm(edges, ePerm, info::NUMBER_EDGES, true);
        setFlip(eOri);
    }

    //set the cube from an existing one and coords
    void CubieCube::copy(const CubieCube &cube) {
        std::memcpy(this->corners, cube.corners, sizeof(corners));
        std::memcpy(this->edges, cube.edges, sizeof(edges));
    }

    //inverse cube
    void CubieCube::inv() {
        int8_t edge, corner;
        CubieCube temps;

        for (edge = 0; edge < info::NUMBER_EDGES; edge++)
            temps.edges[edges[edge] >> 1] = (edge << 1 | edges[edge] & 1);

        for (corner = 0; corner < info::NUMBER_CORNER; corner++)
            temps.corners[corners[corner] & 0x7] = (corner | 0x20 >> (corners[corner] >> 3) & 0x18);

        copy(temps);
    }

    //this = S_urf^-1 * this * S_urf
    void CubieCube::URFConjugate() {
        CubieCube temps;

        cornMult(coords::coords->urfInv, *this, temps);
        cornMult(temps, coords::coords->urf, *this);
        edgeMult(coords::coords->urfInv, *this, temps);
        edgeMult(temps, coords::coords->urf, *this);
    }

    int64_t CubieCube::selfSym() const {
        CubieCube c(*this);
        CubieCube d;
        int8_t urfInv;
        uint8_t i;
        uint16_t cpermx, cperm = c.getCPermSym() >> 4;
        int64_t symType = 0L;

        for (urfInv = 0; urfInv < info::N_BASIC_MOVES; urfInv++) {
            cpermx = c.getCPermSym() >> 4;

            if (cperm == cpermx) {
                for (i = 0; i < info::SYM; i++) {
                    coords::cornConjugate(c, coords::coords->SymMultInv[0][i], d);

                    if (coords::isSameCube(d.corners, corners, false)) {
                        coords::edgeConjugate(c, coords::coords->SymMultInv[0][i], d);

                        if (coords::isSameCube(d.edges, edges, true))
                            symType |= int64_t(1) << std::min(int64_t(urfInv << 4 | i), int64_t (info::FULL_SYM));
                    }
                }
            }

            c.URFConjugate();

            if (urfInv % 3 == 2)
                c.inv();
        }

        return symType;
    }

    //check if the cube is possible
    info::Errors CubieCube::check() const {
        uint8_t i;
        uint16_t sum = 0;
        uint16_t mask = 0;

        for (i = 0; i < info::NUMBER_EDGES; i++) {
            mask |= 1 << (edges[i] >> 1);
            sum ^= edges[i] & 1;
        }

        if (mask != 0xfff)
            return info::MISSING_EDGE;

        if (sum != 0)
            return info::FLIPPED_EDGE;

        mask = 0;
        sum = 0;

        for (i = 0; i < info::NUMBER_CORNER; i++) {
            mask |= 1 << (corners[i] & (info::NUMBER_CORNER - 1));
            sum += corners[i] >> info::CORNE_ORI_CASES;
        }

        if (mask != 0xff)
            return info::MISSING_CORNER;

        if (sum % info::CORNE_ORI_CASES != 0)
            return info::TWISTED_CORNER;

        if ((getNParity(getNPerm(edges, info::NUMBER_EDGES, true), info::NUMBER_EDGES) ^
             getNParity(getCPerm(), info::NUMBER_CORNER)) != 0)
            return info::PARITY_ERROR;

        return info::NO_ERROR;
    }

    //convert the array of colors into a cube object
    void CubieCube::toCubieCube(const int8_t f[], CubieCube &ccRet) {
        int8_t ori, i, j, col1, col2;

        for (i = 0; i < info::NUMBER_CORNER; i++)
            ccRet.corners[i] = 0;

        for (i = 0; i < info::NUMBER_EDGES; i++)
            ccRet.edges[i] = 0;

        for (i = 0; i < info::NUMBER_CORNER; i++) {
            for (ori = 0; ori < info::CORNE_ORI_CASES; ori++)
                if (f[info::cornerFacelet[i][ori]] == info::U ||
                    f[info::cornerFacelet[i][ori]] == info::D)
                    break;

            col1 = f[info::cornerFacelet[i][(ori + 1) % info::CORNE_ORI_CASES]];
            col2 = f[info::cornerFacelet[i][(ori + 2) % info::CORNE_ORI_CASES]];

            for (j = 0; j < info::NUMBER_CORNER; j++) {
                if (col1 == info::cornerFacelet[j][1] / 9 && col2 == info::cornerFacelet[j][2] / 9) {
                    ccRet.corners[i] = (int8_t)(ori % info::CORNE_ORI_CASES << info::CORNE_ORI_CASES | j);
                    break;
                }
            }
        }

        for (i = 0; i < info::NUMBER_EDGES; i++) {
            for (j = 0; j < info::NUMBER_EDGES; j++) {
                if (f[info::edgeFacelet[i][0]] == info::edgeFacelet[j][0] / 9
                    && f[info::edgeFacelet[i][1]] == info::edgeFacelet[j][1] / 9) {
                    ccRet.edges[i] = j << 1;
                    break;
                }

                if (f[info::edgeFacelet[i][0]] == info::edgeFacelet[j][1] / 9
                    && f[info::edgeFacelet[i][1]] == info::edgeFacelet[j][0] / 9) {
                    ccRet.edges[i] = j << 1 | 1;
                    break;
                }
            }
        }
    }

    //from cube to string
    std::string CubieCube::toFaceCube(const CubieCube &cc) {
        int8_t i;
        int8_t j, ori;
        char f[info::N_PLATES+1];

        char ts[] = {'U', 'R', 'F', 'D', 'L', 'B'};

        for (i = 0; i < info::N_PLATES; i++)
            f[i] = ts[i / info::N_PLATES_X_FACE];

        for (i = 0; i < info::NUMBER_CORNER; i++) {
            j = cc.corners[i] & 0x7;
            ori = cc.corners[i] >> 3;
            for (int8_t n = 0; n < info::CORNE_ORI_CASES; n++) {
                f[info::cornerFacelet[i][(n + ori) % info::CORNE_ORI_CASES]] = ts[
                        info::cornerFacelet[j][n] / info::N_PLATES_X_FACE];
            }
        }

        for (i = 0; i < info::NUMBER_EDGES; i++) {
            j = cc.edges[i] >> 1;
            ori = cc.edges[i] & 1;
            for (int8_t n = 0; n < info::EDGE_ORI_CASES; n++) {
                f[info::edgeFacelet[i][(n + ori) % info::EDGE_ORI_CASES]] = ts[info::edgeFacelet[j][n] /
                                                                               info::N_PLATES_X_FACE];
            }
        }

        f[info::N_PLATES] = '\0';

        return std::string(f);
    }

    // a * b edge only
    void CubieCube::edgeMult(const CubieCube &a, const CubieCube &b, CubieCube &prod) {
        for (int8_t edge = 0; edge < info::NUMBER_EDGES; edge++)
            prod.edges[edge] = a.edges[b.edges[edge] >> 1] ^ (b.edges[edge] & 1);
    }

    //a * b corner only
    void CubieCube::cornMult(const CubieCube &a, const CubieCube &b, CubieCube &prod) {
        int8_t oriA, oriB, corn;

        for (corn = 0; corn < info::NUMBER_CORNER; corn++) {
            oriA = a.corners[b.corners[corn] & 7] >> 3;
            oriB = b.corners[corn] >> 3;
            prod.corners[corn] = a.corners[b.corners[corn] & 7] & 7 | (oriA + oriB) % 3 << 3;
        }
    }

    //a * b corner with mirrored cases
    void CubieCube::cornMultFull(const CubieCube &a, const CubieCube &b, CubieCube &prod) {
        int8_t oriA, oriB, corn, ori;

        for (corn = 0; corn < info::NUMBER_CORNER; corn++) {
            oriA = a.corners[b.corners[corn] & 7] >> 3;
            oriB = b.corners[corn] >> 3;
            ori = oriA + ((oriA < 3) ? oriB : 6 - oriB);
            ori = ori % 3 + ((oriA < 3) == (oriB < 3) ? 0 : 3);
            prod.corners[corn] = a.corners[b.corners[corn] & 7] & 7 | ori << 3;
        }
    }

    //get edge or corner from value
    int8_t CubieCube::getVal(int8_t val0, bool isEdge) {
        return isEdge ? val0 >> 1 : val0 & 7;
    }

    //get edge or corner into value
    int8_t CubieCube::setVal(int8_t val0, int8_t val, bool isEdge) {
        return (isEdge ? val << 1 | (val0 & 1) : val | (val0 & ~7));

    }

    //set permutation
    void CubieCube::setNPerm(int8_t arr[], int32_t idx, uint8_t n, bool isEdge) {
        uint8_t i;
        uint64_t val = 0xFEDCBA9876543210L;
        int64_t v;
        int64_t m, extract = 0;

        for (i = 2; i <= n; i++) {
            extract = extract << 4 | idx % i;
            idx /= i;
        }

        for (i = 0; i < n - 1; i++) {
            v = (extract & 0xf) << 2;
            extract >>= 4;
            arr[i] = setVal(arr[i], val >> v & 0xf, isEdge);

            m = (int64_t(1) << v) - 1;
            val = (val & m) | (val >> 4 & ~m);
        }

        arr[n - 1] = setVal(arr[n - 1], val & 0xf, isEdge);
    }

    //get permutation
    int32_t CubieCube::getNPerm(const int8_t arr[], uint8_t n, bool isEdge) {
        uint8_t i, v;
        int32_t idx = 0;
        uint64_t val = 0xFEDCBA9876543210L;

        for (i = 0; i < n - 1; i++) {
            v = getVal(arr[i], isEdge) << 2;
            idx = (n - i) * idx + (val >> v & 0xf);
            val -= 0x1111111111111110L << v;
        }

        return idx;
    }

    //set the parity of corners and edges
    void CubieCube::setComb(int8_t arr[], int16_t idxC, uint8_t mask, bool isEdge) {
        int8_t fill = isEdge ? info::NUMBER_EDGES - 1 : info::NUMBER_CORNER - 1;
        int8_t i;
        int8_t r = 4;

        for (i = fill; i >= 0; i--) {
            if (idxC >= info::Cnk[i][r]) {
                idxC -= info::Cnk[i][r--];
                arr[i] = setVal(arr[i], r | mask, isEdge);
            } else {
                if ((fill & 0xc) == mask)
                    fill -= 4;

                arr[i] = setVal(arr[i], fill--, isEdge);
            }
        }
    }

    //get the parity of corners or edges
    int16_t CubieCube::getComb(const int8_t arr[], uint8_t mask, bool isEdge) {
        int8_t i, perm, r = 4;
        int16_t idxC = 0;

        for (i = isEdge ? info::NUMBER_EDGES - 1 : info::NUMBER_CORNER - 1; i >= 0; i--) {
            perm = getVal(arr[i], isEdge);

            if ((perm & 0xc) == mask)
                idxC += info::Cnk[i][r--];
        }

        return idxC;
    }

    //get the parity permutation
    int8_t CubieCube::getNParity(uint32_t idx, uint8_t n) {
        int8_t i;
        int8_t p = 0;

        for (i = n - 2; i >= 0; i--) {
            p ^= idx % (n - i);
            idx /= (n - i);
        }

        return p & 1;
    }

    //phase 1 coords

    //get edge orientation
    int16_t CubieCube::getFlip() const {
        int16_t eOri = 0;
        uint8_t i;

        for (i = 0; i < info::NUMBER_EDGES - 1; i++)
            eOri = eOri << 1 | edges[i] & 1;

        return eOri;
    }

    //set edge orientation
    void CubieCube::setFlip(int16_t eOri) {
        int8_t parity = 0;
        int16_t val;
        int8_t i;

        for (i = info::NUMBER_EDGES - 2; i >= 0; i--, eOri >>= 1) {
            parity ^= (val = eOri & 1);
            edges[i] = (int8_t)(edges[i] & ~1 | val);
        }

        edges[info::NUMBER_EDGES - 1] = (int8_t)(edges[info::NUMBER_EDGES - 1] & ~1 | parity);
    }

    //get edge orientation symmetry
    int16_t CubieCube::getFlipSym() const {
        return coords::coords->FlipR2S[getFlip()];
    }

    //get corner orientation
    int16_t CubieCube::getTwist() const {
        int16_t cOri = 0;
        uint8_t i;

        for (i = 0; i < info::NUMBER_CORNER - 1; i++)
            cOri += (cOri << 1) + (corners[i] >> info::CORNE_ORI_CASES);

        return cOri;
    }

    //set corner orientation
    void CubieCube::setTwist(int16_t cOri) {
        int8_t i;
        int8_t ori = 15, val;

        for (i = info::NUMBER_CORNER - 2; i >= 0; i--, cOri /= info::CORNE_ORI_CASES) {
            ori -= (val = cOri % info::CORNE_ORI_CASES);
            corners[i] = corners[i] & 0x7 | val << info::CORNE_ORI_CASES;
        }

        corners[info::NUMBER_CORNER - 1] = (corners[info::NUMBER_CORNER - 1] & 0x7) |
                                           ((ori % info::CORNE_ORI_CASES) << info::CORNE_ORI_CASES);
    }

    //get corner orientation symmetry
    int16_t CubieCube::getTwistSym() const {
        return coords::coords->TwistR2S[getTwist()];
    }

    //get udslice coord
    int16_t CubieCube::getUDSlice() const {
        return info::N_SLICE - 1 - getComb(edges, info::NUMBER_CORNER, true);
    }

    //set udslice coord
    void CubieCube::setUDSlice(int16_t udSlice) {
        setComb(edges, info::N_SLICE - 1 - udSlice, info::NUMBER_CORNER, true);
    }

    //phase 2 coords

    //get corner permutation
    uint16_t CubieCube::getCPerm() const {
        return getNPerm(corners, info::NUMBER_CORNER, false);
    }

    //set corner permutation
    void CubieCube::setCPerm(uint16_t perm) {
        setNPerm(corners, perm, info::NUMBER_CORNER, false);
    }

    //get corner permutation symmetry
    uint16_t CubieCube::getCPermSym() const {
        return coords::ESym2CSym(coords::coords->EPermR2S[getCPerm()]);
    }

    //get edge permutation
    uint16_t CubieCube::getEPerm() const {
        return getNPerm(edges, info::NUMBER_CORNER, true);
    }

    //set edge permutation
    void CubieCube::setEPerm(uint16_t idx) {
        setNPerm(edges, idx, info::NUMBER_EDGES - 4, true);
    }

    //get edge permutation symmetry
    uint16_t CubieCube::getEPermSym() const {
        return coords::coords->EPermR2S[getEPerm()];
    }

    //get udslicesorted permutation
    int8_t CubieCube::getMPerm() const {
        return getNPerm(edges, info::NUMBER_EDGES, true) % info::N_MPERM;
    }

    //set udslicesorted permutation
    void CubieCube::setMPerm(int8_t idx) {
        setNPerm(edges, idx, info::NUMBER_EDGES, true);
    }

    //get corner symmetry
    uint8_t CubieCube::getCComb() const {
        return (uint8_t)getComb(corners, 0, false);
    }

    //set corner symmetry
    void CubieCube::setCComb(uint8_t idx) {
        setComb(corners, idx, 0, false);
    }

    char constexpr CubieCube::OutputFormat::move2str[info::N_MOVES][3];

    //reset output
    void CubieCube::OutputFormat::reset() {
        format = 0;
        urfIdx = 0;
        depth1 = 0;
        length = 0;
    }

    //set data
    void CubieCube::OutputFormat::setArgs(int8_t format, int8_t urfIdx, int8_t depth1) {
        MIN2PHASE_OUTPUT("Solution output set.")
        this->format = format;
        this->urfIdx = urfIdx;
        this->depth1 = depth1;
    }

    //add move
    void CubieCube::OutputFormat::appendSolMove(int8_t curMove) {
        int8_t axisCur, axisLast, pow;

        if (length == 0) {
            moves[length++] = curMove;
            return;
        }

        axisCur = curMove / info::N_GROUP_MOVES;
        axisLast = moves[length - 1] / info::N_GROUP_MOVES;

        if (axisCur == axisLast) {
            pow = (curMove % info::N_GROUP_MOVES + moves[length - 1] % info::N_GROUP_MOVES + 1) % 4;

            if (pow == info::N_GROUP_MOVES)
                length--;
            else
                moves[length - 1] = axisCur * info::N_GROUP_MOVES + pow;

            return;
        }

        if (length > 1 && axisCur % info::N_GROUP_MOVES == axisLast % info::N_GROUP_MOVES &&
            axisCur == moves[length - 2] / info::N_GROUP_MOVES) {
            pow = (curMove % info::N_GROUP_MOVES + moves[length - 2] % info::N_GROUP_MOVES + 1) % 4;

            if (pow == info::N_GROUP_MOVES) {
                moves[length - 2] = moves[length - 1];
                length--;
            } else
                moves[length - 2] = axisCur * info::N_GROUP_MOVES + pow;

            return;
        }

        moves[length++] = curMove;
    }

    int8_t CubieCube::OutputFormat::parallelLength() const {
        int8_t slots = 0;
        for (int8_t i = 0; i < length; i++) {
            ++slots;
            if (i + 1 < length) {
                const int8_t firstFace = moves[i] / info::N_GROUP_MOVES;
                const int8_t secondFace = moves[i + 1] / info::N_GROUP_MOVES;
                if ((firstFace + 3) % info::FACES == secondFace)
                    ++i;
            }
        }
        return slots;
    }

    //compute the string
    std::string CubieCube::OutputFormat::toString() {
        std::string solutionStr;
        char* solution = new char[1+length*3 +((format & USE_SEPARATOR) != 0? 2: 0)+((format & APPEND_LENGTH) != 0? 5: 0)];
        uint8_t solutionPos;
        int8_t urf = (format & INVERSE_SOLUTION) != 0 ? (urfIdx + info::N_GROUP_MOVES) % 6 : urfIdx;
        const bool useInv = urf < info::N_GROUP_MOVES;
        int8_t s;

        MIN2PHASE_OUTPUT("Converting to string.")

        solutionPos = 0;

        for (s = useInv? 0: length-1; useInv? s < length: s >= 0; useInv? s++: s--) {
            if (useInv && s == depth1 && (format & USE_SEPARATOR) != 0){
                solution[solutionPos++] = '.';
                solution[solutionPos++] = ' ';
            }

            solution[solutionPos++] = move2str[info::urfMove[urf][moves[s]]][0];
            solution[solutionPos++] = move2str[info::urfMove[urf][moves[s]]][1];

            solution[solutionPos++] = ' ';

            if (!useInv && (format & USE_SEPARATOR) != 0 && s == depth1){
                solution[solutionPos++] = '.';
                solution[solutionPos++] = ' ';
            }
        }

        if ((format & APPEND_LENGTH) != 0){
            solution[solutionPos++] = '(';

            if(length/10 != 0)
                solution[solutionPos++] = '0'+length/10;

            solution[solutionPos++] = '0'+(length%10);
            solution[solutionPos++] = 'f';
            solution[solutionPos++] = ')';
        }

        solution[solutionPos] = '\0';

        solutionStr = solution;

        delete[] solution;

        MIN2PHASE_OUTPUT("Sol found: " + solutionStr);

        return solutionStr;
    }
}

