/**
 * min2phaseCXX Copyright (C) 2022 Borgo Federico
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 *
 * This file contains the class that contains a cube.
 * It has method that computes the coordinates to increase the speed of the algorithm.
 * This class contains also the output formatter for the algorithm.
 */

#ifndef MIN2PHASE_CUBIECUBE_H
#define MIN2PHASE_CUBIECUBE_H 1

#include <string>
#include "info.h"

namespace min2phase {
    /**
     * This class is used to store the cube: the order of
     * edges and corner.
     * It is also used to compute some useful coordinate
     * necessary at runtime.
     * This class is used to generate the output string.
     * Inside that there are the necessary moves to solve the cube.
     */
    class CubieCube {
    public:

        /**
         * This is used to store the permutation and orientation of the edges
         * of the cube.
         */
        int8_t edges[info::NUMBER_EDGES] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22};

        /**
         * This is used to store the permutation and orientation of the corners
         * of the cube.
         */
        int8_t corners[info::NUMBER_CORNER] = {0, 1, 2, 3, 4, 5, 6, 7};

        /**
         * This constructor is used to create a cube with solved edges and corners
         * and not generated coordinates.
         */
        CubieCube() = default;

        /**
         * This constructor is used to create a cube from coordinates edges and corners
         * and not generated coordinates.
         */
        CubieCube(uint16_t cPerm, int16_t cOri, int32_t ePerm, int16_t eOri);

        /**
         * This method is used to recreate a cube from a permutation ad orientation
         * stored in coordinates. It converts the coordinates to cube.
         *
         * @param cPerm  : the corner permutation.
         * @param cOri   : the corner orientation.
         * @param ePerm  : the edge permutation.
         * @param eOri   : the edge orientation.
         */
        void setCoords(uint16_t cPerm, int16_t cOri, int32_t ePerm, int16_t eOri);

        /**
         * This method is used to create a new cube from an existing one.
         * It copy the old cube and paste is in the new one. It also needs the
         * object of coordinates.
         *
         * @param cube   : the cube where is stored the old cube to copy.
         */
        void copy(const CubieCube &cube);

        /**
         * This method create a inverse cube from the object.
         * It changes the edge and corner position.
         */
        void inv();

        /**
         * This method is used to apply the S_URF symmetry. It creates a new cube
         * with the URF transformation and replace it in the object.
         * this = S_urf^-1 * this * S_urf.
         */
        void URFConjugate();

        /**
         * This method is used to get the symmetry of the object.
         *
         * @return : the symmetry.
         */
        int64_t selfSym() const;

        /**
         * This function check if the cube is possible.
         * The cube analyzed is the one store in the object of the method.
         *
         * @return : a value that indicates the error, see info.
         */
        info::Errors check() const;

        /**
         * This function is used to convert an order of face of cube into
         * a cube object.
         *
         * @param f     : the array of the color of the cube to convert.
         * @param ccRet : the cube to store the new cube created.
         */
        static void toCubieCube(const int8_t f[], CubieCube &ccRet);

        /**
         * This method is used to convert a cube with coordinates to a string.
         *
         * @param cc : the cube to convert.
         * @return   : the string that represent the cube.
         */
        static std::string toFaceCube(const CubieCube &cc);

        /**
         * This method is used to compute the result of a * b edge only.
         *
         * @param a    : cube with permutation.
         * @param b    : cube with orientation.
         * @param prod : the result cube reference.
         */
        static void edgeMult(const CubieCube &a, const CubieCube &b, CubieCube &prod);

        /**
         * This method is used to compute the result of a * b corner only.
         *
         * @param a    : cube with permutation.
         * @param b    : cube with orientation.
         * @param prod : the result cube reference.
         */
        static void cornMult(const CubieCube &a, const CubieCube &b, CubieCube &prod);

        /**
         * This method is used to compute the result of a * b corner with mirrored cases.
         *
         * @param a    : cube with permutation.
         * @param b    : cube with orientation.
         * @param prod : the result cube reference.
         */
        static void cornMultFull(const CubieCube &a, const CubieCube &b, CubieCube &prod);

    private:

        /**
         * This method is used to extract the edge or corner from a value.
         *
         * @param val0   : the value where the edge or corner is stored.
         * @param isEdge : true if the value is from and edge, false if it's a corner.
         * @return       : the extracted value.
         */
        static int8_t getVal(int8_t val0, bool isEdge);

        /**
         * This method is used to set the edge or corner into a value.
         *
         * @param val0   : the value where the corner or edge will be stored.
         * @param val    : the edge or corner index.
         * @param isEdge : true if the val is and edge, false if it's a corner.
         * @return       : the value with the corner or edge stored.
         */
        static int8_t setVal(int8_t val0, int8_t val, bool isEdge);

        /**
         * This method is used to set the permutation of the corner of edges
         * into an array.
         *
         * @param arr    : the array to store the order of corners and edges.
         * @param idx    : the coordinate index of permutation to convert.
         * @param n      : the size of the array.
         * @param isEdge : true if the array contains edges, false if corners.
         */
        static void setNPerm(int8_t arr[], int32_t idx, uint8_t n, bool isEdge);

        /**
         * This method is used to convert an array of corners of edges to a
         * coordinate of the permutation.
         *
         * @param arr    : the array that contains the order of corners and edges.
         * @param n      : the size of the array.
         * @param isEdge : true if the array contains edges, false if corners.
         * @return       : the coordinate permutation.
         */
        static int32_t getNPerm(const int8_t arr[], uint8_t n, bool isEdge);

        /**
         * This method is used to set the order of edges or corners into an array
         * from the coordinate value. The coordinate indicates the parity of the edges
         * or corners.
         * The range of this coordinate is [0, 494).
         *
         * @param arr    : the array that contains the order of corners or edges.
         * @param idxC   : the coordinate of the edges or corners.
         * @param mask   : the mask used to compute the coordinate.
         * @param isEdge : true if the array contains edges, false if corners.
         */
        static void setComb(int8_t arr[], int16_t idxC, uint8_t mask, bool isEdge);

    public:

        /**
         * This method is used to convert the parity of four corner of edges into a
         * coordinate.
         * The range of this coordinate is [0, 494).
         *
         * @param arr    : the array that contains the order of corners or edges.
         * @param mask   : the mask used to compute the coordinate.
         * @param isEdge : true if the array contains edges, false if corners.
         * @return       : the coordinate of the parity of edges or corners.
         */
        static int16_t getComb(const int8_t arr[], uint8_t mask, bool isEdge);

        /**
         * This method is used to get the parity of a permutation.
         *
         * @param idx : the coordinate.
         * @param n   : the position.
         * @return    : the parity.
         */
        static int8_t getNParity(uint32_t idx, uint8_t n);

        ///Phase 1 coordinates

        /**
         * This method is used to get the edge orientation of the object.
         * The range of this coordinate is [0, 2048).
         *
         * @return : the coordinate of the this cube.
         */
        int16_t getFlip() const;

        /**
         * This method is used to set the edge orientation from a coordinate.
         * The range of this coordinate is [0, 2048).
         *
         * @param eOri : the edge orientation coord to set.
         */
        void setFlip(int16_t eOri);

        /**
         * This method is used to get the symmetry of the edge orientation.
         * The range of this symmetry is [0, 336 * 8).
         *
         * @return : the symmetry of the edge orientation.
         */
        int16_t getFlipSym() const;


        /**
         * This method is used to get the corner orientation of the object.
         * The range of this coordinate is [0, 2187).
         *
         * @return : the coordinate of the this cube.
         */
        int16_t getTwist() const;

        /**
         * This method is used to set the corner orientation from a coordinate.
         * The range of this coordinate is [0, 2187).
         *
         * @param cOri : the corner orientation coord to set.
         */
        void setTwist(int16_t cOri);

        /**
         * This method is used to get the symmetry of the corner orientation.
         * The range of this symmetry is [0, 324 * 8).
         *
         * @return : the symmetry of the corner orientation.
         */
        int16_t getTwistSym() const;

        /**
         * This method return the coordinate from the object that indicate the
         * positions of the 4 UDSlice edges, the order is ignored.
         * The range of this coordinate is [0, 495).
         *
         * @return : the UDSlice edges order coordinate.
         */
        int16_t getUDSlice() const;

        /**
         * This method is used to set the coordinate that indicate the positions
         * of the 4 UDSlice edges, the order is ignored from a coordinate.
         * The range of this coordinate is [0, 495).
         *
         * @param udSlice : the UDSlice edges order coordinate.
         */
        void setUDSlice(int16_t udSlice);

        ///Phase 2.

        /**
         * This method is used to get the corner permutation from the object.
         * The range of this coordinate is [0, 40320).
         *
         * @return : the corner permutation orientation
         */
        uint16_t getCPerm() const;

        /**
         * This method is used to set the corner permutation from a coordinate.
         * The range of this coordinate is [0, 40320).
         *
         * @param perm : the corner permutation coordinate.
         */
        void setCPerm(uint16_t perm);

        /**
         * This method is used to get the symmetry of corner permutation.
         * The range of this symmetry is [0, 2187 * 16).
         *
         * @return : the symmetry of the corner permutation.
         */
        uint16_t getCPermSym() const;


        /**
         * This method is used to get the edge permutation from the object.
         * The range of this coordinate is [0, 40320).
         *
         * @return : the edge permutation orientation
         */
        uint16_t getEPerm() const;

        /**
         * This method is used to set the edge permutation from a coordinate.
         * The range of this coordinate is [0, 40320).
         *
         * @param perm : the edge permutation coordinate.
         */
        void setEPerm(uint16_t idx);

        /**
         * This method is used to get the symmetry of edge permutation
         * from the object.
         * The range of this symmetry is [0, 2187 * 16).
         *
         * @return : the symmetry of the edge permutation.
         */
        uint16_t getEPermSym() const;


        /**
         * This method is used to get the permutation of the 4 edges in the
         * middle slice from the object.
         * The range og this coordinate is [0, 24).
         *
         * @return : the 4 edge in the middle slice coordinate.
         */
        int8_t getMPerm() const;

        /**
         * This method is used to set the permutation of the 4 edges in the
         * middle slice.
         * The range og this coordinate is [0, 24).
         *
         * @param idx : the 4 edge in the middle slice coordinate.
         */
        void setMPerm(int8_t idx);

        /**
         * This method is used to get the parity of the 4 corners.
         * The range of this coordinate is [0, 140).
         *
         * @return : the coordinate of the corners of the object.
         */
        uint8_t getCComb() const;

        /**
         * This method is used to set the coordinate of the 4 corners
         * from the coordinate.
         * The range of this coordinate is [0, 140).
         *
         * @param idx : the coordinate to set at the object.
         */
        void setCComb(uint8_t idx);


        /**
         * This class is used to generate the output string.
         * Inside that there are the necessary moves to solve the cube.
         */
        class OutputFormat {

        private:

            /**
             * This is used to convert a integer move into a string one.
             */
            static constexpr char move2str[info::N_MOVES][3] = {
                "U ", "U2", "U'", "R ", "R2", "R'", "F ", "F2", "F'",
                        "D ", "D2", "D'", "L ", "L2", "L'", "B ", "B2", "B'"
            };

            /**
             * The number of moves used in phase 1.
             */
            int8_t depth1 = 0;

            /**
             * This is used the format of the output string.
             */
            int8_t format = 0;

            /**
             * The index of symmetry.
             */
            int8_t urfIdx = 0;

            /**
             * In this array are stored all the moved used to solve the cube.
             */
            int8_t moves[info::MAX_LENGTH]{};

        public:

            OutputFormat() = default;

            /**
             * The number of moved used to solve the cube.
             */
            int8_t length = 0;

            /**
             * This indicate if a solution is found.
             */
            bool isFound = false;

            /**
             * This method is used to reset the output formatting.
             */
            void reset();

            /**
             * This method is used to configure the information necessary to generate
             * the output string.
             *
             * @param format : the format used for the outputString.
             * @param urfIdx : the index of symmetry.
             * @param depth1 : the number of moves used in phase 1.
             */
            void setArgs(int8_t format, int8_t urfIdx, int8_t depth1);

            /**
             * This method is used to add a move for the output string.
             *
             * @param curMove : the moved to store.
             */
            void appendSolMove(int8_t curMove);

            /**
             * Number of motor time slots. Adjacent moves on opposite faces can
             * run at the same time and therefore count as one slot.
             */
            int8_t parallelLength() const;

            /**
             * This is used to convert all the information into a string.
             *
             * @return : the string with the moved used to solve the cube.Delete the return value when
             *           you don't need it anymore.
             */
            std::string toString();
        };
    };
}

#endif //MIN2PHASE_CUBIECUBE_H
