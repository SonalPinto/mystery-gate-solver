#include "eventSimulator.h"


eventSimulator::eventSimulator(const gateLevelCkt& ckt){
	numTieNodes = 0;
	isConverged = false;

    sched.resize(ckt.numgates+64, 0);
    value1.resize(ckt.numgates+64);
    value2.resize(ckt.numgates+64);
    gate_override.resize(ckt.numgates+64, T_mystery);
    goodState.resize(ckt.numff, 'X');
    isUnitClause.resize(ckt.nummg, false);
    non_UnitClauses = ckt.nummg;

	// Handle ties
	for(int i=1; i<ckt.numgates; ++i){
		if (ckt.gtype[i] == T_tie1 || ckt.gtype[i] == T_tie0) {

			TIES.push_back(i);
		    numTieNodes++;

		    if (numTieNodes > 511) {
				cerr, "Can't handle more than 512 tied nodes\n";
				exit(-1);
		    }

		}
	}

	// Set initial signal values for the nodes
	for(int i=1; i<ckt.numgates; ++i){
		switch(ckt.gtype[i]){
			case T_tie1:
				value1[i] = ALLONES;
            	value2[i] = ALLONES;
				break;

			case T_tie0:
				value1[i] = 0;
           		value2[i] = 0;
				break;

			default:
				value1[i] = 0;
	    		value2[i] = ALLONES;
		}
	}

    setupWheel(ckt);
}



/**
 *
 * Clone eventSimulator object
 *
 */
void eventSimulator::clone(const gateLevelCkt& ckt, const eventSimulator& sim){
    isConverged = sim.isConverged;
    isUnitClause = sim.isUnitClause;
    sched = sim.sched;
    value1 = sim.value1;
    value2 = sim.value2;
    gate_override = sim.gate_override;
   	goodState = sim.goodState;
   	non_UnitClauses = sim.non_UnitClauses;

   	numTieNodes = sim.numTieNodes;
   	TIES = sim.TIES;
    
    numlevels = sim.numlevels;
    currLevel = sim.currLevel;

    activation = sim.activation;
    actLen = sim.actLen;
    actFFList = sim.actFFList;
    actFFLen = sim.actFFLen;
    
    levelLen = sim.levelLen;
    
    for(int i=0; i<sim.levelEvents.size(); ++i){
		levelEvents.push_back(sim.levelEvents[i]);
	}

	for(int i=0; i<sim.suspect_list.size(); ++i){
		suspect_list.push_back(sim.suspect_list[i]);
	}


	// cout<<"\n\n\tCLONE: ";
 //    for (int i=0; i<ckt.numout; i++) {
 //    	if (value1[ckt.outputs[i]] == ALLONES && value2[ckt.outputs[i]] == ALLONES){
 //    	    cout << "1";
 //        }
 //    	else if ((value1[ckt.outputs[i]] == 0) && (value2[ckt.outputs[i]] == 0)){
 //    	    cout << "0";
 //        }
 //    	else {
 //    	    cout << "X";
 //        }
 //    }

 //    if(ckt.numff){
 //        cout << "  \tFF: ";
 //        for (int i=0; i<ckt.numff; i++) {
 //        	if (value1[ckt.ff_list[i]] == ALLONES && value2[ckt.ff_list[i]] == ALLONES){
 //                cout << "1";
 //            }
 //        	else if ((value1[ckt.ff_list[i]] == 0) && (value2[ckt.ff_list[i]] == 0)){
 //        	    cout << "0";
 //            }
 //        	else{
 //        	    cout << "X";
 //            }
 //        }
 //    }
}


eventSimulator::eventSimulator(const gateLevelCkt& ckt, const eventSimulator& sim){
	clone(ckt, sim);
}

/**
 *
 * Set suspect list
 *
 */
void eventSimulator::setSuspectList(const vector< vector<int> > & gate_psblty){
	for(int i=0; i<gate_psblty.size(); ++i){
		suspect_list.push_back(gate_psblty[i]);
	}
}




/**
 *
 * Low-Wheel Class
 *
 */

void eventSimulator::setupWheel(const gateLevelCkt& ckt) {
	numlevels = ckt.maxlevels;

    levelLen.resize(ckt.maxlevels);
    levelEvents.resize(ckt.maxlevels);

    for (int i=0; i < ckt.maxlevels; ++i) {
		levelEvents[i].resize(ckt.maxLevelSize * 2);
		levelLen[i] = 0;
    }

    activation.resize(ckt.maxLevelSize);
    
    actFFList.resize(ckt.numff + 64);
}


/**
 *
 * Inserts even in the wheel
 *
 */
inline void eventSimulator::insertEvent(int levelN, int gateN) {
    levelEvents[levelN][levelLen[levelN]] = gateN;
    levelLen[levelN]++;
}


/**
 *
 * This function set up the events for tied nodes
 *
 */
void eventSimulator::setTieEvents(const gateLevelCkt& ckt, int initFromFile) {
    int predecessor, successor;
    int i, j;

    for (i=0; i < numTieNodes; i++) {
	  // different from previous time frame, place in wheel
	  for (j=0; j<ckt.fanout[TIES[i]]; j++) {

	    successor = ckt.fnlist[TIES[i]][j];
	    if (sched[successor] == 0){
	    	insertEvent(ckt.levelNum[successor], successor);
			sched[successor] = 1;
	    }

	  }
    }

   // initialize state if necessary
    if (initFromFile) {
		cout << "Initialize circuit to values in *.initState!\n";
		for (i=0; i<ckt.numff; i++) {
		    value1[ckt.ff_list[i]] = value2[ckt.ff_list[i]] = ckt.RESET_FF1[i];

		  	for (j=0; j<ckt.fanout[ckt.ff_list[i]]; j++) {
			    successor = ckt.fnlist[ckt.ff_list[i]][j];
			    if (sched[successor] == 0) {
			    	insertEvent(ckt.levelNum[successor], successor);
					sched[successor] = 1;
			    }
			}	// for j

		    predecessor = ckt.inlist[ckt.ff_list[i]][0];
		    value1[predecessor] = value2[predecessor] = ckt.RESET_FF1[i];

		  	for (j=0; j<ckt.fanout[predecessor]; j++) {
			    successor = ckt.fnlist[predecessor][j];
			    if (sched[successor] == 0) {
			    	insertEvent(ckt.levelNum[successor], successor);
					sched[successor] = 1;
			    }
		  	}	// for j

		}	// for i
    }
}



/**
 *
 * This function prints the outputs of the fault-free circuit.
 *
 */
int eventSimulator::observeOutputs(const gateLevelCkt& ckt, string outputVector, string outputFF, int DEBUG, int CHECK_FF) {
    int i;
    int flag=0, flag2=0;

    if(DEBUG)cout<<"\t";
    for (i=0; i<ckt.numout; i++) {
    	if (value1[ckt.outputs[i]] == ALLONES && value2[ckt.outputs[i]] == ALLONES){
    	    if(DEBUG) cout << "1";
            if(outputVector[i] == '1') flag++;
        }
    	else if ((value1[ckt.outputs[i]] == 0) && (value2[ckt.outputs[i]] == 0)){
    	    if(DEBUG) cout << "0";
            if(outputVector[i] == '0') flag++;
        }
    	else {
    	    if(DEBUG) cout << "X";
            if(outputVector[i] == 'X') flag++;
        }
    }

    if(ckt.numff){
        // cout << "  \tFF: ";
        for (i=0; i<ckt.numff; i++) {
        	if (value1[ckt.ff_list[i]] == ALLONES && value2[ckt.ff_list[i]] == ALLONES){
                // cout << "1";
                if(outputFF[i] == '1') flag2++;
            }
        	else if ((value1[ckt.ff_list[i]] == 0) && (value2[ckt.ff_list[i]] == 0)){
        	    // cout << "0";
                if(outputFF[i] == '0') flag2++;
            }
        	else{
        	    // cout << "X";
                 if(outputFF[i] == 'X') flag2++;
            }
        }
    }

    if(!CHECK_FF && ckt.numout == flag) return 1;
    else if (CHECK_FF && ckt.numout == flag && ckt.numff == flag2) return 1;
    else return 0;
}


/**
 *
 * This function prints the outputs of the fault-free circuit.
 *
 */
void eventSimulator::printOutputs(const gateLevelCkt& ckt, string &outPO, string &outFF) {
    int i;

    cout<<"\t";
    outPO = "";
    outFF = "";
    for (i=0; i<ckt.numout; i++) {
    	if (value1[ckt.outputs[i]] == ALLONES && value2[ckt.outputs[i]] == ALLONES){
    	    cout << "1";
    	    outPO += "1";
    	}
    	else if ((value1[ckt.outputs[i]] == 0) && (value2[ckt.outputs[i]] == 0)){
    	    cout << "0";
    	    outPO += "0";
    	}
    	else {
    	    cout << "X";
    	    outPO += "X";
    	}
    }

    if(ckt.numff){
    	cout<<endl;
        for (i=0; i<ckt.numff; i++) {
        	if (value1[ckt.ff_list[i]] == ALLONES && value2[ckt.ff_list[i]] == ALLONES){
                cout << "1";
                outFF += "1";
        	}
        	else if ((value1[ckt.ff_list[i]] == 0) && (value2[ckt.ff_list[i]] == 0)){
        	    cout << "0";
        	    outFF += "0";
        	}
        	else{
        	    cout << "X";
        	    outFF += "X";
        	}
        }
    }
}



/**
 *
 * This function applies the vector to the inputs of the ckt.
 *
 */

void eventSimulator::applyVector(const gateLevelCkt& ckt, string vec) {
    int origVal1, origVal2;
    char origBit;
    int successor;
    int net;
    int i, j;

    for (i = 0; i < ckt.numpri; i++) {

    	net = ckt.inputs[i];

		origVal1 = value1[net] & 1;
		origVal2 = value2[net] & 1;

		if ((origVal1 == 1) && (origVal2 == 1))
		    origBit = '1';
		else if ((origVal1 == 0) && (origVal2 == 0))
		    origBit = '0';
		else
		    origBit = 'x';


		if ((origBit != vec[i]) && ((origBit != 'x') || (vec[i] != 'X'))) {
			switch (vec[i]) {
				case '0':
					value1[net] = 0;
					value2[net] = 0;
					break;

				case '1':
					value1[net] = ALLONES;
					value2[net] = ALLONES;
					break;

				case 'x':
				case 'X':
					value1[net] = 0;
					value2[net] = ALLONES;
					break;

				default:
					printf("ERROR: Unkown [%c] in input vector.\n", vec[i]);
					exit(-1);
			}

			// different from previous time frame, place in wheel
			for (j=0; j<ckt.fanout[net]; j++) {
				successor = ckt.fnlist[net][j];
				if (sched[successor] == 0){
					insertEvent(ckt.levelNum[successor], successor);
				    sched[successor] = 1;
				}
			}
		}
    }
}



/**
 *
 * Retrieves the latest event from the wheel
 *
 */

int eventSimulator::retrieveEvent(const gateLevelCkt& ckt) {
	while (currLevel<ckt.maxlevels && levelLen[currLevel]==0)
		currLevel++;

	if (currLevel < ckt.maxlevels) {
		levelLen[currLevel]--;
    	return(levelEvents[currLevel][levelLen[currLevel]]);
    } else
		return(-1);
}



/**
 *
 * SIMULATE (no faults inserted)
 *
 */
void eventSimulator::goodsim(const gateLevelCkt& ckt) {
	int sucLevel;
    int net, predecessor, successor;
    vector<int> predList;
    int i;
    int val1, val2, tmpVal;
    int gtype;
    bool flag;

    currLevel = 0;
    actLen = actFFLen = 0;

    while (currLevel < ckt.maxlevels) {
    	// fetch next event off the wheel
    	net = retrieveEvent(ckt);

    	// process valid event
    	if (net != -1) {	

    		// mark its schedule as done
    		sched[net]= 0;

    		// get gate type
    		gtype = ckt.gtype[net];

    		// override mystery gate type
    		if(ckt.is_mgate[net]){
    			gtype = gate_override[net];
    		}

    		// Decide gate signal based on gate type and inputs
    		switch(gtype){
    			case T_and:
	    	    	val1 = val2 = ALLONES;
					predList = ckt.inlist[net];
	    	    	for (i=0; i<ckt.fanin[net]; i++) {
					    predecessor = predList[i];
					    val1 &= value1[predecessor];
					    val2 &= value2[predecessor];
	    	    	}
		    		break;

		    	case T_nand:
		    	    val1 = val2 = ALLONES;
					predList = ckt.inlist[net];
	    	    	for (i=0; i<ckt.fanin[net]; i++) {
					    predecessor = predList[i];
					    val1 &= value1[predecessor];
					    val2 &= value2[predecessor];
	    	    	}
			    	tmpVal = val1;
			    	val1 = ALLONES ^ val2;
			    	val2 = ALLONES ^ tmpVal;
			    	break;

			    case T_or:
		    	    val1 = val2 = 0;
					predList = ckt.inlist[net];
		    	    for (i=0; i<ckt.fanin[net]; i++) {
					    predecessor = predList[i];
					    val1 |= value1[predecessor];
					    val2 |= value2[predecessor];
		    	    }
			    	break;

			    case T_nor:
		    	    val1 = val2 = 0;
					predList = ckt.inlist[net];
					for (i=0; i<ckt.fanin[net]; i++) {
					    predecessor = predList[i];
					    val1 |= value1[predecessor];
					    val2 |= value2[predecessor];
		    	    }
			    	tmpVal = val1;
			    	val1 = ALLONES ^ val2;
			    	val2 = ALLONES ^ tmpVal;
			    	break;

			    case T_not:
			    	predecessor = ckt.inlist[net][0];
			    	val1 = ALLONES ^ value2[predecessor];
			    	val2 = ALLONES ^ value1[predecessor];
			    	break;

		      	case T_buf:
			    	predecessor = ckt.inlist[net][0];
			    	val1 = value1[predecessor];
			    	val2 = value2[predecessor];
					break;

		      	case T_dff:
			    	predecessor = ckt.inlist[net][0];
			    	val1 = value1[predecessor];
			    	val2 = value2[predecessor];
					actFFList[actFFLen] = net;
					actFFLen++;
			    	break;

			    case T_xor:
					predList = ckt.inlist[net];
			    	val1 = value1[predList[0]];
			    	val2 = value2[predList[0]];

	            	for(i=1;i<ckt.fanin[net];i++){
		    	    	predecessor = predList[i];
	                    tmpVal = ALLONES^(((ALLONES^value1[predecessor]) &
	              			(ALLONES^val1)) | (value2[predecessor]&val2));
	                    val2 = ((ALLONES^value1[predecessor]) & val2) |
	                  		(value2[predecessor] & (ALLONES^val1));
	                    val1 = tmpVal;
	            	}
			    	break;

			    case T_xnor:
					predList = ckt.inlist[net];
					val1 = value1[predList[0]];
			    	val2 = value2[predList[0]];

	            	for(i=1;i<ckt.fanin[net];i++) {
		    	    	predecessor = predList[i];
	                    tmpVal = ALLONES^(((ALLONES^value1[predecessor]) &
	                   		(ALLONES^val1)) | (value2[predecessor]&val2));
	                    val2 = ((ALLONES^value1[predecessor]) & val2) |
	                  		(value2[predecessor]& (ALLONES^val1));
	                    val1 = tmpVal;
	            	}
			    	tmpVal = val1;
					val1 = ALLONES ^ val2;
			    	val2 = ALLONES ^ tmpVal;
			    	break;

			    case T_output:
				    predecessor = ckt.inlist[net][0];
			    	val1 = value1[predecessor];
			    	val2 = value2[predecessor];
	        		break;

				case T_input:
				case T_tie0:
				case T_tie1:
				case T_tieX:
				case T_tieZ:
					val1 = value1[net];
					val2 = value2[net];
					break;

				case T_mystery:
					// Mystery gate Stuck-at propagation
					flag=true;
					predList = ckt.inlist[net];
					for (i=0; i<ckt.fanin[net]; i++) {
					    predecessor = predList[i];
					    if(value1[predecessor] != value2[predecessor]){
					    	flag=false;
					    	break;
					    }
		    	    }

		    	    if(flag){
		    	    	// real inputs => real outputs
	            		val1 = value1[net];
	                    val2 = value2[net];
	                } else{
	                	// X-input => X-output
	                	val1 = 0;
	                    val2 = ALLONES;
	                }
                    break;

	      		default: 
	      			printf("[ERROR] Illegal gate type: [net %d] %d\n", net, gtype);
					exit(-1);
    		}


    		// if gate value changed
    		if ((val1 != value1[net]) || (val2 != value2[net])) {   

	    		value1[net] = val1;
	    		value2[net] = val2;

	    		for (i=0; i<ckt.fanout[net]; i++) {
	    		    successor = ckt.fnlist[net][i];
	    		    sucLevel = ckt.levelNum[successor];
	    		    if (sched[successor] == 0) {
						if (sucLevel != 0)
							insertEvent(sucLevel, successor);
						// same level, wrap around for next time
						else {	
							activation[actLen] = successor;
							actLen++;
						}

						sched[successor] = 1;
	    		    }
	    		}
		    }
    	}
    }


    // now re-insert the activation list for the FF's
    for (i=0; i < actLen; i++) {
    	insertEvent(0, activation[i]);
    	sched[activation[i]] = 0;

        predecessor = ckt.inlist[activation[i]][0];
        net = ckt.ffMap[activation[i]];
        if (value1[predecessor])
            goodState[net] = '1';
        else if (value2[predecessor] == 0)
            goodState[net] = '0';
        else
            goodState[net] = 'X';
    }
}


/**
 *
 * Set gate ovverides for simulation. Froces gate behavious
 *
 */
void eventSimulator::setGateOverride(int gate, int new_gtype){
	gate_override[gate] = new_gtype;
}


/**
 *
 * Reset simulator
 *
 */
void eventSimulator::reset(const gateLevelCkt& ckt){
    int i,j;
    int predecessor, successor;

    // gate_override.assign(gate_override.size()-1, 99);

    for(i=0; i<ckt.numgates; i++){
        value1[i] = 0;
        value2[i] = ALLONES;
    }

    for (i=0; i<ckt.numff; i++) {
        value1[ckt.ff_list[i]] = 0;
        value2[ckt.ff_list[i]] = ALLONES;

        for (j=0; j<ckt.fanout[ckt.ff_list[i]]; j++) {
            successor = ckt.fnlist[ckt.ff_list[i]][j];
            if (sched[successor] == 0) {
                insertEvent(ckt.levelNum[successor], successor);
                sched[successor] = 1;
            }
        }

        predecessor = ckt.inlist[ckt.ff_list[i]][0];
        value1[predecessor] = 0;
        value2[predecessor] = ALLONES;

        for (j=0; j<ckt.fanout[predecessor]; j++) {
            successor = ckt.fnlist[predecessor][j];
            if (sched[successor] == 0) {
                insertEvent(ckt.levelNum[successor], successor);
                sched[successor] = 1;
            }
        }

    }
}



/**
 *
 * Tie the value of a gate
 *
 */
void eventSimulator::tieValue(int net, int val) {
    value1[net] = val;
    value2[net] = val;
}


void eventSimulator::tieValue_MG(const gateLevelCkt& ckt, vector<int> val){
	int i,j;
	i=j=0;

	for(;i<ckt.nummg; ++i){
		if(!isUnitClause[i]){
			tieValue(ckt.mystery_gate[i], val[j]);
			j++;
		}
	}
}


/**
 *
 * Propagate Mystery Gate events
 *
 */
void eventSimulator::propagateMysteryEvents(const gateLevelCkt& ckt) {
    int predecessor, successor;
    int i, j;

    for (i=0; i<ckt.nummg; i++) {
    	insertEvent(ckt.levelNum[ckt.mystery_gate[i]], ckt.mystery_gate[i]);

	  	for (j=0; j<ckt.fanout[ckt.mystery_gate[i]]; j++) {

		    successor = ckt.fnlist[ckt.mystery_gate[i]][j];
		    if (sched[successor] == 0){
		    	insertEvent(ckt.levelNum[successor], successor);
				sched[successor] = 1;
		    }

		}
    }

    for (i=0; i<ckt.numff; i++) {
        // value1[ckt.ff_list[i]] = 0;
        // value2[ckt.ff_list[i]] = ALLONES;

        for (j=0; j<ckt.fanout[ckt.ff_list[i]]; j++) {
            successor = ckt.fnlist[ckt.ff_list[i]][j];
            if (sched[successor] == 0) {
                insertEvent(ckt.levelNum[successor], successor);
                sched[successor] = 1;
            }
        }

        predecessor = ckt.inlist[ckt.ff_list[i]][0];
        // value1[predecessor] = 0;
        // value2[predecessor] = ALLONES;

        for (j=0; j<ckt.fanout[predecessor]; j++) {
            successor = ckt.fnlist[predecessor][j];
            if (sched[successor] == 0) {
                insertEvent(ckt.levelNum[successor], successor);
                sched[successor] = 1;
            }
        }
    }
}


/* 
 * interrogate_suspects
 * At the end of every test, based on whether we have any new leads
 * we need to question each of our suspects
 * 
 * Initially, all gates are suspects.
 * But, slowly the Truth Table begins to grow, shedding light on our Mystery Gates 
 */
bool eventSimulator::iterrogate_suspects(const gateLevelCkt& ckt, const vector<int>& testValue, int DEBUG){
	unsigned int j, value, gateType;
    int predecessor, gateChecked, mysteryGate_fanin, mgate_ID, i;
    vector<int> predList;
    bool clues=true; // assume there are clues
    int tval=0;

    // iterate through all mystery gates
    for(int mgate=0; mgate<ckt.nummg; ++mgate){
    	// no need to eval unit clauses
    	if(!isUnitClause[mgate]) {
	        // Keep some values local
	        mgate_ID = ckt.mystery_gate[mgate];
	        predList = ckt.inlist[mgate_ID];
	        mysteryGate_fanin = ckt.fanin[mgate_ID];

	        // Now, based on the inputs to the mystery gate, and our accepted Stuck-At value,
	        // we shall trim the suspect list

	        if(DEBUG) printf("Suspect: [GATE# %d]\n", mgate_ID);

	        for(j=0; j<suspect_list[mgate].size(); j++) {
	            gateType = suspect_list[mgate][j];
	            gateChecked = 0;

	            // Check whether the suspect will talk (see if the gate verifies the truth table)
	            switch(gateType) {
	                case T_and:
	                    value = ALLONES;
	                    for (i=0; i<mysteryGate_fanin; i++) {
	                        predecessor = predList[i];
	                        value &= value1[predecessor];
	                    }
	                    gateChecked++;
	                    break;

	                case T_or:
	                    value = 0;
	                    for (i=0; i<mysteryGate_fanin; i++) {
	                        predecessor = predList[i];
	                        value |= value1[predecessor];
	                    }
	                    gateChecked++;
	                    break; 

	                case T_nand:
	                    value = ALLONES;
	                    for (i=0; i<mysteryGate_fanin; i++) {
	                        predecessor = predList[i];
	                        value &= value1[predecessor];
	                    }
	                    value = ALLONES ^ value;
	                    gateChecked++;
	                    break;

	                case T_nor:
	                    value = 0;
	                    for (i=0; i<mysteryGate_fanin; i++) {
	                        predecessor = predList[i];
	                        value |= value1[predecessor];
	                    }
	                    value = ALLONES ^ value;
	                    gateChecked++;
	                    break;


	                case T_not:
	                    predecessor = predList[0];
	                    value = ALLONES ^ value1[predecessor];
	                    gateChecked++;
	                    break;

	                case T_buf:
	                    predecessor = predList[0];
	                    value = value1[predecessor];
	                    gateChecked++;
	                    break;

	                case T_xor:
	                    value = value1[predList[0]];

	                    for(i=1;i<mysteryGate_fanin;i++) {
	                        predecessor = predList[i];
	                        value = ALLONES^(((ALLONES^value1[predecessor]) 
	                            & (ALLONES^value)) | (value1[predecessor]&value));
	                    }
	                    gateChecked++;
	                    break;

	                case T_xnor:
	                    value = value1[predList[0]];

	                    for(i=1;i<mysteryGate_fanin;i++) {
	                        predecessor = predList[i];
	                        value = ALLONES^(((ALLONES^value1[predecessor]) 
	                            & (ALLONES^value)) | (value1[predecessor]&value));
	                    }
	                    value = ALLONES ^ value;
	                    gateChecked++;
	                    break;
	            }

	            if(gateChecked == 0){
	                printf("[ERROR] Woah! This gatetype (%s) for Gate#%d couldn't be checked. Call the coder asap.\n", gate_names[gateType], mgate_ID);
	                exit(1);
	            }

	            if(DEBUG){
	                printf("Checking [%s] :\t", gate_names[gateType]);

	                printf("input: ");
	                for (i=0; i<mysteryGate_fanin; i++) {
	                    predecessor = predList[i];
	                    cout<<(value1[predecessor] & 1);
	                }
	                printf("\t, ouput: %u <-?-> %u,\t", (value & 1), (testValue[tval] & 1));
	            }

	            // This suspect is clear, i.e. the output for the given inputs doesn't match the right output
	            if(value != testValue[tval]){
	                if(DEBUG) printf("Not a suspect...\n");
	                suspect_list[mgate].erase(suspect_list[mgate].begin() + j);
	                j--;
	            } else {
	                if(DEBUG) printf("Suspicious!\n");
	            }
	        }


	        // Print the current Suspect List!
	        if(DEBUG){
		        printf("\t[GATE# %d] Suspect List: { ",mgate_ID);
		        for(j=0; j<suspect_list[mgate].size(); j++){
		            cout<<gate_names[suspect_list[mgate][j]];
		            if(j!=suspect_list[mgate].size()-1) cout<<", ";
		        }
		        cout<<" }\n";
	    	}

	    	tval++;

	   		if(suspect_list[mgate].size()==0){
	   			clues=false;
	   			break;
	   		} else if(suspect_list[mgate].size()==1){
	   			// We found a Unit Clause!
	   			non_UnitClauses--;
	   			isUnitClause[mgate]=true;
	   			gate_override[mgate_ID] = suspect_list[mgate][0];
	   		}
	   	}
    }

    // Check for convergence, i.e each gate has only one suspect, i.e there are only unit clauses
    isConverged == (non_UnitClauses==0) ? true : false;

    return clues;
}
