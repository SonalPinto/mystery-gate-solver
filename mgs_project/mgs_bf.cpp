#include "gateLevelCkt.h"
#include "eventSimulator.h"


/*===============================
=            Globals            =
===============================*/

// netlist object
gateLevelCkt ckt;

// flags
int OBSERVE, INIT0, DEBUG, VERIFY, IS_SEQ, CHECK_FF;

// Simulator I/O
int inputVector_size;
int outputVector_size;

vector<string> all_InputVector;
vector<string> all_OutputVector;
vector<string> all_OutputFF;

// Mystery Gate Solver vars
int solved;
int mystery_gate_count;
vector < vector<int> > gate_possibilities;
vector < vector<int> > TEST_COMBINATION;


/*----------  Functions  ----------*/

void getSimVectors(string);
void logicSim_BF();
void permuteVector(vector<int>, int);
void printResults();


/*============================
=            Main            =
============================*/

int main(int argc, char *argv[]){

    int nameIndex, i;
    string fileName;

    // Defaults
    nameIndex = 1;
	OBSERVE = 0;
	INIT0 = 0;
    DEBUG = 0;
    VERIFY = 0;
    CHECK_FF = 0;
    IS_SEQ = 0;


    // Parse inputs
    if ((argc != 2) && (argc != 3)) {
        cerr << "Usage: " << argv[0] << "[-io] <ckt>\n";
        cerr << "The -i option is to begin the circuit in a state in *.initState.\n";
        cerr << "and -o option is to OBSERVE fault-free outputs and FF's.\n";
        cerr << "and -d option is to see the Debug information.\n";
        cerr << "and -f option is to CHECK FLIPFLOP outputs while validating. Default false.\n";
        cerr << " Example: " << argv[0] << " s27\n";
        cerr << " Example2: " << argv[0] << " -o s27\n";
        cerr << " Example3: " << argv[0] << " -io s27\n";
        exit(-1);
    }

    if (argc == 3) {
        i = 1;
        nameIndex = 2;
        while (argv[1][i] != EOS) {
            switch (argv[1][i]) {
            case 'o':
                OBSERVE = 1;
                break;
            case 'i':
                INIT0 = 1;
                break;
            case 'd':
                DEBUG = 1;
                break;
            case 'f':
                CHECK_FF = 1;
                break;
            default:
                    cerr << "Invalid option: " << argv[1] << "\n";
                    cerr << "Usage: " << argv[0] << " [-io] <ckt> <type>\n";
                    cerr << "The -i option is to begin the circuit in a state in *.initState.\n";
                    cerr << "and -o option is to OBSERVE fault-free outputs.\n";
                    cerr << "and -d option is to see the Debug information.\n";
                    cerr << "and -f option is to CHECK FLIPFLOP outputs while validating. Default false.\n";
                    cerr << " Example3: " << argv[0] << " -vd c17\n";
                exit(-1);
                break;
            }
            i++;
        }
    }

    // Read the circuit
	fileName = argv[nameIndex];
	ckt.readNetlist(fileName);
    if(ckt.numff) IS_SEQ=1;

	// Read simulation vectors
	getSimVectors(fileName);

    // Find Mystery gates!
    mystery_gate_count = ckt.nummg;
    if(mystery_gate_count){
        logicSim_BF();

        if(solved){
            cout<<"\n\nSOLVED!\n";
            cout<<"Mystery Gate possibilites: \n\n";
            printResults();
        } else {
            cout<<"\n\n[ERROR]: Could not solve! How is this possible?!\n";
        }

    } else {
        cout<<"\n\nThere's no mystery here to solve. Case closed...\n";
    }

	return 0;
}


/*=================================
=            FUNCTIONS            =
=================================*/

// Read all input and output vectors and populate the list
void getSimVectors(string circutName){
    int i;
    string s, vecName, outName;
    ifstream inFile, outFile;
    char thisChar, buffer[MAXFFS];

    vecName = circutName;
    vecName += ".vec";
    outName = circutName;
    outName += ".out";

    inFile.open(vecName.c_str(), ios::in);
    outFile.open(outName.c_str(), ios::in);

    if (!inFile) {
        cerr << "Can't open " << vecName << "\n";
        exit(-1);
    }

    if (!outFile) {
        cerr << "Can't open " << outName << "\n";
        exit(-1);
    }

    inFile >> buffer;

    // Process INPUT vector file
    while(inFile>>buffer){
        if(buffer[0] == 'E') break;
        s = buffer;
        all_InputVector.push_back(s);
    }

    // Process OUTPUT vector file
    while(outFile>>buffer) {
        outFile >> buffer >> buffer >> buffer;
        s = buffer;
        all_OutputVector.push_back(s);
        outFile >> thisChar;
        if(thisChar != 'v') {
            outFile >> buffer;
            s = thisChar;
            s +=  buffer;
            all_OutputFF.push_back(s);
        }
    }

    inFile.close();
    outFile.close();

    inputVector_size = all_InputVector[0].size();
    outputVector_size = all_OutputVector[0].size();

    printf("\nNumber of vectors read: %lu\n", all_InputVector.size());
    printf("Input Vector size: %d\n", inputVector_size);
    printf("Output Vector size: %d\n\n", outputVector_size);
}



// Generates and stores various permutations of the possible suspects eligible for each mystery gate
// Requried for Brute Force method
void permuteVector(vector<int> v, int index) {
    for(unsigned int p=0; p<gate_possibilities[index].size(); p++) {

        v[index] = gate_possibilities[index][p];
        
        if(index==mystery_gate_count-1){
            TEST_COMBINATION.push_back(v);
        } else {
            permuteVector(v, (index+1));
        }
    }
}



// Simulate the circuit with the input vectors that were read in
void logicSim_BF() {
    // sim object
	eventSimulator sim(ckt);

	string inputVector;
	string outputVector;
	string outputFF;

    // build suspect list
    ckt.buildPossibilitiesList(gate_possibilities);
    // permute possible solutions from the suspect list and build TEST_COMBINATION
    vector<int> current_suspect(mystery_gate_count);
    permuteVector(current_suspect, 0);

    for(int trial=0; trial<TEST_COMBINATION.size(); trial++){

        current_suspect = TEST_COMBINATION[trial];

        // reset the sim, when dealing with a new trial combination
        sim.reset(ckt);

        // set the sim to the trial combination
        for(int mgate=0; mgate<mystery_gate_count; mgate++)
            sim.setGateOverride(ckt.mystery_gate[mgate], current_suspect[mgate]);

        // Start with assuming the puzzle isnt solved, obviously
        solved=0;

        if(DEBUG){
            printf("\n====================\n[Gates-Under-Test] { ");
            for(int mgate=0; mgate<mystery_gate_count; mgate++) 
                printf(" g%d = %s, ", ckt.mystery_gate[mgate], gate_names[current_suspect[mgate]]);
            printf("}\n");
        }


        // Run all vectors for each test solution
        for(int k=0; k<all_InputVector.size(); k++){
            // get current sim I/O
            inputVector = all_InputVector[k];
            outputVector = all_OutputVector[k];

            if(IS_SEQ)
                outputFF = all_OutputFF[k];
            
            // Fresh Vector!
            if(DEBUG)
            	// printf("vector #%d: %s\n", k, inputVector.c_str());
            	printf("[%d] {IN: %s, OUT: %s}\n", k, inputVector.c_str(), outputVector.c_str());

            // Nothing much to do here
            // Simulate the vectors for the given netlist
            sim.applyVector(ckt, inputVector);
            sim.goodsim(ckt);
            if(DEBUG) printf("Circuit Output: ");
            if(sim.observeOutputs(ckt, outputVector, outputFF, DEBUG, CHECK_FF)) {
                if(DEBUG) printf("     PASS\n");
                solved=1;
            } else {
                if(DEBUG) printf("     FAIL\n");
                solved=0;
                break;
            }
        }


        // Erase the bad trial from the list
        if(solved==0){
            TEST_COMBINATION.erase(TEST_COMBINATION.begin() + trial);
            trial--;
        } else {
            // Good Combo!
            if(DEBUG){
                cout<<"[INFO] Converged: {";
                for(int mgate=0; mgate<mystery_gate_count; ++mgate)
                    cout<<gate_names[current_suspect[mgate]]<<" ";
                cout<<"}\n";
            }
        }
    }


    // Some final cleanup and print
    if(TEST_COMBINATION.size()){
        solved = 1;
    } else {
        solved = 0;
    }
}



// Print remaining trial combinations that succesfully simulate the circuit for the given combinations
void printResults(){
    int i,k;

    printf("\n\n\n");
    for(i=0; i<mystery_gate_count; i++) 
        printf("------------");
    printf("-\n");
    for(i=0; i<mystery_gate_count; i++) 
        printf("| g%-8d ", ckt.mystery_gate[i]);
    printf("|\n");
    for(i=0; i<mystery_gate_count; i++) 
        printf("------------");
    printf("-\n");

    for(k=0; k<TEST_COMBINATION.size(); k++) {
        for(i=0; i<mystery_gate_count; i++) 
            printf("|%10s ", gate_names[TEST_COMBINATION[k][i]]);
        printf("|\n");
    }

    for(i=0; i<mystery_gate_count; i++) 
        printf("------------");
    printf("-\n\n\n");
}
