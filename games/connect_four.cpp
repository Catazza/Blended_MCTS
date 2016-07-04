// Petter Strandmark 2013
// petter.strandmark@gmail.com

#include <iostream>
using namespace std;

#include <mcts.h>

#include "connect_four.h"

void main_program()
{
  using namespace std;

  bool human_player = false;
  int games_won_P1 = 0;
  int games_won_P2 = 0;
  int games_drawn = 0;
  int games_to_play = 100;

  MCTS::ComputeOptions player1_options, player2_options;
  player1_options.max_iterations = 100000;
  player1_options.verbose = false;   //to be changed back to true eventually
  player2_options.max_iterations =  10000;
  player2_options.verbose = false;//to be changed back to true eventually

  
  for (int i=0; i<games_to_play; i++){
    
    //    cout << "game " << i << endl;

    ConnectFourState state;
    while (state.has_moves()) {
      //cout << endl << "State: " << state << endl;

      ConnectFourState::Move move = ConnectFourState::no_move;
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
	  move = MCTS::compute_move_capped(state, player2_options);
	  state.do_move(move);
	}
      }
    }

    //cout << endl << "Final state: " << state << endl;

    if (state.get_result(2) == 1.0) {
      games_won_P1++;
      //cout << "Player 1 wins!" << endl;
    }
    else if (state.get_result(1) == 1.0) {
      games_won_P2++;
      //cout << "Player 2 wins!" << endl;
    }
    else {
      games_drawn++;
      //cout << "Nobody wins!" << endl;
    }
    
    /* runtime tracker */
    cerr <<".";
    if (!(i%5))
      cerr <<i<<endl;
  }

  cout << "Player 1 won: " << games_won_P1 << " games."<< endl;
  cout << "Player 2 won: " << games_won_P2 << " games."<< endl;
  cout << "Drawn games: " << games_drawn << " games."<< endl;
  
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
