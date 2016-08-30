#include "gateLevelCkt.h"
#include "eventSimulator.h"
#include <omp.h>


/*===============================
=            Globals            =
===============================*/
int threadCount=8;


// netlist object
gateLevelCkt ckt;

// flags
int OBSERVE, INIT0, DEBUG, VERIFY, IS_SEQ, CHECK_FF;

// Simulator I/O
int inputVector_size;
int outputVector_size;
int numTests;

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
void permuteVector(vector<int>, int, const vector< vector<int> >&);
void logicSim_SAT(int, const eventSimulator&);
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

    threadCount = strtol(getenv ("NUMTHREADS"), NULL, 10);
    printf("NumThreads = %d\n\n", threadCount);

    // Read the circuit
    fileName = argv[nameIndex];
    ckt.readNetlist(fileName);
    if(ckt.numff) IS_SEQ=1;

    // Read simulation vectors
    getSimVectors(fileName);

    // Find Mystery gates!
    mystery_gate_count = ckt.nummg;
    if(mystery_gate_count){
        solved = 0;
        ckt.buildPossibilitiesList(gate_possibilities);

        #pragma omp parallel num_threads(threadCount) \
            default(none) \
            shared(TEST_COMBINATION, all_InputVector, all_OutputVector, all_OutputFF, \
            IS_SEQ, DEBUG, ckt, mystery_gate_count, gate_names, CHECK_FF, cout, gate_possibilities)
        {
            #pragma omp single nowait
            {
                eventSimulator sim(ckt);
                sim.setSuspectList(gate_possibilities);
                logicSim_SAT(0, sim);
            }
        }

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
    numTests = all_InputVector.size();

    printf("\nNumber of vectors read: %d\n", numTests);
    printf("Input Vector size: %d\n", inputVector_size);
    printf("Output Vector size: %d\n\n", outputVector_size);
}


// Generates permutations of the possible suspects eligible for each mystery gate
void permuteVector(vector<int> v, int index, const vector< vector<int> >& gate_psblty) {
    for(unsigned int p=0; p<gate_psblty[index].size(); p++) {

        v[index] = gate_psblty[index][p];
        
        if(index==mystery_gate_count-1){
            TEST_COMBINATION.push_back(v);
        } else {
            permuteVector(v, (index+1), gate_psblty);
        }
    }
}



// Simulate the circuit with the input vectors that were read in
void logicSim_SAT(int test_index, const eventSimulator& sim) {

    if(test_index>=numTests){
        #pragma omp critical 
        {
            vector<int> v(mystery_gate_count);
            permuteVector(v, 0, sim.suspect_list);
            if(DEBUG){
                cout<<"\n[INFO] Converged: {";
                for(int mgate=0; mgate<mystery_gate_count; ++mgate)
                    cout<<gate_names[sim.suspect_list[mgate][0]]<<" ";
                cout<<"}";
            }
            solved=1;
        }
    } else {

        string inputVector;
        string outputVector;
        string outputFF;

        // get current sim I/O
        inputVector = all_InputVector[test_index];
        outputVector = all_OutputVector[test_index];

        if(IS_SEQ)
            outputFF = all_OutputFF[test_index];

        if(!sim.isConverged){
            /*----------  SEARCH MODE  ----------*/

            // These are possible stuck at values the gates can take at any point in time.
            // For 1 gate, there are 2 possibilites
            // For 2 gates, there are 4 combinations
            // For 5 gates - 32... and so on
            int test_vector_max = pow(2,sim.non_UnitClauses);
            vector<int> TEST_VALUE(sim.non_UnitClauses);
            

            for(int trial=0; trial<test_vector_max; trial++) {
                    // clone sim object
                    eventSimulator mysim(ckt, sim);      

                    // Set the TEST value for each mystery gate, according to the current trial
                    // We plant a "Stuck-at" fault at each mystery gate
                    if(DEBUG) {
                        printf("\n[%d] {IN: %s, OUT: %s}\n", test_index, inputVector.c_str(), outputVector.c_str());
                        printf("[%d] Test Vector (%d): ", test_index, trial);
                    }

                    for(int i=0; i<sim.non_UnitClauses; i++){
                        TEST_VALUE[i] = ((trial>>i) & 0x1) ? ALLONES : 0;
                        if(DEBUG) printf("%u, ", (TEST_VALUE[i] & 0x1));
                    }
                    if(DEBUG) printf("\n");


                    mysim.tieValue_MG(ckt, TEST_VALUE);  
                    mysim.propagateMysteryEvents(ckt);
                    mysim.applyVector(ckt, inputVector);
                    mysim.goodsim(ckt);

                    if(DEBUG) printf("Circuit Output: ");
                    if(mysim.observeOutputs(ckt, outputVector, outputFF, DEBUG, CHECK_FF)) {
                        if(DEBUG) printf("     PASS\n");
                        // We just caught a good setup of the stuck-at faults. 
                        // Let's test the hypothesis, i.e. reduce the suspect list
                        if(mysim.iterrogate_suspects(ckt, TEST_VALUE, DEBUG)){
                            // Persue this path further...
                            #pragma omp task
                            logicSim_SAT((test_index+1), mysim);
                        } else {
                            if(DEBUG) printf("[INFO] FAILED to converge.\n");
                        }

                    } else {
                        if(DEBUG) printf("     FAIL\n");
                    }
            }

        } else {
            /*----------  VERIFICATION MODE  ----------*/
            
            // clone sim object
            eventSimulator mysim(ckt, sim);

            // set the sim to the trial combination
            // for(int mgate=0; mgate<mystery_gate_count; mgate++)
            //     mysim.setGateOverride(ckt.mystery_gate[mgate], mysim.suspect_list[mgate][0]);

            mysim.propagateMysteryEvents(ckt);
            mysim.applyVector(ckt, inputVector);
            mysim.goodsim(ckt);

            if(DEBUG) printf("Circuit Output: ");
            if(mysim.observeOutputs(ckt, outputVector, outputFF, DEBUG, CHECK_FF)) {
                if(DEBUG) printf("     PASS\n");
                logicSim_SAT((test_index+1), mysim);
            } else {
                if(DEBUG) printf("     FAIL\n");
            }
        }
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
