/**
 *
 * class: gateLevelCkt
 * Original Source: lsim_3val.cpp
 * Original Author: Dr. Michael S. Hsiao
 *
 *
 *
 * Modified by: Sonal Pinto
 * 
 * Describes all attributes and functions relevant to a gate netlist
 * 
 */


#ifndef GATELEVELCKT_H
#define GATELEVELCKT_H


#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <vector>
#include <array>
#include <algorithm>

using namespace std;

/*=================================
=            CONSTANTS            =
=================================*/

#define HZ 100
#define RETURN '\n'
#define EOS '\0'
#define COMMA ','
#define SPACE ' '
#define TAB '\t'
#define COLON ':'
#define SEMICOLON ';'

#define SPLITMERGE 'M'

#define T_INPUT 1
#define T_OUTPUT 2
#define T_SIGNAL 3
#define T_MODULE 4
#define T_COMPONENT 5
#define T_EXIST 9
#define T_COMMENT 10
#define T_END 11

#define TABLESIZE 5000
#define MAXIO 5000
#define MAXMODULES 5000
#define MAXDFF 10560

#define GOOD 1
#define FAULTY 2
#define DONTCARE -1
#define ALLONES 0xffffffff

#define MAXlevels 10000
#define MAXIOS 5120
#define MAXFanout 10192
#define MAXFFS 40048
#define MAXGATES 100000
#define MAXevents 100000

#define TRUE 1
#define FALSE 0

#define EXCITED_1_LEVEL 1
#define POTENTIAL 2
#define LOW_DETECT 3
#define HIGH_DETECT 4
#define REDUNDANT 5


// Possible gate types indices and their string names
enum {
   JUNK,           /* 0 */
   T_input,        /* 1 */
   T_output,       /* 2 */
   T_xor,          /* 3 */
   T_xnor,         /* 4 */
   T_dff,          /* 5 */
   T_and,          /* 6 */
   T_nand,         /* 7 */
   T_or,           /* 8 */
   T_nor,          /* 9 */
   T_not,          /* 10 */
   T_buf,          /* 11 */
   T_tie1,         /* 12 */
   T_tie0,         /* 13 */
   T_tieX,         /* 14 */
   T_tieZ,         /* 15 */
   T_mux_2,        /* 16 */
   T_bus,          /* 17 */
   T_bus_gohigh,   /* 18 */
   T_bus_golow,    /* 19 */
   T_mystery,      /* 20 */  // originally T_tristate
   T_tristateinv,  /* 21 */
   T_tristate1     /* 22 */
};

const char gate_names[][32] = {
   "JUNK",         /* 0 */
   "input",        /* 1 */
   "output",       /* 2 */
   "xor",          /* 3 */
   "xnor",         /* 4 */
   "dff",          /* 5 */
   "and",          /* 6 */
   "nand",         /* 7 */
   "or",           /* 8 */
   "nor",          /* 9 */
   "not",          /* 10 */
   "buf",          /* 11 */
   "tie1",         /* 12 */
   "tie0",         /* 13 */
   "tieX",         /* 14 */
   "tieZ",         /* 15 */
   "mux_2",        /* 16 */
   "bus",          /* 17 */
   "bus_gohigh",   /* 18 */
   "bus_golow",    /* 19 */
   "mystery",      /* 20 */
   "tristateinv",  /* 21 */
   "tristate1"     /* 22 */
};



/*===========================================
=            Class: gateLevelCkt            =
===========================================*/

// forward declare eventSimulator class to break circular dependancy while compiling
class eventSimulator;


class gateLevelCkt {
private:
    /*----------  Basic Circuit Information  ----------*/

    // total number of gates (including faulty ones)
    int numgates;
    // number of Fault-Free gates
    int numFaultFreeGates;
    // number of levels in gate level ckt
    int maxlevels;
    // maximum number of gates in one given level
    int maxLevelSize;	
    // levelSize for each level
    vector <int> levelSize;
    // input list
    vector <int> inputs;
    // output list
    vector <int> outputs;
    // flop list
    vector <int> ff_list;
    // flop ID map
    vector <int> ffMap;

    /*----------  Gate Attributes  ----------*/

    // gate type
    vector <int> gtype;
    // fanin count
    vector <int> fanin;
    // fanout count
    vector <int> fanout;
    // level
    vector <int> levelNum;
    // PO
    vector <int> po;
    // fanin list
    vector < vector<int> > inlist;
    // fanout list
    vector < vector<int> > fnlist;
    // predecessor of successor input-pin list
    vector < vector<int> > predOfSuccInput;
    // successor of predecessor output-pin list
    vector < vector<int> > succOfPredOutput;  


public:
    // number of Primary Inputs (PI)
    int numpri;
    // number of Primary Outputs (PO)
    int numout;
    // number of FF's
    int numff;
    // number of mystery gates
    int nummg;
    // list of mystery gates
    vector<int> mystery_gate;
    // mystery gate map
    vector<bool> is_mgate;

    // Reset states for the FFs
    vector <int> RESET_FF1;
    vector <int> RESET_FF2;


    gateLevelCkt();
    gateLevelCkt(string, int initFromFile=0);
    void readNetlist(string, int initFromFile=0);
    void setFaninoutMatrix();

    // A friend indeed...
    // Abstracts all simulator data (variable),
    // while the gateLevelCkt holds all circuit data (i.e. constant/global)
    friend class eventSimulator;

    // mystery gate solver helper functions
    void existsMysteryGate();
    void buildPossibilitiesList(vector< vector<int> > &);
    void insertRandomMystery();

};


#endif