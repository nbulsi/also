//#include "./exact_sto_m3ig.hpp"
#include "../../lib/mockturtle/include/mockturtle/networks/mig.hpp"
#include "node.h"
#include<stdlib.h>
#include <fstream>
#include<string>
#include<algorithm>

using namespace mockturtle;
using namespace also;
unordered_map< int, vector<CubeDecomposition>> unorderedMapOfPossibleCubeDecompositionVector;

SolutionTree::SolutionTree(vector< int> initialProblemVector, vector< int> degrees,  int accuracy,  int caseNumber,int variableNumber )
{
    // NOTE: _log2LengthOfTotalCube equals to d1 + d2 + ... + dk
    _log2LengthOfTotalCube = 0;
    for (auto d : degrees)
    {
        _log2LengthOfTotalCube += d;
    }
    _degrees = degrees;
    _accuracy = accuracy;
    _caseNumber = caseNumber;
    _minLiteralCount = INT_MAX;
    num_vars = (unsigned)variableNumber;
	m= (unsigned)_accuracy;
	n= (unsigned)_log2LengthOfTotalCube;

    // initialize root node
    auto lengthOfTotalCube =  int(pow(2, _log2LengthOfTotalCube));
    auto powAccuracy =  int(pow(2, _accuracy));
    auto assignmentMatrix = AssMat(powAccuracy, string(lengthOfTotalCube, '0'));

    //auto root = Node(assignmentMatrix, initialProblemVector, 0, unordered_multiset<CubeDecomposition>(), CubeDecomposition(), 0);
    auto root = Node(assignmentMatrix, initialProblemVector, 0, vector<CubeDecomposition>(), CubeDecomposition(), 0);

    _nodeVector.push_back(root);

    // initialize other data
    _updateTime = 0;
    _nodeNumber = 1;
    _maxLevel = 0;
    _optimalNode = Node();
    _optimalNodes = vector<Node>();
}

void SolutionTree::ProcessTree()
{
   ofstream re("result.txt");
    //re << "-------------------------------SolutionTree::ProcessNodeVector----------------------------------" << endl;
    auto processedNodeNumber =  int(_nodeVector.size());

    while (!_nodeVector.empty())
    {
        // process all of the nodes in the vector
        auto nodeVectorOfNextLevel = ProcessNodeVector(_nodeVector);

        // update the node vector
        _nodeVector = nodeVectorOfNextLevel;

        // add to the count
        processedNodeNumber +=  int(_nodeVector.size());
    }

    //cout << "Node processed: " << processedNodeNumber << endl << endl;

    assert(IsZeroMintermVector(_optimalNode._remainingProblemVector));
    assert(!_optimalNode._assignedAssMat.empty());

    // Display the result
    cout << "BEGIN: display final result" << endl;
    //cout << "The matrix is" << endl;
    //for (auto line : _optimalNode._assignedAssMat)
    //{
    //    cout << line << endl;
    //}
    //cout << "The assigned cube decompositions are" << endl;
    //for (auto cDcmp : _optimalNode._assignedCubeDecompositions)
    //{
    //    cout << "2^" << cDcmp.first;
    //    for (auto c : cDcmp.second)
    //    {
    //        cout << " X [ ";
    //        for (auto m : c)
    //        {
    //            cout << m << " ";
    //        }
    //        cout << "]";
    //    }
    //    cout << endl;
    //}
    cout << "The minimum literal number is " << _minLiteralCount << endl;
    auto result = ofstream("result.txt", ofstream::app);
    result << "The minimum literal number is " << _minLiteralCount << endl;
    result.close();
    //cout << "The branch number is " << _nodeNumber << endl;
    //cout << "END: display final result" << endl;
    //cout << endl;

    //// write the .pla file
    //stringstream ss;
    //ss << "pla\\";
    //ss.fill('0');
    //ss.width(2);
    //ss << _caseNumber << ".pla";
    //auto plaFile = ofstream(ss.str());
    //plaFile << ".i " << _log2LengthOfTotalCube + _accuracy << endl;
    //plaFile << ".o 1" << endl;

    //for (auto line = 0; line < int(_optimalNode._assignedAssMat.size()); ++line)
    //{
    //    for (auto col = 0; col < int(_optimalNode._assignedAssMat[line].size()); ++col)
    //    {
    //        if (_optimalNode._assignedAssMat[line][col] == '0') continue;
    //        plaFile << IntToBin(col, _log2LengthOfTotalCube - 1) << IntToBin(line, _accuracy - 1) << " 1" << endl;
    //    }
    //}
    //plaFile << ".e" << endl;

    // process the _optimalNodes vector
    cout << "Solution number: " << _optimalNodes.size() << endl << endl;

    auto r = ofstream("result.txt", ofstream::app);
    r << "Solution number: " << _optimalNodes.size() << endl << endl;
    r.close();

    for (auto sol = 0; sol < _optimalNodes.size(); ++sol)
    {
        stringstream ss;
        ss << "pla\\case-";
        ss.fill('0');
        ss.width(2);
        ss << _caseNumber << "\\";
        ss << "solution-";
        ss.fill('0');
        ss.width(3);
        ss << sol << ".pla";
		auto plaFile = ofstream(ss.str(), ofstream::app );
        plaFile << ".i " << _log2LengthOfTotalCube + _accuracy << endl;
        plaFile << ".o 1" << endl;

        for (auto line = 0; line <  int(_optimalNodes[sol]._assignedAssMat.size()); ++line)
        {
            for (auto col = 0; col <  int(_optimalNodes[sol]._assignedAssMat[line].size()); ++col)
            {
                if (_optimalNodes[sol]._assignedAssMat[line][col] == '0') continue;
                plaFile  << IntToBin(col, _log2LengthOfTotalCube - 1) << IntToBin(line, _accuracy - 1) << " 1" << endl;////竖
                //plaFile << IntToBin(line, _accuracy - 1) << IntToBin(col, _log2LengthOfTotalCube - 1) << " 1" << endl;////横
            }
        }
        plaFile << ".e" << endl;
    }
}

vector<Node> SolutionTree::ProcessNode(Node currentNode)
{
    
    //using namespace also;
    // the vector of nodes to be returned
    // they should have the same literal count
    // and the same size, the size must be the largest size
    auto ret = vector<Node>{};

    // currentNode is originally "cNode"

    // at first, we sum up the number of minterms
    //// mintermCount is originally numMinTerm
    auto mintermCount = 0;
    for (auto i : currentNode._remainingProblemVector)
    {
        mintermCount += i;
    }
    cout << "mintermCount:" << mintermCount << endl;

    auto result = ofstream("result.txt", ofstream::app);
    result << "mintermCount:" << mintermCount << endl;
    result.close();

    // then, we find the maximal possible cubes

    auto found = false;

    // log2MintermCount is originally MTNinCube
    // It is the log2 of minterm count in the cube
    auto log2MintermCount =  int(floor(log2(mintermCount)));

    // the main loop;
    while (!found && (log2MintermCount >= 0))
    {
        auto possibleCubeDecompositionVector = vector<CubeDecomposition>{};
        if (unorderedMapOfPossibleCubeDecompositionVector[log2MintermCount].empty())
        {
            // originally, the function here is "possibleCubes"
            // but for multi-var, it should return possible cube 
            // decompositions -- CubeDecomposition, 
            // i.e., vector<int, vector<MintermVector>>
            possibleCubeDecompositionVector = PossibleCubeDecompositions(log2MintermCount, _degrees, _accuracy);
            unorderedMapOfPossibleCubeDecompositionVector[log2MintermCount] = possibleCubeDecompositionVector;
        }
        else
        {
            possibleCubeDecompositionVector = unorderedMapOfPossibleCubeDecompositionVector[log2MintermCount];
        }

        // traverse all of the possible cube decompositions

        for (auto cubeDecomposition : possibleCubeDecompositionVector)
        {
            // first we need to get the vector form
            // i.e., we multiply the vectors to get
            // the total vector:
            // (v0, v1, v2)X(u0, u1)=(w00, w10, w20, w01, w11, w21)
            auto totalCubeVector = multiply( int(pow(2, cubeDecomposition.first)), multiply(cubeDecomposition.second));
			//unsigned int m=_accuracy;
			//unsigned int n=_degrees;
			//mig_network mig;
			//mig= stochastic_synthesis( 1, m, n, totalCubeVector );
			//default_simulator<kitty::dynamic_truth_table> sim( m+n );
			//const auto tt = simulate<kitty::dynamic_truth_table>( mig, sim )[0];
			// const auto tt = simulate<kitty::static_truth_table<3u>>( mig )[0];
			//kitty::print_binary(tt, std::cout);

            // checking the capacity constraint
            if (!CapacityConstraintSatisfied(currentNode._remainingProblemVector, totalCubeVector))
            {
                // if "currentNode._remainingProblemVector" does not contain the
                // cube with the vector of "totalCubeVector"
                // skip this "cubeVector" and process the next one
                continue;
            }

            // "newAssignedCubeDecompositions" is the so called "cube set"
            // it is "new" because the new set will replace the old one
            // Here we declare a new set instead of inserting
            // a vector into the old one because in some cases
            // we will prune this node and keep the old set unchanged
            //auto newAssignedCubeDecompositions = currentNode._assignedCubeDecompositions;
            auto newAssignedCubeDecompositionsVec = currentNode._assignedCubeDecompositionsVec;

            // insert the new cube decomposition
            //newAssignedCubeDecompositions.insert(cubeDecomposition);
            newAssignedCubeDecompositionsVec.push_back(cubeDecomposition);
            auto newAssignedCubeDecompositionsSet = unordered_multiset<CubeDecomposition>(newAssignedCubeDecompositionsVec.begin(), newAssignedCubeDecompositionsVec.end());

            // checking for the dupe cube set
            //if (_processedCubeDecompositions[newAssignedCubeDecompositions] == true)
            //{
            //    // if this cube set is processed before
            //    // prune it and display the deletion
            //    continue;
            //    // comment out
            //    //cout << "** BEGIN:Pruning DUPE cube set **" << endl;
            //    //cout << "Following decomposition is duped: " << endl;
            //    // TODO: display the decomposition
            //}
            size_t unorderedSeed = 0;
            size_t orderedSeed = 0;
            hash<unordered_multiset<CubeDecomposition>> unorderedHasher;
            hash<vector<CubeDecomposition>> orderedHasher;
            unorderedSeed = unorderedHasher(newAssignedCubeDecompositionsSet);
            orderedSeed = orderedHasher(newAssignedCubeDecompositionsVec);

            if ((_processedCubeDecompositionsUnorderedSeed[unorderedSeed] == true) && (_processedCubeDecompositionsOrderedSeed[orderedSeed] == false))
            {
                continue;
            }

            // set the processed cubeset
            //_processedCubeDecompositions[newAssignedCubeDecompositions] = true;
            _processedCubeDecompositionsOrderedSeed[orderedSeed] = true;
            _processedCubeDecompositionsUnorderedSeed[unorderedSeed] = true;

            // assign the cube
            //auto newAssMat = AssignMatrixByEspresso(currentNode._assignedAssMat, cubeDecomposition);
            vector<AssMat> newAssMatVec;
			vec.assign(totalCubeVector.begin(),totalCubeVector.end());
			bool sto_flag =1;	
            if (currentNode._level == 0)
            {	
				 
					mig_network mig;
					mig = stochastic_synthesis( num_vars, m, n, vec );
					default_simulator<kitty::dynamic_truth_table> sim( m+n );
					const auto tt = simulate<kitty::dynamic_truth_table>( mig, sim )[0];
					kitty::print_binary(tt, std::cout); 
					std::cout<<std::endl;
					string stringtt = to_binary(tt);
					//process_truthtalbe(stringtt );
					newAssMatVec.push_back(process_truthtalbe(currentNode._assignedAssMat,stringtt ));
					
				if(sto_flag == 0)
				{
                newAssMatVec.push_back(AssignMatrixByEspresso(currentNode._assignedAssMat, cubeDecomposition));//only choose the first AssignMatrix that meets the cube vector
				}
            }
            else
            {
                newAssMatVec = AssignMatrixByEspressoVector(currentNode._assignedAssMat, cubeDecomposition);
            }
			
            //auto newAssMatVec = AssignMatrixByEspressoVector(currentNode._assignedAssMat, cubeDecomposition);
			
            // calculate the hash for the new matrix
            for (auto& newAssMat : newAssMatVec)
            {
                cout << "----------------------AssMat----------------------" << endl;

                auto result = ofstream("result.txt", ofstream::app);
                result << "----------------------AssMat----------------------" << endl;
                result.close();

                for (auto it = newAssMat.begin(); it != newAssMat.end(); it++) {
                    cout << *it << " " << endl;

                    auto result = ofstream("result.txt", ofstream::app);
                    result << *it << " " << endl;
                    result.close();

                }
                size_t matSeed = 0;
                hash<vector<string>> hasher;
                matSeed = hasher(newAssMat);
                if (_existingMatrices[matSeed] == true)
                {
                    // if the matrix exists purne it
                    //continue;
                    newAssMat[0][0] = 'x';
                    continue;
                }
                _existingMatrices[matSeed] = true;
            }
            // delete the identity matrix
            for ( int idx =  int(newAssMatVec.size()) - 1; idx >= 0; --idx)
            {
                if (newAssMatVec[idx][0][0] == 'x')
                {
                    newAssMatVec.erase(newAssMatVec.begin() + (newAssMatVec.size() - 1));
                }
            }

            // if it's unassignable, go to next cube decomposition
            //if (newAssMat[0][0] == 'x')
            //{
            //    continue;
            //}
            if (newAssMatVec.empty())
            {
                continue;
            }

            // evaluate the assigned matrix
            for (auto newAssMat : newAssMatVec)
            {
                // create a temporary .pla file
                ofstream ofs("temp.pla");
                ofs << ".i " << _log2LengthOfTotalCube + _accuracy << endl;
                ofs << ".o 1" << endl;
                for (auto line = 0; line <  int(newAssMat.size()); ++line)
                {
                    for (auto col = 0; col <  int(newAssMat[line].size()); ++col)
                    {
                        if (newAssMat[line][col] == '0') continue;
                        ofs << IntToBin(line, _accuracy - 1) << IntToBin(col, _log2LengthOfTotalCube - 1) << " 1" << endl;
                    }
                }
                ofs << ".e" << endl;

                // invoke the espresso through MVSIS
                system("./mvsis -c \"read_pla temp.pla; espresso; print_stats -s;\" > tempres.txt");

                cout << " invoke the espresso through MVSIS" << endl;
                auto result = ofstream("result.txt", ofstream::app);
                result << " invoke the espresso through MVSIS" << endl;
                result.close();


                auto ifs = ifstream("tempres.txt");

                string tempString;
                auto literalCount = 0;
                while (ifs >> tempString)
                {
                    if (tempString == "lits")
                    {
                        ifs >> tempString >> literalCount;
                        break;
                    }
                }
                cout << "literalCount:  " << literalCount << endl;
                auto re = ofstream("result.txt", ofstream::app);
                re << "literalCount:  " << literalCount << endl;
                re.close();
                // prune if the current level's literal count exceeds
                if ((_minLiteralCountOfLevel[currentNode._level] != 0) && (_minLiteralCountOfLevel[currentNode._level] * MULTIPLE <= literalCount))
                {
                    // display the pruning message
                    // continue;
                }

                // update the current level's literal count
                if ((_minLiteralCountOfLevel[currentNode._level] > literalCount) || (_minLiteralCountOfLevel[currentNode._level] == 0))
                {
                    _minLiteralCountOfLevel[currentNode._level] = literalCount;
                }

                // prune if the literal count exceed the overall minimum literal count
                if (literalCount > _minLiteralCount) //modified
                {
                    // display the pruning message
                    continue;
                }

                // sub node
                auto remainingProblemVector = SubtractCube(currentNode._remainingProblemVector, totalCubeVector);//
                //auto newNode = Node(newAssMat, remainingProblemVector, currentNode._level + 1, newAssignedCubeDecompositions, cubeDecomposition, literalCount);
                auto newNode = Node(newAssMat, remainingProblemVector, currentNode._level + 1, newAssignedCubeDecompositionsVec, cubeDecomposition, literalCount);

                // update the max level
                if (_maxLevel <= currentNode._level)
                {
                    _maxLevel = currentNode._level;
                }

                // the current level's cubes' size must be 0 (un-initialized) or <= 2^log2MintermCount
                assert((_sizeOfCubeInLevel[currentNode._level] == 0) || (_sizeOfCubeInLevel[currentNode._level] <=  int(pow(2, log2MintermCount))));

                // if it is less than 2^log2MintermCount strictly: reset the following levels
                if (_sizeOfCubeInLevel[currentNode._level] <  int(pow(2, log2MintermCount)))
                {
                    _sizeOfCubeInLevel[currentNode._level] =  int(pow(2, log2MintermCount));
                    for (auto i = currentNode._level + 1; i <= _maxLevel; ++i)
                    {
                        _sizeOfCubeInLevel[i] = 0;
                    }
                }

                found = true;

                // if this is leaf node, update the solution
                if (IsZeroMintermVector(remainingProblemVector))
                {
                    // update the solution
                    _minLiteralCount = literalCount;
                    ////cout << "_minLiteralCount"<< _minLiteralCount <<endl;
                    _optimalNode = newNode;
                    _optimalNodes.push_back(newNode);

                    // display the update message TODO

                    ++_updateTime;

                    // reset the return value
                    // since it is the leaf node
                    // any node in this level will not
                    // be inserted into return value
                    ret = vector<Node>();

                    // continue to skip insertion
                    continue;
                }

                // insert the node to the vector
                ret.push_back(newNode);
            } // end of for (auto newAssMat : newAssMatVec)
        } // end of for (auto cubeDecomposition : possibleCubeDecompositionVector)

        --log2MintermCount;
        if ((_sizeOfCubeInLevel[currentNode._level] != 0) && (_sizeOfCubeInLevel[currentNode._level] >  int(pow(2, log2MintermCount)))) break;
    } // end of while (!found && (log2MintermCount >= 0))

    return ret;
}

vector<Node> SolutionTree::ProcessNodeVector(vector<Node> nodeVecToBeProcessed)
{

    cout << "-------------------------------SolutionTree::ProcessNodeVector----------------------------------" << endl;
    auto result = ofstream("result.txt", ofstream::app);
    result << "-------------------------------SolutionTree::ProcessNodeVector----------------------------------" << endl;
    result.close();
    
    auto resultSubNodeVector = vector<Node>{};

    // first process each node in the vector and get the optimal nodes
    // use for loop to traverse all of the nodes

    for (auto traversedNode : nodeVecToBeProcessed)
    {
        // we need a helper method to process the node
        // INPUT: Node
        // OUTPUT: a vector of sub nodes that have the same literal count and size
        auto tempNodeVector = ProcessNode(traversedNode);

        // insert the nodes into the vector
        resultSubNodeVector.insert(resultSubNodeVector.end(), tempNodeVector.begin(), tempNodeVector.end());
    }

    // if the level is the leaf _level, then return
    //if (resultSubNodeVector.empty()) return resultSubNodeVector;
    bool resultSubNodeVectorEmpty = false;
    if (resultSubNodeVector.empty())
    {
        resultSubNodeVector = _optimalNodes;
        resultSubNodeVectorEmpty = true;
    }

    auto countSize = [](vector< int> v)
    {
        auto s = 0;
        for (auto i : v)
        {
            s += i;
        }
        return s;
    };

    // for each sub node we need to sort them by the size of extracted cube and the literal count
    sort(resultSubNodeVector.begin(), resultSubNodeVector.end(), [countSize](Node n1, Node n2)
    {
        //// if the literal count not equal
        //if (n1._literalCountSoFar < n2._literalCountSoFar) return true;
        //if (n1._literalCountSoFar > n2._literalCountSoFar) return false;

        //auto v1 = n1._lastAssignedCubeDecomposition;
        //auto v2 = n2._lastAssignedCubeDecomposition;

        //int size1 = countSize(multiply(int(pow(2, v1.first)), multiply(v1.second)));
        //int size2 = countSize(multiply(int(pow(2, v2.first)), multiply(v2.second)));

        //return (size1 > size2);
            cout << "for each sub node we need to sort them by the size of extracted cube and the literal count" << endl;
            auto result = ofstream("result.txt", ofstream::app);
            result << "for each sub node we need to sort them by the size of extracted cube and the literal count" << endl;
            result.close();


        auto v1 = n1._lastAssignedCubeDecomposition;
        auto v2 = n2._lastAssignedCubeDecomposition;
         int size1 = countSize(multiply( int(pow(2, v1.first)), multiply(v1.second)));
         int size2 = countSize(multiply( int(pow(2, v2.first)), multiply(v2.second)));
        if (size1 > size2) return true;
        if (size1 < size2) return false;
        return (n1._literalCountSoFar < n2._literalCountSoFar);
    });

     int smallestLiteralCount = resultSubNodeVector[0]._literalCountSoFar;
     int smallestCubeSize = countSize(multiply( int(pow(2, resultSubNodeVector[0]._lastAssignedCubeDecomposition.first)), multiply(resultSubNodeVector[0]._lastAssignedCubeDecomposition.second)));

    auto delIt = resultSubNodeVector.end();

    for (auto it = resultSubNodeVector.begin(); it != resultSubNodeVector.end(); ++it)
    {
         int size = countSize(multiply( int(pow(2, (it->_lastAssignedCubeDecomposition).first)), multiply((it->_lastAssignedCubeDecomposition).second)));
        //if ((it->_literalCountSoFar != smallestLiteralCount) || (size != smallestCubeSize))
         int literal_limit_parameter = LITERAL_LIMIT_PARAM_w; //the controlled parameter
        if ((it->_literalCountSoFar > smallestLiteralCount + literal_limit_parameter) || (size != smallestCubeSize))
        {
            delIt = it;
            break;
        }
    }

    // delete all nodes greater than the smallest ones
    resultSubNodeVector.erase(delIt, resultSubNodeVector.end());

    // the limit of x-combinations
	
	/*
    auto x_comb = 7;//the controlled parameter
    if (resultSubNodeVector.size() > x_comb)
    {
        resultSubNodeVector.erase(resultSubNodeVector.begin() + x_comb, resultSubNodeVector.end());
    }
	*/

    //return resultSubNodeVector;
	
    if (resultSubNodeVectorEmpty)
    {
        _optimalNodes = resultSubNodeVector;
        resultSubNodeVector = vector<Node>();
    }
	
    return resultSubNodeVector;
}

AssMat SolutionTree::AssignMatrixByEspresso(AssMat originalAssMat, CubeDecomposition cubeDecompositionToBeAssigned) const
{
	 
    auto lineNeeded =  int(pow(2, cubeDecompositionToBeAssigned.first));

    // input example 2^1 [1,1,0] [1,1]
    // we could use std::set to impliment assignment vector
    // or we should call it assignment column set
    // first, for this example, we need to get
    // the assignment set for [1,1,0]
    // which is {00,01} and {00,10}
    // and the assignment set for [1,1]
    // which is {0,1}

    // vector<MintermVector> vectors
    // for each MintermVector in the cubeDecompositionToBeAssigned
    // we find its assignment set
    
    auto assignmentSetsVector = vector<vector<set<string>>>{};

    // find all assignment sets for each cube vector
    for (auto mintermVec : cubeDecompositionToBeAssigned.second)
    {
        // for cube minterm vector [1, 1, 0], we find the assignment sets as following
        // {00, 01} and {00, 10}.
        // vector<set<int>> assignmentSets
        auto assignmentSets = FindAssignmentSetsOfStringForMintermVector(mintermVec);
        assignmentSetsVector.push_back(assignmentSets);//vector<vector<set>>
    }

    // it contains all of the possible assignment sets
    // for this cube decomposition in string
    auto assignmentSetsOfStringForCubeDecomposition = FindAssignmentSetsOfStringForCubeDecomposition(assignmentSetsVector);

    // transform the string version to integer version
    auto assignmentSetsOfIntForCubeDecomposition = vector<set< int>>{};
    for (auto setOfString : assignmentSetsOfStringForCubeDecomposition)
    {
        auto setOfInt = set< int>{};
        for (auto str : setOfString)
        {
            setOfInt.insert(BinToInt(str));
        }
        assignmentSetsOfIntForCubeDecomposition.push_back(setOfInt);
    }

    // one of the assignment set, like {100, 101, 110, 111}
    auto assignmentSetIt = assignmentSetsOfIntForCubeDecomposition.begin();

    while (assignmentSetIt != assignmentSetsOfIntForCubeDecomposition.end())
    {
        // find all available lines that can put the current assignment in
        // for example, find all lines that the columns 100, 101, 110, 111 
        // are still 0's for the assignment set {100, 101, 110, 111}
        auto availableLines = vector< int>{};
        for (auto lineNo = 0; lineNo <  int(originalAssMat.size()); ++lineNo)
        {
            auto available = true;
            auto currentLine = originalAssMat[lineNo];
			//每一个assignment Set中的每一个元素即代表一列；
            for (auto col : *assignmentSetIt)
            {
                if (currentLine[col] == '1')
                {
                    available = false;
                    break;
                }
            }

            if (available)
            {
                availableLines.push_back(lineNo);
            }
        }

        // check wheather the line number is enough
        if ( int(availableLines.size()) < lineNeeded)
        {
            ++assignmentSetIt;
            if (assignmentSetIt == assignmentSetsOfIntForCubeDecomposition.end())
            {
                originalAssMat[0][0] = 'x';
                return originalAssMat;
            }
            continue;
        }

        // write line numbers to file
        auto ofs = ofstream("acc.pla");
        ofs << ".i " << _accuracy << endl << ".o 1" << endl;
        for (auto ix : availableLines)
        {
            ofs << IntToBin(ix, _accuracy - 1) << " 1" << endl;
        }
        ofs << ".e" << endl;

        system("./mvsis -c \"read_pla acc.pla; espresso; print;\" > acc.txt");

        auto ofs_acc = ofstream("acc.txt", ofstream::app);
        ofs_acc << " end_of_file" << endl;
        ofs_acc.close();

        auto ifs_acc = ifstream("acc.txt");
        auto tempStr = string();
        ifs_acc >> tempStr >> tempStr; // "{z0}" and ":"
        auto minLiteralCount = INT_MAX;
        auto bestCube = vector<string>{}; // the form is like [x0 x3' x4]

        auto endOfFile = false;
        while (!endOfFile)
        {
            auto tempBestCube = vector<string>{};
            while (ifs_acc >> tempStr)
            {
                if (tempStr == "Constant")
                {
                    // all lines are there
                    minLiteralCount = 0;
                    tempStr = "end_of_file";
                }
                if (tempStr == "+") break;
                if (tempStr == "end_of_file")
                {
                    endOfFile = true;
                    break;
                }
                tempStr.erase(tempStr.begin());
                tempBestCube.push_back(tempStr);
            }
            if ( int(tempBestCube.size()) < minLiteralCount)
            {
                minLiteralCount =  int(tempBestCube.size());
                bestCube = tempBestCube;
            }
        }

        // all column-cubes are smaller than the line we need
        auto availLineNo =  int(pow(2, _accuracy - minLiteralCount));
        if (availLineNo < lineNeeded)
        {
            ++assignmentSetIt;
            continue;
        }

        // after this scope, there must have an available assignment

        auto pattern = string(_accuracy, 'z'); // z0z
        for (auto litNoInBestCube = 0; litNoInBestCube <  int(bestCube.size()); ++litNoInBestCube)
        {
            auto currentCubeStr = bestCube[litNoInBestCube]; // x3'
            auto ss = stringstream();
            auto N =  int();
            if (currentCubeStr.back() == '\'') // like x3' it is complemented form
            {
                // erase the complemented mark
                currentCubeStr.erase(currentCubeStr.begin() + (currentCubeStr.size() - 1));
                ss << currentCubeStr;
                ss >> N;
                pattern[N] = '0'; // because it is complemented form
            }
            else
            {
                ss << currentCubeStr;
                ss >> N;
                pattern[N] = '1'; // it is un-complemented form
            }
        }
        auto restAcc = _accuracy -  int(bestCube.size());
        auto powRestAcc =  int(pow(2, restAcc));
        auto lineNumberOccupiedByCube = (powRestAcc < lineNeeded) ? powRestAcc : lineNeeded;
        auto assignLines = vector< int>{};
        for (auto ii = 0; ii < lineNumberOccupiedByCube; ++ii)
        {
            auto tempString = IntToBin(ii, restAcc - 1);
            auto tempStringIndex = 0;
            auto tempPattern = pattern; // x0x -> 100
            for (auto jj = 0; jj <  int(tempPattern.size()); ++jj)
            {
                if (tempPattern[jj] == 'z')
                {
                    tempPattern[jj] = tempString[tempStringIndex++];
                }
            }
            assignLines.push_back(BinToInt(tempPattern));
        }

        for (auto assignLineNo : assignLines)
        {
            for (auto i : *assignmentSetIt)
            {
                assert(originalAssMat[assignLineNo][i] == '0');
                originalAssMat[assignLineNo][i] = '1';
            }
        }
		break;

    }//while (assignmentSetIt != assignmentSetsOfIntForCubeDecomposition.end())

    return originalAssMat;
}

vector<AssMat> SolutionTree::AssignMatrixByEspressoVector(AssMat originalAssMatConst, CubeDecomposition cubeDecompositionToBeAssigned) const
{
    auto lineNeeded =  int(pow(2, cubeDecompositionToBeAssigned.first));

    // input example 2^1 [1,1,0] [1,1]
    // we could use std::set to impliment assignment vector
    // or we should call it assignment column set
    // first, for this example, we need to get
    // the assignment set for [1,1,0]
    // which is {00,01} and {00,10}
    // and the assignment set for [1,1]
    // which is {0,1}

    // vector<MintermVector> vectors
    // for each MintermVector in the cubeDecompositionToBeAssigned
    // we find its assignment set

    auto assignmentSetsVector = vector<vector<set<string>>>{};

    // find all assignment sets for each cube vector
    for (auto mintermVec : cubeDecompositionToBeAssigned.second)
    {
        // for cube minterm vector [1, 1, 0], we find the assignment sets as following
        // {00, 01} and {00, 10}.
        // vector<set<int>> assignmentSets
        auto assignmentSets = FindAssignmentSetsOfStringForMintermVector(mintermVec);
        assignmentSetsVector.push_back(assignmentSets);
    }

    // it contains all of the possible assignment sets
    // for this cube decomposition in string
    auto assignmentSetsOfStringForCubeDecomposition = FindAssignmentSetsOfStringForCubeDecomposition(assignmentSetsVector);

    // transform the string version to integer version
    auto assignmentSetsOfIntForCubeDecomposition = vector<set< int>>{};
    for (auto setOfString : assignmentSetsOfStringForCubeDecomposition)
    {
        auto setOfInt = set< int>{};
        for (auto str : setOfString)
        {
            setOfInt.insert(BinToInt(str));
        }
        assignmentSetsOfIntForCubeDecomposition.push_back(setOfInt);
    }

    auto ret = vector<AssMat>();

    // one of the assignment set, like {100, 101, 110, 111}
    auto assignmentSetIt = assignmentSetsOfIntForCubeDecomposition.begin();

	 int valid_count = 0;
	 int x_comb_limit = X_COMB_PARAM_h;
    while (assignmentSetIt != assignmentSetsOfIntForCubeDecomposition.end())
    {
        auto newAssMat = originalAssMatConst;
        // find all available lines that can put the current assignment in
        // for example, find all lines that the columns 100, 101, 110, 111 
        // are still 0's for the assignment set {100, 101, 110, 111}
        auto availableLines = vector< int>{};
        for (auto lineNo = 0; lineNo <  int(newAssMat.size()); ++lineNo)
        {
            auto available = true;
            auto currentLine = newAssMat[lineNo];
            for (auto col : *assignmentSetIt)
            {
                if (currentLine[col] == '1')
                {
                    available = false;
                    break;
                }
            }

            if (available)
            {
                availableLines.push_back(lineNo);
            }
        }

        // check wheather the line number is enough
        if ( int(availableLines.size()) < lineNeeded)
        {
            ++assignmentSetIt;
            //if (assignmentSetIt == assignmentSetsOfIntForCubeDecomposition.end())
            //{
            //    originalAssMat[0][0] = 'x';
            //    return originalAssMat;
            //}
            continue;
        }

        // write line numbers to file
        auto ofs = ofstream("acc.pla");
        ofs << ".i " << _accuracy << endl << ".o 1" << endl;
        for (auto ix : availableLines)
        {
            ofs << IntToBin(ix, _accuracy - 1) << " 1" << endl;
        }
        ofs << ".e" << endl;

        system("./mvsis -c \"read_pla acc.pla; espresso; print;\" > acc.txt");

        auto ofs_acc = ofstream("acc.txt", ofstream::app);
        ofs_acc << " end_of_file" << endl;
        ofs_acc.close();

        auto ifs_acc = ifstream("acc.txt");
        auto tempStr = string();
        ifs_acc >> tempStr >> tempStr; // "{z0}" and ":"
        auto minLiteralCount = INT_MAX;
        auto bestCube = vector<string>{}; // the form is like [x0 x3' x4]

        auto endOfFile = false;
        while (!endOfFile)
        {
            auto tempBestCube = vector<string>{};
            while (ifs_acc >> tempStr)
            {
                if (tempStr == "Constant")
                {
                    // all lines are there
                    minLiteralCount = 0;
                    tempStr = "end_of_file";
                }
                if (tempStr == "+") break;
                if (tempStr == "end_of_file")
                {
                    endOfFile = true;
                    break;
                }
                tempStr.erase(tempStr.begin());
                tempBestCube.push_back(tempStr);
            }
            if ( int(tempBestCube.size()) < minLiteralCount)
            {
                minLiteralCount =  int(tempBestCube.size());
                bestCube = tempBestCube;
            }
        }

        // all column-cubes are smaller than the line we need
        auto availLineNo =  int(pow(2, _accuracy - minLiteralCount));
        if (availLineNo < lineNeeded)
        {
            ++assignmentSetIt;
            continue;
        }

        // after this scope, there must have an available assignment

        auto pattern = string(_accuracy, 'z'); // z0z
        for (auto litNoInBestCube = 0; litNoInBestCube <  int(bestCube.size()); ++litNoInBestCube)
        {
            auto currentCubeStr = bestCube[litNoInBestCube]; // x3'
            auto ss = stringstream();
            auto N =  int();
            if (currentCubeStr.back() == '\'') // like x3' it is complemented form
            {
                // erase the complemented mark
                currentCubeStr.erase(currentCubeStr.begin() + (currentCubeStr.size() - 1));
                ss << currentCubeStr;
                ss >> N;
                pattern[N] = '0'; // because it is complemented form
            }
            else
            {
                ss << currentCubeStr;
                ss >> N;
                pattern[N] = '1'; // it is un-complemented form
            }
        }
        auto restAcc = _accuracy -  int(bestCube.size());
        auto powRestAcc =  int(pow(2, restAcc));
        auto lineNumberOccupiedByCube = (powRestAcc < lineNeeded) ? powRestAcc : lineNeeded;
        auto assignLines = vector< int>{};
        for (auto ii = 0; ii < lineNumberOccupiedByCube; ++ii)
        {
            auto tempString = IntToBin(ii, restAcc - 1);
            auto tempStringIndex = 0;
            auto tempPattern = pattern; // x0x -> 100
            for (auto jj = 0; jj <  int(tempPattern.size()); ++jj)
            {
                if (tempPattern[jj] == 'z')
                {
                    tempPattern[jj] = tempString[tempStringIndex++];
                }
            }
            assignLines.push_back(BinToInt(tempPattern));
        }

        for (auto assignLineNo : assignLines)
        {
            for (auto i : *assignmentSetIt)
            {
                assert(newAssMat[assignLineNo][i] == '0');
                newAssMat[assignLineNo][i] = '1';
            }
        }

        /*break;*/
        ret.push_back(newAssMat);
        ++assignmentSetIt;
		valid_count++;
		
		if (valid_count == x_comb_limit) {
			break;
		}
    }

    //return originalAssMat;
    return ret;
}

vector<set<string>> SolutionTree::FindAssignmentSetsOfStringForMintermVector(MintermVector lineCubeVector) const
{
    cout << "--------SolutionTree::FindAssignmentSetsOfStringForMintermVector---------" << endl;
    auto result = ofstream("result.txt", ofstream::app);
    result << "--------SolutionTree::FindAssignmentSetsOfStringForMintermVector---------" << endl;
    result.close();
    auto countOfOne = 0;

    // find the first non-zero term in lineCubeVector
    // because the number of zeros in the beginning
    // is the number of un-complemented X-variables
    for (; countOfOne <  int(lineCubeVector.size()); ++countOfOne)
    {
        if (lineCubeVector[countOfOne] == 0) continue;
        break;
    }

    auto countOfMinterm = 0;

    // find the count of minterms
    for (auto i = countOfOne; i <  int(lineCubeVector.size()); ++i)
    {
        countOfMinterm += lineCubeVector[i];
    }

    // the number of zeros in the end is the 
    // number of complemented X-variables
    // it's d - t - r, d = size - 1, t = countOfOne, r = log2(countOfMinterm)
    // since countOfMinterm = choose(r, 0) + choose(r, 1) + ... + choose(r, r)
    auto countOfZero =  int(lineCubeVector.size()) - 1 - countOfOne -  int(log2(countOfMinterm));

    // then we have all of the information we need

    // next we need to find a basic assignment set according to the 
    // count of minterms. For example, [1, 1, 0] has two minterms,
    // the basic assignment set will be {0, 1}, then insert 0/1
    // according to the countOfOne and countOfZero we have got.
    auto basicAssignmentSet = BuildBasicAssignmentSet(countOfMinterm);

    // then we will find all of the assignment sets
    auto assignmentSetsOfString = BuildAssignmentSet(basicAssignmentSet, countOfZero, countOfOne);
    return assignmentSetsOfString;
}

set<string> SolutionTree::BuildBasicAssignmentSet( int mintermCount) const
{
    auto ret = set<string>{};

    // if the cube has only one minterm, like [0, 1, 0]
    // its basic assignment set is defined as {""}
    if (mintermCount == 1)
    {
        ret.insert("");
        return ret;
    }

    auto log2MintermCount =  int(log2(mintermCount));
    auto grayCode = ConstructGrayCode(log2MintermCount);

    for (auto i : grayCode)
    {
        ret.insert(IntToBin(i, log2MintermCount - 1));
    }

    return ret;
}

vector<set<string>> SolutionTree::FindAssignmentSetsOfStringForCubeDecomposition(vector<vector<set<string>>> assignmentSetsVector) const
{
    return FindAssignmentSetsOfStringForCubeDecompositionHelper(assignmentSetsVector, vector<set<string>>{set<string>{""}});
}

vector<set<string>> SolutionTree::FindAssignmentSetsOfStringForCubeDecompositionHelper(vector<vector<set<string>>> remainingAssignmentSetsVector, vector<set<string>> currentAssignmentSets) const
//i.e., remainingAssignmentSetsVector=[[{000,010,001,011},{000,010,100,110}],[{00,01},{10,00}],[{1,0}]]
{
    assert(!remainingAssignmentSetsVector.empty());

    auto setVec = vector<set<string>>{};
    for (auto set1 : currentAssignmentSets)
    {
        for (auto set2 : remainingAssignmentSetsVector[0])
        {
            setVec.push_back(MultiplyAssignmentSets(set1, set2));
        }
    }

    if (remainingAssignmentSetsVector.size() == 1)
    {
        return setVec;
    }

    // remainingAssignmentSetsVector.size() >= 2
    remainingAssignmentSetsVector.erase(remainingAssignmentSetsVector.begin());
    return FindAssignmentSetsOfStringForCubeDecompositionHelper(remainingAssignmentSetsVector, setVec);
}

MintermVector multiply( int line, MintermVector cubeVec)
{
    for (auto &i : cubeVec)
    {
        i *= line;
    }
    return cubeVec;
}

MintermVector multiply(MintermVector cubeVec1, MintermVector cubeVec2)
{
    MintermVector product;
    for (auto u : cubeVec2)
    {
        for (auto v : cubeVec1)
        {
            product.push_back(v * u);
        }
    }
    return product;
}

MintermVector multiply(vector<MintermVector> cubeVecs)
{
    assert(!cubeVecs.empty());
    if (cubeVecs.size() == 1)
    {
        return cubeVecs[0];
    }

    auto cv = multiply(cubeVecs[0], cubeVecs[1]);

    if (cubeVecs.size() == 2)
    {
        return cv;
    }

    // size >= 3
    cubeVecs.erase(cubeVecs.begin());
    cubeVecs[0] = cv;

    return multiply(cubeVecs);
}

string IntToBin( int num,  int highestDegree)//ret.insert(IntToBin(i, log2MintermCount - 1));
{
    assert(num <  int(pow(2, highestDegree + 1)));

    string ret;

    while (num)
    {
        auto digit = char((num & 1) + '0');
        ret.insert(ret.begin(), 1, digit);

        num >>= 1;
    }

    ret.insert(ret.begin(), highestDegree + 1 - ret.size(), '0');

    return ret;
}

 int BinToInt(string str)
{
    auto ret = 0;
    for (auto ch : str)
    {
        ret <<= 1;
        ret |=  int(ch - '0');
    }
    return ret;
}

bool CapacityConstraintSatisfied(vector< int> problemVector, MintermVector cubeVector)
{
    assert(problemVector.size() == cubeVector.size());

    for (auto i = 0; i <  int(problemVector.size()); ++i)
    {
        if (problemVector[i] < cubeVector[i]) return false;
    }
    return true;
}

MintermVector SubtractCube(MintermVector problemVector, MintermVector cube)
{
    assert(CapacityConstraintSatisfied(problemVector, cube));
    for (auto i = 0; i <  int(problemVector.size()); ++i)
    {
        problemVector[i] -= cube[i];
    }
    return problemVector;
}

bool IsZeroMintermVector(MintermVector vec)
{
    for (auto i : vec)
    {
        if (i != 0) return false;
    }
    return true;
}

long long  int choose( int n,  int k)
{
    if ((n < k) || (k < 0)) return 0;

    long long  int ret = 1;

    for (auto i = 1; i <= k; ++i)
    {
        ret *= n--;
        ret /= i;
    }

    return ret;
}

vector< int> ConstructGrayCode( int size)
{
    auto grayCode = vector< int>{};
    grayCode.push_back(0);
    grayCode.push_back(1);

    for (auto i = 1; i < size; ++i)
    {
        grayCode = ConstructGrayCodeHelper(grayCode);
    }

    return grayCode;
}

vector< int> ConstructGrayCodeHelper(vector< int> grayCode)
{
    auto ret = vector< int>{};
    for (auto i = 0; i <  int(grayCode.size()); ++i)
    {
        if (i % 2 == 0)
        {
            ret.push_back(grayCode[i] << 1);
            ret.push_back((grayCode[i] << 1) | 1);
        }
        else
        {
            ret.push_back((grayCode[i] << 1) | 1);
            ret.push_back(grayCode[i] << 1);
        }
    }
    return ret;
}

vector<string> BuildZeroOneTwoPermutation( int countOfZero,  int countOfOne,  int countOfTwo)
//BuildZeroOneTwoPermutation(countOfZero, int(basicAssignmentSet.begin()->size()), countOfOne);
{
    auto vic = string(countOfZero, '0') + string(countOfOne, '1') + string(countOfTwo, '2');
    auto ret = vector<string>();

    do
    {
        ret.push_back(vic);
    } while (next_permutation(vic.begin(), vic.end()));

    return ret;
}

set<string> MultiplyAssignmentSets(set<string> set1, set<string> set2)
{
    auto ret = set<string>{};
    for (auto str1 : set1)
    {
        for (auto str2 : set2)
        {
            ret.insert(str1 + str2);
        }
    }
    return ret;
}

set<string> MultiplyAssignmentSets(vector<set<string>> sets)
{
    assert(!sets.empty());
    if (sets.size() == 1)
    {
        return sets[0];
    }

    auto prod = MultiplyAssignmentSets(sets[0], sets[1]);
    if (sets.size() == 2)
    {
        return prod;
    }

    // if the size >= 3
    sets.erase(sets.begin());
    sets[0] = prod;

    return MultiplyAssignmentSets(sets);      
}

vector<CubeDecomposition> PossibleCubeDecompositions( int log2CubeSize, vector< int> degrees,  int accuracy)
{
    return PossibleCubeDecompositionsHelper(log2CubeSize, vector<CubeDecomposition>{make_pair(0, vector<MintermVector>())}, degrees, accuracy);
}

vector<CubeDecomposition> PossibleCubeDecompositionsHelper( int remainingLog2CubeSize, vector<CubeDecomposition> partialDecompositions, vector< int> remainingDegrees,  int accuracy)
{
    auto ret = vector<CubeDecomposition>{};
    // here remainingDegrees keeps d1, d2, ..., d(k-1)
    if (remainingDegrees.size() == 1)
    {
        // if it is the last variable, use loop to find all decompositions
        // l + s_1 = N'
        for (auto s = 0; s <= *(remainingDegrees.rbegin()); ++s)
        {
            auto l = remainingLog2CubeSize - s;
            if (l > accuracy || l < 0)
            {
                // the line number exceeds the hight of the matrix
                continue;
            }
            // if l satisfies the constraint, then find all possible vectors
            // for s_1. We should notice that there are 2^s_1 minterms and the vector
            // is a line cube vector.
            auto possibleCubes = PossibleLineCubeVectors(s, *remainingDegrees.rbegin());
            for (auto cube : possibleCubes)
            {
                for (auto cubeDecomp : partialDecompositions)
                {
                    auto newCubeDecomp = cubeDecomp;
                    // assume newCubeDecomp = (0, ([1, 0])), here first = 0 means unset.
                    // assign the log2 line number l, assume it's 2
                    newCubeDecomp.first = l;
                    // then newCubeDecomp = (2, ([1, 0]))
                    // assume cube = [1, 1, 0],
                    newCubeDecomp.second.insert(newCubeDecomp.second.begin(), cube);
                    // now newCubeDecomp = (2, ([1, 1, 0], [1, 0]))
                    // it means 2^2 X [1, 1, 0] X [1, 0] = [4, 4, 0, 0, 0, 0]
                    ret.push_back(newCubeDecomp);
                }
            }//ret: 8*[1,2,1,0,0]; 8*[0,1,2,1,0]; 8*[0,0,1,2,1] 
        }//for (auto s = 0; s <= *(remainingDegrees.rbegin()); ++s)
        return ret;
    }//if (remainingDegrees.size() == 1)

    // if the remainingDegrees has more than one term
    // i.e., d1, d2, ..., dk
    // first get the partial decompositions
    for (auto s = 0; s <= *(remainingDegrees.rbegin()); ++s)
    {
        auto partialDecompositionsToPass = vector<CubeDecomposition>{};
        // if this is the first level of recursion, we assume partialDecompositions.second
        // is empty, i.e., it is (0, ())
        auto possibleCubes = PossibleLineCubeVectors(s, *remainingDegrees.rbegin());

        for (auto cube : possibleCubes)
        {
            for (auto cubeDecomp : partialDecompositions)
            {
                auto newCubeDecomp = cubeDecomp;
                newCubeDecomp.second.insert(newCubeDecomp.second.begin(), cube);
                partialDecompositionsToPass.push_back(newCubeDecomp);
            }
        }

        auto remainingDegreesToPass = remainingDegrees;
        remainingDegreesToPass.pop_back();

        auto results = PossibleCubeDecompositionsHelper(remainingLog2CubeSize - s, partialDecompositionsToPass, remainingDegreesToPass, accuracy);
        ret.insert(ret.end(), results.begin(), results.end());
    }

    return ret;
}

vector<MintermVector> PossibleLineCubeVectors(int log2CubeSize, int degree)//give the definite r
{
    auto ret = vector<MintermVector>{};

    for (auto zeroBefore = 0; zeroBefore <= degree - log2CubeSize; ++zeroBefore)
    {
        MintermVector line;

        for (auto i = 0; i < zeroBefore; ++i)
        {
            line.push_back(0);
        }

        for (auto i = 0; i <= log2CubeSize; ++i)
        {
            auto mintermCountAtPositionI = choose(log2CubeSize, i);
            line.push_back(int(mintermCountAtPositionI));
        }

        for (auto i = 0; i < degree - log2CubeSize - zeroBefore; ++i)
        {
            line.push_back(0);
        }

        ret.push_back(line);
    }
    return ret;
}

vector<set<string>> BuildAssignmentSet(set<string> basicAssignmentSet, int countOfZero, int countOfOne)
{
    auto ret = vector<set<string>>{};
    auto zeroOneTwoPermutation = BuildZeroOneTwoPermutation(countOfZero, int(basicAssignmentSet.begin()->size()), countOfOne);

    for (auto pattern : zeroOneTwoPermutation)
    {
        auto tempAssignmentSet = set<string>{};

        for (auto str : basicAssignmentSet)
        {
            auto pos = 0;
            auto res = string();

            for (auto i = 0; i < int(pattern.size()); ++i)
            {
                if (pattern[i] == '0')
                {
                    res += "0";
                }
                else if (pattern[i] == '2')
                {
                    res += "1";
                }
                else
                {
                    res += str[pos++];
                }
            }
            tempAssignmentSet.insert(res);
        }
        ret.push_back(tempAssignmentSet);
    }
    return ret;
}

AssMat process_truthtalbe(AssMat originalAssMat,string stringtt )
{
	
	/*for(int i=0;i<stt.size()/2;i++)
	{
		auto tmp=stt[i];
		stt[i]=stt[stt.size()-1-i];
		stt[stt.size()-1-i]=tmp;
	}*/
	/*for(int i=0;i<stt.size();i++)
	{
		str[i] =stt[stt.size()-1-i];
	}*/
	auto retret=vector<string>();	
	string stt,X,Y,tmp;
	int m=2,n=2;
	stt.assign(stringtt);
	reverse(stt.begin(),stt.end());
	
	std::cout<<"stringtt:  "<<stt<<std::endl;
	std::cout<<"stt:  "<<stt<<std::endl;
	for(int i=0;i<stt.size();i++)
	{
		if(stt[i]=='1')
		{
		tmp = IntToBin(i,3);
		reverse(tmp.begin(),tmp.end());
		X=tmp.substr(0,m);
		Y=tmp.substr(m,n);
		auto Xp=BinToInt(X);
		auto Yp=BinToInt(Y);
		originalAssMat[Xp][Yp]='1';
			 
			 cout << tmp << endl;
			 cout << X << endl;
			 cout << Y << endl;
			 retret.push_back(tmp);		
			
		}
	}
	return originalAssMat;
	
}

//Node::Node(AssMat newAssMat, MintermVector newProblemVector, int newLevel, unordered_multiset<CubeDecomposition> newAssignedCubeDecompositions, CubeDecomposition lastAssignedCubeDecomposition, int literalCount)
//{
//    _assignedAssMat = newAssMat;
//    _remainingProblemVector = newProblemVector;
//    _level = newLevel;
//    _assignedCubeDecompositions = newAssignedCubeDecompositions;
//    _lastAssignedCubeDecomposition = lastAssignedCubeDecomposition;
//    _literalCountSoFar = literalCount;
//}

Node::Node(AssMat newAssMat, MintermVector newProblemVector,  int newLevel, vector<CubeDecomposition> newAssignedCubeDecompositions, CubeDecomposition lastAssignedCubeDecomposition,  int literalCount)
{
    _assignedAssMat = newAssMat;
    _remainingProblemVector = newProblemVector;
    _level = newLevel;
    _assignedCubeDecompositionsVec = newAssignedCubeDecompositions;
    _lastAssignedCubeDecomposition = lastAssignedCubeDecomposition;
    _literalCountSoFar = literalCount;
}
