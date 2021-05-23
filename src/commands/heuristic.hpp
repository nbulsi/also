

#ifndef heuristic_HPP
#define heuristic_HPP

#include <mockturtle/mockturtle.hpp>

#include <sstream>
#include "../core/node.h"
//#include "./sto.hpp"
#include "../core/exact_sto_m3ig.hpp"
#include <chrono>
//#include "../core/aig2xmg.hpp"

namespace alice
{
  class heuristic_command : public command
  {
    public:
      explicit heuristic_command( const environment::ptr& env ) : command( env, "" )
      {
      
      }
    
    protected:
      void execute()
      {
       // std::cout<<"just test"<<std::endl;


	using namespace std;

// int main(int int argc, char* argv[])
// int main( )

    // first line: number of variables, n and accuracy m
    // second line: degrees
    // following lines: one problem vector per line

    int variableNumber;
    int accuracy;
    int n;
    vector<int> degrees;
    int temp;

    ifstream ifs("vector.txt");
    auto line = string();
    auto caseCount = 0;
    getline(ifs, line); // first line
    stringstream ss;
    ss << line;
    ss >> variableNumber ;
    ss.str(string());
    ss.clear();
    getline(ifs, line); // second line  
    ss << line;
    ss >> accuracy;
    ss.str(string());
    ss.clear();
    getline(ifs, line); // third line
    ss << line;
    while (ss >> temp)
    {
        degrees.push_back(temp);
    }
    //n=degrees[0];

   assert(degrees.size() == variableNumber);
    
    cout<<endl;
    cout << "Variable number: " << variableNumber << endl;
    cout << "Accuracy: " << accuracy << endl;
    cout << "Degrees: ";
    for (auto d : degrees)
    {
        cout << d << " ";
    }
    cout << endl << endl;

    while (getline(ifs, line))//following lines
    {
        //cout << "Case No. " << caseCount << endl;
       // cout << "======" << endl;

        ss.str(string());
        ss.clear();
        ss << line;
       std::vector<int> problemVector;
       std::vector<unsigned> vector ;
        while (ss >> temp)
        {
            problemVector.push_back(temp);
            //vector.push_back(temp);
        }
        
        cout << "problemVector:  " ;
        for(int i=0;i<problemVector.size();i++)
        {
		std::cout<<	problemVector[i]<<"  ";
		}
       cout << endl;
        /*
        mig_network mig;
        mig = stochastic_synthesis( variableNumber, accuracy, n, vector );
		default_simulator<kitty::dynamic_truth_table> sim( accuracy+n );
		const auto tt = simulate<kitty::dynamic_truth_table>( mig, sim )[0];
		kitty::print_binary(tt, std::cout); 
		*/
        auto solutionTree = SolutionTree(problemVector, degrees, accuracy, caseCount,variableNumber );
        auto start = chrono::system_clock::now();
        solutionTree.ProcessTree();
        auto end = chrono::system_clock::now();
        auto duration = chrono::duration_cast<std::chrono::milliseconds>(end - start);
        auto milliseconds = duration.count();
        cout << "milliseconds: " << milliseconds << endl;
        auto hours = milliseconds / 3600000;
        milliseconds -= hours * 3600000;
        auto minitues = milliseconds / 60000;
        milliseconds -= minitues * 60000;
        auto seconds = milliseconds / 1000;
        milliseconds -= seconds * 1000;
        cout << "Time used: ";
        cout << hours << ":";
        cout.fill('0');
        cout.width(2);
        cout << minitues << ":";
        cout.fill('0');
        cout.width(2);
        cout << seconds << ".";
        cout.fill('0');
        cout.width(3);
        cout << milliseconds << endl;
        cout << endl;
        ++caseCount;
        
      }
      
		//pause();
      	//system("pause");
    //
  }
  };

  ALICE_ADD_COMMAND( heuristic, "Various" )   //name of command, type of command
}

#endif
