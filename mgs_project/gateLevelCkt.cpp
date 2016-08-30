#include "gateLevelCkt.h"


/**
 *
 * Constructor default
 *
 */
gateLevelCkt::gateLevelCkt(){
}



/**
 *
 * Constructor: Reads a Levelized netlist and translates it into the gateLevelCkt object
 *
 */

gateLevelCkt::gateLevelCkt(string cktName, int initFromFile){
	readNetlist(cktName, initFromFile);
}


/**
 *
 * This function builds the matrix of succOfPredOutput and predOfSuccInput.
 *
 */

void gateLevelCkt::setFaninoutMatrix(){
	int i, j, k;
    int predecessor, successor;
    int checked[MAXFanout];
    int checkID;	// needed for gates with fanouts to SAME gate
    int prevSucc, found;

    predOfSuccInput.resize(numgates+64);
    succOfPredOutput.resize(numgates+64);
    for (i=0; i<MAXFanout; i++)
		checked[i] = 0;

	checkID = 1;
	prevSucc = -1;


	for (i=1; i<numgates; i++) {
		predOfSuccInput[i].resize(fanout[i]);
		succOfPredOutput[i].resize(fanin[i]);

		for (j=0; j<fanout[i]; j++) {
		    if (prevSucc != fnlist[i][j])
				checkID++;

		    prevSucc = fnlist[i][j];
		    successor = fnlist[i][j];
		    k=found=0;

		    while ((k<fanin[successor]) && (!found)) {
				if ((inlist[successor][k] == i) && (checked[k] != checkID)) {
				    predOfSuccInput[i][j] = k;
				    checked[k] = checkID;
				    found = 1;
				}
				k++;
		    }
		}

		for (j=0; j<fanin[i]; j++) {
		    if (prevSucc != inlist[i][j])
				checkID++;

		    prevSucc = inlist[i][j];
		    predecessor = inlist[i][j];
		    k=found=0;

		    while ((k<fanout[predecessor]) && (!found)) {
				if ((fnlist[predecessor][k] == i) && (checked[k] != checkID)) {
				    succOfPredOutput[i][j] = k;
				    checked[k] = checkID;
				    found=1;
				}
				k++;
		    }
		}
    }

    for (i=numgates; i<numgates+64; i+=2) {
		predOfSuccInput[i].resize(MAXFanout);
		succOfPredOutput[i].resize(MAXFanout);
    }
}


/**
 *
 * Reads a Levelized netlist and translates it into the gateLevelCkt object
 *
 */

void gateLevelCkt::readNetlist(string cktName, int initFromFile){
	ifstream yyin;
    string fName;
    char c;
    int netnum, junk;
    int f1, f2, f3;

    // start reading the lev file
    fName = cktName + ".lev";
    yyin.open(fName.c_str(), ios::in);
    if (!yyin) {
		cerr << "Can't open your .lev file\n";
		exit(-1);
    }

    // init
    numpri = numgates = numout = maxlevels = numff = 0;
    maxLevelSize = 32;
    levelSize.resize(MAXlevels);
    fill(levelSize.begin(), levelSize.end(), 0);

    // read number of gates
    yyin >> numgates;
    yyin >> junk;

    // Reserve space for attributes
    gtype.resize(numgates+64);
    fanin.resize(numgates+64);
    fanout.resize(numgates+64);
    levelNum.resize(numgates+64);
    po.resize(numgates+64);
    inlist.resize(numgates+64);
    fnlist.resize(numgates+64);
    ffMap.resize(numgates+64);

    // now read in the circuit
    for(int i=1; i<numgates; ++i){
    	yyin >> netnum;
		yyin >> f1;
		yyin >> f2;
		yyin >> f3;

		gtype[netnum] = f1;
		levelNum[netnum] = f2;
		levelSize[f2]++;

		if (f2 >= maxlevels) maxlevels = f2 + 5;
		if (maxlevels > MAXlevels) {
		    cerr << "MAXIMUM level (" << maxlevels << ") exceeded.\n";
		    exit(-1);
		}

		// fanin count
		fanin[netnum] = f3;

		if (f3 > MAXFanout)
		    cerr << "Fanin count (" << f3 << " exceeded\n";

		// handle PI type
		if (gtype[netnum] == T_input){
		    inputs.push_back(netnum);
		    numpri++;
		}

		// handle PO type
		if (gtype[netnum] == T_output) {
		    po[netnum] = 1;
		    outputs.push_back(netnum);
		    numout++;
		} else
		    po[netnum] = 0;

		// handle DFF type
		if (gtype[netnum] == T_dff) {
		    if (numff >= (MAXFFS-1)) {
				cerr << "The circuit has more than " << MAXFFS -1 << " FFs\n";
				exit(-1);
		    }
		    ff_list.push_back(netnum);
		    numff++;
		}

		// now read in the faninlist
		inlist[netnum].resize(fanin[netnum]);
		for (int j=0; j<fanin[netnum]; ++j) {
		    yyin >> f1;
		    inlist[netnum][j] = f1;
		}

		// The same info as before is repeated... not sure why. But, it's junk
		for (int j=0; j<fanin[netnum]; ++j) yyin >> junk;

		// read in the fanout list
		yyin >> f1;
		fanout[netnum] = f1;

		if (fanout[netnum] > MAXFanout){
	    	cerr << "Fanout count (" << fanout[netnum] << ") exceeded\n";
	    	exit(-1);
		}

		fnlist[netnum].resize(fanout[netnum]);
		for (int j=0; j<fanout[netnum]; ++j) {
		    yyin >> f1;
		    fnlist[netnum][j] = f1;
		}

		// head to next line
		yyin.get(c);
		while(c!=RETURN){yyin.get(c);}
    }

    // close file
    yyin.close();

    // number of faultFree gates
    numFaultFreeGates = numgates;


    // now compute the maximum width across all levels
    for (int i=0; i<maxlevels; ++i) {
		if (levelSize[i] > maxLevelSize)
	    	maxLevelSize = levelSize[i] + 1;
    }


    // allocate space for the faulty gates
    for (int i = numgates; i < numgates+64; i+=2) {
        inlist[i].resize(2);
        fnlist[i].resize(MAXFanout);
        po[i] = 0;
        fanin[i] = 2;
        inlist[i][0] = i+1;
    }


    // get the ffMap
    for (int i=0; i<numff; ++i)
		ffMap[ff_list[i]] = i;


    // if start from a initial state, then record the data into the RESET state config
    if (initFromFile) {
		RESET_FF1.resize(numff+2);
		RESET_FF2.resize(numff+2);

		fName = cktName + ".initState";
		yyin.open(fName.c_str(), ios::in);
		if (!yyin) {	
			cerr << "Can't open " << fName << "\n";
			exit(-1);
		}

		for (int i=0; i<numff; ++i) {
		    yyin >>  c;

		    if (c == '0') {
				RESET_FF1[i] = 0;
				RESET_FF2[i] = 0;

		    } else if (c == '1') {
				RESET_FF1[i] = ALLONES;
				RESET_FF2[i] = ALLONES;

		    } else {
				RESET_FF1[i] = 0;
				RESET_FF2[i] = ALLONES;
		    }
		}

		yyin.close();
    }


    is_mgate.resize(numgates+64,0);

    setFaninoutMatrix();
    existsMysteryGate();

    // Print read result
    cout << "Successfully read in circuit:\n";
    cout << "\t" << numpri << " PIs.\n";
    cout << "\t" << numout << " POs.\n";
    cout << "\t" << numff << " Dffs.\n";
    cout << "\t" << numFaultFreeGates << " total number of gates\n";
    cout << "\t" << maxlevels / 5 << " levels in the circuit.\n";
}



/**
 *
 * Checks the existence of mystery gates and enlists them
 *
 */
void gateLevelCkt::existsMysteryGate() {
    int i;
    nummg=0;

    printf("\n");
    for(i=1; i<numgates; i++){
        if(gtype[i]==T_mystery){
            printf("[INFO] Found Mystery Gate at [GATE#%d]\n", i);

            mystery_gate.push_back(i);
            is_mgate[i] = 1;
            nummg++;
        }
    }
}


/**
 *
 * Build suspect list
 *
 */
void gateLevelCkt::buildPossibilitiesList(vector< vector<int> > & gate_psblty) {
    // Populate suspect list with all possible gates (whatever the Simulator enumerates)

    if(gate_psblty.size()==0)
    	gate_psblty.resize(nummg);

    for(int i=0; i<nummg; i++){
        // if Single input, then the obvious suspect is the NOT gate or buffer. Obvious.
        // Else add the other gates
        vector<int> gp;

        if(fanin[mystery_gate[i]]>1){
            gp.push_back(T_and);
            gp.push_back(T_nand);
            gp.push_back(T_or);
            gp.push_back(T_nor);
            
            // If the fanin is more than 2, it's not a XOR/XNOR gate
            if(fanin[mystery_gate[i]]==2){
                gp.push_back(T_xor);
                gp.push_back(T_xnor);
            }
        } else {
            gp.push_back(T_not);
            gp.push_back(T_buf);
        }

        gate_psblty[i] = gp;
    }
}


/**
 *
 * randomly set one gate as random
 *
 */

void gateLevelCkt::insertRandomMystery(){
	int k;

	k = rand()%numgates;

	while(
		gtype[k]==T_input ||
		gtype[k]==T_output ||
		gtype[k]==T_dff ||
		gtype[k]==JUNK ||
		gtype[k]==T_buf ||
		gtype[k]==T_not ||
		is_mgate[k]
		) {
		k = rand()%numgates;
	}

	printf("Obfuscating [g%d, %s]\n", k, gate_names[gtype[k]]);

	mystery_gate.push_back(k);
	is_mgate[k] = 1;
	nummg++;
}