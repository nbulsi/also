/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file heusto.hpp
 *
 * @brief stochastic circuit synthesis based on heuristic
 *
 * @author Xiang He
 * @since  0.1
 */

#ifndef HEURISTIC_HPP
#define HEURISTIC_HPP

#include <mockturtle/mockturtle.hpp>

#include <sstream>
#include <chrono>

#include "../core/node.hpp"
#include "../core/exact_sto_m3ig.hpp"

using namespace std;

namespace alice
{
	class heusto_command : public command
	{
		public:
			explicit heusto_command( const environment::ptr& env ) : command( env, "stochastic circuit synthesis based on heuristic method" )
		{
			add_option( "filename, -f", filename, "the input txt file name" );
			add_option( "time, -t", time_limit, "the time limit of the SAT solver; run without stochastic_systhesis if time==0" );
			add_flag( "--verbose, -v", "verbose output" );
		}

		protected:
			void execute()
			{
				//test if MVSIS is available
				if( system( "./mvsis -c \"quit\"") != 0)
				{
					std::cerr << "[e] mvsis binary is not available in the current working directory.\n";
					exit( 0 );
				}

				std::ifstream myfile( filename );

				if( myfile.is_open() )
				{
					degrees.clear();
					problemVector.clear();

					int temp;	
					auto line = string();
					getline( myfile , line ); // first line
					stringstream ss;
					ss << line;
					ss >> variableNumber;
					ss.str( string() );
					ss.clear();

					getline( myfile , line ); // second line  
					ss << line;
					ss >> accuracy;
					ss.str( string()) ;
					ss.clear();

					getline( myfile , line ); // third line
					ss << line;
					while ( ss >> temp )
					{
						degrees.push_back(temp);
					}

					assert( degrees.size() == variableNumber );

					cout << "Variable number: " << variableNumber << endl;
					cout << "Accuracy: " << accuracy << endl;
					cout << "Degrees: ";

					for (auto d : degrees)
					{
						cout << d << " ";
					}
					cout << endl;

					while (getline( myfile, line ))//following lines
					{
						ss.str(string());
						ss.clear();
						ss << line;

						while (ss >> temp)
						{
							problemVector.push_back(temp);
						}

						cout << "problemVector:  " ;
						for(int i=0;i<problemVector.size();i++)
						{
							std::cout<< problemVector[i] << "  ";
						}
						cout << endl;

					}
				}

				myfile.close();
				bool verbose = is_set( "verbose" );

				auto solutionTree = SolutionTree( problemVector, degrees, accuracy, variableNumber, time_limit, verbose );
				auto start = chrono::system_clock::now();
				solutionTree.ProcessTree();
				auto end = chrono::system_clock::now();
				auto duration = chrono::duration_cast<std::chrono::milliseconds>(end - start);
				auto milliseconds = duration.count();

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
			}


		private:
			int variableNumber;
			int accuracy;
			vector<int> degrees;
			vector<int> problemVector;
			unsigned time_limit = 60 * 60; //default is 1 hour
			std::string filename = "vector.txt";
	};

	ALICE_ADD_COMMAND( heusto, "Various" )   //name of command, type of command
}

#endif
