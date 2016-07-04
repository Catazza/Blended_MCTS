// Petter Strandmark 2013
// petter.strandmark@gmail.com

#include <iostream>
#include <cstring>
using namespace std;

#include <mcts.h>

#include "connect_four.h"


//void convertBoard(vector<vector<char>> a_board, string& a_string, int& argc);
void convertBoard(vector<vector<char>> a_board, char argv[][4], int& argc);



void main_program()
{
  using namespace std;

  bool human_player = true;

  MCTS::ComputeOptions player1_options, player2_options;
  player1_options.max_iterations = 100000;
  player1_options.verbose = true;
  player2_options.max_iterations =  10000;
  player2_options.verbose = true;

  ConnectFourState state;
  int argc = 0;
  char argv[41][4];      
  for (int i=0; i<41; i++){
    for (int j=0; j<4; j++)
      argv[i][j] = '\0';
  }
  vector<vector<char>> board;
  //string converted_board = "";  

  while (state.has_moves()) {
    cout << endl << "State: " << state << endl;


    ConnectFourState::Move move = ConnectFourState::no_move;

    /* COMMENT OUT WHILE TRY THE MINIMAX
    if (state.player_to_move == 1) {
      move = MCTS::compute_move(state, player1_options);
      state.do_move(move);
    }
    else {
      if (human_player) {
	while (true) {
	  cout << "Input your move: ";
	  move = ConnectFourState::no_move;
	  cin >> move;
	  try {
	    state.do_move(move);
	    break;
	  }
	  catch (std::exception& ) {
	    cout << "Invalid move." << endl;
	  }
	}
      }
      else {
	move = MCTS::compute_move(state, player2_options);
	state.do_move(move);
      }
    }
    */


    /* NEW PART FOR MINIMAX */
    if (state.player_to_move == 1) {      

      // have program calculate the move

      // step 1. convert the board into usable format:
      board = state.getBoard();
      //convertBoard(board, converted_board, argc);
      
      // RESET
      argc = 0;
      for (int i=0; i<41; i++){
	for (int j=0; j<4; j++)
	  argv[i][j] = '\0';
      }

      convertBoard(board, argv, argc);
      //cout << "The converted string is " << converted_board << endl;
      cout << "The converted string is ARGV" << argv << endl;
      cout << "ARGV is :";
      for (int i=0; i<argc; i++){
	cout << argv[i];
      }
      cout << endl;
      cout << "argc is: " << argc << endl;

      // step2. convert the board into argv**
      

      // step3. spit argc and argv** into LoadBoard

      // step4. compute the move


      // to be deleted
      move = MCTS::compute_move(state, player1_options);      
      state.do_move(move);
      
      //check
      argc = 0;
      for (int i=0; i<41; i++){
	for (int j=0; j<4; j++)
	  argv[i][j] = '\0';
      }
      //converted_board = "";
      board = state.getBoard();
      //convertBoard(board, converted_board, argc);
      convertBoard(board, argv, argc);
      //cout << "The converted string is " << converted_board << endl;
      cout << "The converted string is ARGV " << argv << endl;
      cout << "ARGV is :";
      for (int i=0; i<argc; i++){
	cout << argv[i];
      }
      cout << endl;
      cout << "argc is: " << argc << endl;

      //Board try_board = loadBoard(argc, argv);
      
    }
    else {
      if (human_player) {
	while (true) {      
	  cout << "Input your move: ";
	  move = ConnectFourState::no_move;
	  cin >> move;
	  try {
	    state.do_move(move);
	    break;
	  }
	  catch (std::exception& ) {
	    cout << "Invalid move." << endl;
	  }
	}
      }
      else {
	move = MCTS::compute_move(state, player2_options);
	state.do_move(move);
      }
    }
    /* NEW PART FOR MINIMAX */


  }

  cout << endl << "Final state: " << state << endl;

  if (state.get_result(2) == 1.0) {
    cout << "Player 1 wins!" << endl;
  }
  else if (state.get_result(1) == 1.0) {
    cout << "Player 2 wins!" << endl;
  }
  else {
    cout << "Nobody wins!" << endl;
  }
}



int main()
{
  try {
    main_program();
  }
  catch (std::runtime_error& error) {
    std::cerr << "ERROR: " << error.what() << std::endl;
    return 1;
  }
}



/*
void convertBoard(vector<vector<char>> a_board, string& a_string, int& argc){

  for (int row = 0; row < 6; row++){
    for (int col = 0; col < 7; col++){
      if (a_board[row][col] != '.'){
	argc++;
	a_string += " ";
	if (a_board[row][col] == 'X')
	  a_string += "o";
	if (a_board[row][col] == 'O')
	  a_string += "y";
	a_string += (char)(row)+'0';
	a_string += (char)(col)+'0';
      }     
    }
  } 

}
*/


void convertBoard(vector<vector<char>> a_board, char argv[][4], int& argc){

  for (int row = 0; row < 6; row++){
    for (int col = 0; col < 7; col++){
      if (a_board[row][col] != '.'){
	if (a_board[row][col] == 'X')
	  argv[argc][0] = 'o';
	if (a_board[row][col] == 'O')
	  argv[argc][0] = 'y';
	argv[argc][1] = (char)(row)+'0';
	argv[argc][2] = (char)(col)+'0';
	argv[argc][3] = ' ';
	argc++;
      }     
    }
  } 

}

