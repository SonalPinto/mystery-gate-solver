/**
 *
 * class: eventSimulator
 * Original Source: lsim_3val.cpp
 * Original Author: Dr. Michael S. Hsiao
 *
 *
 *
 * Modified by: Sonal Pinto
 * 
 * Describes the simulator object that simualtes a gateLevelCkt object
 * 
 */


#ifndef EVENTSIMULATOR_H
#define EVENTSIMULATOR_H

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include "gateLevelCkt.h"


class eventSimulator {
private:
    // tied nodes
    int numTieNodes;
	vector <int> TIES;
    
    // schedule marker
    vector <bool> sched;
    // gate signal value
    vector <int> value1;
    vector <int> value2;

    // gate overrides
    vector <int> gate_override;
    // event list for each level in the circuit
    vector < vector<int> > levelEvents;	
    // evenlist length
    vector<int> levelLen;	
    // total number of levels in wheel
    int numlevels;
    // current level
    int currLevel;
    // activation list for the current level in circuit
    vector<int> activation;
    // length of the activation list
    int actLen;
    // activation list for the FF's	
    vector<int> actFFList;	
    // length of the actFFList
    int actFFLen;	

public:
	// good state (without scan)
    vector <int> goodState;
    // suspect_list per gate
    vector < vector<int> > suspect_list;
    // convergence flag - all gates have one suspect - all clauses are unit clauses
    bool isConverged;
    // When a particular gate has only one suspect - it can either be that, or nothing at all!
    // Same as Unit Clauses in Boolean SAT!
    vector<bool> isUnitClause;
    int non_UnitClauses;

    eventSimulator(const gateLevelCkt&);
    eventSimulator(const gateLevelCkt&, const eventSimulator&);

    // init sim vars
    void setupWheel(const gateLevelCkt&);
    // insert events into the wheel
    void insertEvent(int, int);
    // inject events from tied nodes
    void setTieEvents(const gateLevelCkt&, int initFromFile=0); 
    // Print and validate outputs
    int observeOutputs(const gateLevelCkt&, string, string, int, int);
    void printOutputs(const gateLevelCkt&, string&, string&);
    // Apply input vector at the PI
    void applyVector(const gateLevelCkt&, string);
    // pop the next event to process
    int retrieveEvent(const gateLevelCkt&);
    // Simulate till convergence!
    void goodsim(const gateLevelCkt&);

 	// Mystery Gate Solver functions
 	void setGateOverride(int, int);
 	void reset(const gateLevelCkt&);
 	void clone(const gateLevelCkt&, const eventSimulator&);
 	void setSuspectList(const vector< vector<int> > &);
 	void tieValue(int, int);
 	void tieValue_MG(const gateLevelCkt&, vector<int>);
 	void propagateMysteryEvents(const gateLevelCkt&);
 	bool iterrogate_suspects(const gateLevelCkt&, const vector<int>&, int);
};

#endif