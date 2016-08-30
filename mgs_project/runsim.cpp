#include "gateLevelCkt.h"
#include "eventSimulator.h"


/*===============================
=            Globals            =
===============================*/

gateLevelCkt ckt;

// flags
int OBSERVE, INIT0, DEBUG, VERIFY, IS_SEQ, CHECK_FF;

int inputVector_size;
int outputVector_size;

vector<string> all_InputVector;
vector<string> all_OutputVector;
vector<string> all_OutputFF;


/*----------  Functions  ----------*/

void getSimVectors(string);
void genSimVectors(const gateLevelCkt&);
string rand_inputvector(int);
void logicSim(const gateLevelCkt&);



/*============================
=            Main            =
============================*/
string fileName;

int main(int argc, char *argv[]){
    srand((unsigned int)time(NULL));
	fileName = argv[1];

	// Read the circuit
	ckt.readNetlist(fileName);
	// Read simulation vectors
	genSimVectors(ckt);
	// Run simulation
	logicSim(ckt);

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

// Generates a random input vector
string rand_inputvector(int l){
    string iv;

    for(int i=0;i<l;i++){
        iv += (rand()%2) ? '1' : '0';
    }

    return iv;
}

// Generate random input vectors
void genSimVectors(const gateLevelCkt& circuit) {
    printf("#PI: %d\n", circuit.numpri);
    for(int i=0; i<256; i++) {
        all_InputVector.push_back(rand_inputvector(circuit.numpri));
    }
}

// Simulate the circuit with the input vectors that were read in
void logicSim(const gateLevelCkt& circuit) {
	eventSimulator sim(circuit);

	string inputVector;
	string outputVector;
	string outputFF;

    string vecFileName = fileName + ".vec";
    string outFileName = fileName + ".out";
    ofstream vecfile;
    vecfile.open(vecFileName);
    ofstream outfile;
    outfile.open(outFileName);

    vecfile << circuit.numpri << endl;

    for(int k=0; k<all_InputVector.size(); k++){
        // get current vectors
        inputVector = all_InputVector[k];
        
        // Nothing much to do here
        // Simulate the vectors for the given netlist
        sim.applyVector(circuit, inputVector);
        sim.goodsim(circuit);
        cout << "vector #" << k << ": " << inputVector << "\n";
        sim.printOutputs(circuit, outputVector, outputFF);
        cout<<endl;

        //TO: out file
        outfile << "vector #" << k << ": " << inputVector << "\n";
        outfile << "\t" << outputVector;
        outfile << endl << endl;

        //TO: vec file
        vecfile<<inputVector<<endl;

    }

    vecfile<<"END\n";

    vecfile.close();
    outfile.close();
}