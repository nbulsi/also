#pragma once
#ifndef NODE_H
#define NODE_H

#include <mockturtle/mockturtle.hpp>
#include "./exact_sto_m3ig.hpp"
#define MULTIPLE (1.5)
//#define INT_MAX 2147483647
#define w 2

#include <vector>
#include <stack>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <string>
#include <unordered_set>
#include <fstream>
#include <unordered_map>
#include <sstream>
#include <set>
#include "hash_extend.h"
#include<climits>
//#include "../../lib/mockturtle/include/mockturtle/algorithms/simulation.hpp"
//#include <mockturtle/mockturtle.hpp>
//#include "./exact_sto_m3ig.hpp"

using namespace std;
using namespace also;

#define LITERAL_LIMIT_PARAM_w 2 // prune by literal count in "processNodeVector()"
#define X_COMB_PARAM_h 5 // keep first k x-cubes for each line cube vector

typedef vector<string> AssMat; // assignment matrix
typedef vector< int> MintermVector; // cube vector [0, 1, 2, 1]
typedef vector< int> AssVec; // assignment vector [0, 1, 5, 4] ([000 001 101 100])
typedef vector<string> StrAssVec; // string assignment vector ["000", "001", "101", "100"]

// !!! IMPORTANT! the first int is the log2 of the line
// i.e., if a CubeDecomposition has 16 lines,
// then the int should be 4!
typedef pair< int, vector<MintermVector>> CubeDecomposition; 

// use pair instead
//struct cubeDecompostion
//{
//    cubeDecompostion( int L, vector<MintermVector> CV) : line(L), cubeVectors(CV) {}
//
//     int line;
//    vector<MintermVector> cubeVectors;
//};

extern unordered_map< int, vector<CubeDecomposition>> unorderedMapOfPossibleCubeDecompositionVector;

struct Node
{
    // TODO: constructor to be written
    Node(): _level(0), _literalCountSoFar(0)
    {
    }

    //Node(AssMat newAssMat, MintermVector newProblemVector,  int newLevel, unordered_multiset<CubeDecomposition> newAssignedCubeDecompositions, CubeDecomposition lastAssignedCubeDecomposition,  int literalCount);

    Node(AssMat newAssMat, MintermVector newProblemVector,  int newLevel, vector<CubeDecomposition> newAssignedCubeDecompositions, CubeDecomposition lastAssignedCubeDecomposition,  int literalCount);

    // assigned "assignment matrix"
    AssMat _assignedAssMat;

    // the remaining problem vector
    MintermVector _remainingProblemVector;

    // the level in the tree; root is _level 0
     int _level;

    // the set of cubes that are already assigned
    //unordered_multiset<CubeDecomposition> _assignedCubeDecompositions;
    vector<CubeDecomposition> _assignedCubeDecompositionsVec;

    // the last cube vector assigned
    CubeDecomposition _lastAssignedCubeDecomposition;

    // the literal count so far
     int _literalCountSoFar;
};

class SolutionTree
{
public:

	
    // methods

    // constructor
    // TODO: complete the constructor
    SolutionTree(vector< int> initialProblemVector, vector< int> degrees,  int accuracy,  int caseNumber,int variableNumber);

    // process the tree
    void ProcessTree();

    // process a single Node
    vector<Node> ProcessNode(Node currentNode);

    // process the node vector, which has 
    vector<Node> ProcessNodeVector(vector<Node> nodeVecToBeProcessed);

    AssMat AssignMatrixByEspresso(AssMat originalAssMat, CubeDecomposition cubeDecompositionToBeAssigned) const;

    vector<AssMat> AssignMatrixByEspressoVector(AssMat originalAssMat, CubeDecomposition cubeDecompositionToBeAssigned) const;

    // find the assignment sets for a single vector
    vector<set<string>> FindAssignmentSetsOfStringForMintermVector(MintermVector lineCubeVector) const;

    set<string> BuildBasicAssignmentSet( int mintermCount) const;

    // get all of the assignment sets by recursion, although the name is "ForCubeDecomposition"
    // the input is not a CubeDecomposition, but the assignment sets for each cube vector in the decomposition
    // INPUT: vector<vector<set<string>>> assignmentSetsVector
    // each element in this vector is a vector<set<string>>, which contains
    // all of the assignment sets of the corresponding cube component in the decomposition
    // OUTPUT: vector<set<string>> ret
    // it contains all of the assignment sets -- by "multiplication" we can get them
    vector<set<string>> FindAssignmentSetsOfStringForCubeDecomposition(vector<vector<set<string>>> assignmentSetsVector) const;
    vector<set<string>> FindAssignmentSetsOfStringForCubeDecompositionHelper(vector<vector<set<string>>> remainingAssignmentSetsVector, vector<set<string>> currentAssignmentSets) const;


    // data
     int _caseNumber;
     int _log2LengthOfTotalCube; // d1 + d2 + ...
    vector< int> _degrees;
     int _accuracy;

   	unsigned num_vars;
	unsigned m;
	unsigned n;
	vector<unsigned> vec; 
	//mig_network stochastic_synthesis( num_vars, m, n, vec );

    vector<Node> _nodeVector;
     int _minLiteralCount;   // current minimum literal count
    Node _optimalNode;
    vector<Node> _optimalNodes; 
    unordered_map< int,  int> _minLiteralCountOfLevel;

    vector< int> _rowGrayCode;
    vector< int> _colGrayCode;

    //unordered_map<unordered_multiset<CubeDecomposition>, bool> _processedCubeDecompositions; // true if the set is processed before
    //unordered_map<vector<CubeDecomposition>, bool> _processedCubeDecompositionsVec;
    unordered_map<size_t, bool> _existingMatrices; // true if the matrix exists
    unordered_map<size_t, bool> _processedCubeDecompositionsOrderedSeed; 
    unordered_map<size_t, bool> _processedCubeDecompositionsUnorderedSeed;

     int _updateTime;
     int _nodeNumber;
     int _maxLevel;
    unordered_map< int,  int> _sizeOfCubeInLevel;
};

// multiply number and a vector
MintermVector multiply( int line, MintermVector cubeVec);

// multiply two cube vectors
MintermVector multiply(MintermVector cubeVec1, MintermVector cubeVec2);

// multiply n cube vectors, the size of cubeVecs should be greater than 1
MintermVector multiply(vector<MintermVector> cubeVecs);

string IntToBin( int num,  int highestDegree);
 int BinToInt(string str);

bool CapacityConstraintSatisfied(vector< int> problemVector, MintermVector cubeVector);

MintermVector SubtractCube(MintermVector problemVector, MintermVector cube);

bool IsZeroMintermVector(MintermVector vec);

long long  int choose( int n,  int k);

vector< int> ConstructGrayCode( int size);
vector< int> ConstructGrayCodeHelper(vector< int> grayCode);

vector<string> BuildZeroOneTwoPermutation( int countOfZero,  int countOfOne,  int countOfTwo);

set<string> MultiplyAssignmentSets(set<string> set1, set<string> set2);
set<string> MultiplyAssignmentSets(vector<set<string>> sets);

// returns all of the possible cube decompositions
// INPUT: the log2 of the cube size
// OUTPUT: the vector of the possible CubeDecomposition
// NOTE: this method will use _accuracy and the _degrees
vector<CubeDecomposition> PossibleCubeDecompositions( int log2CubeSize, vector< int> degrees,  int accuracy);
vector<CubeDecomposition> PossibleCubeDecompositionsHelper( int remainingLog2CubeSize, vector<CubeDecomposition> partialDecompositions, vector< int> remainingDegrees,  int accuracy);
vector<MintermVector> PossibleLineCubeVectors( int log2CubeSize,  int degree);

vector<set<string>> BuildAssignmentSet(set<string> basicAssignmentSet,  int countOfZero,  int countOfOne);

#endif
