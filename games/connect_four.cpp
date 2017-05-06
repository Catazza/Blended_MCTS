// Petter Strandmark 2013
// petter.strandmark@gmail.com

// Cataldo Azzariti 2016
// cataldo.azzariti@gmail.com


#include <iostream>
#include <Eigen/Dense>
#include <unistd.h>
using namespace std;
using namespace Eigen;

// sight_level of the opponent algorithm
int max_level = 2;
// flag for saving moves
bool save_move = false;

#include <mcts.h>


#include "connect_four.h"


void main_program()
{
  using namespace std;

  bool human_player = false;   // toggle true-false for human player
  int games_won_P1 = 0;
  int games_won_P2 = 0;
  int games_drawn = 0;
  int moves_per_player = 0;
  int games_to_play = 3;    // choose as desired
  const int MAX_SIGHT = 5;
  vector<vector<ConnectFourState::Move>> TS_sight_array; 
  vector<ConnectFourState::Move> moves_chosen;
  RowVectorXd lambda_evidence(MAX_SIGHT);
  RowVectorXd prior(MAX_SIGHT);
  vector<double> updated_post;

  /* TOGGLE A-KEY ON-OFF */
  char a_key; // to make algos wait    // toggle on-off
  MatrixXd link_matrix(MAX_SIGHT, MAX_SIGHT);
  vector<vector<double>> TS_belief_sight;
  vector<double> game_break;
  for (int i=0; i<MAX_SIGHT; i++){
    game_break.push_back(-9999);
  }
  vector<ConnectFourState::Move> game_break_SA;
  for (int i=0; i<MAX_SIGHT; i++){
    game_break_SA.push_back(-9999);
  }
  string filename = "";  // to allow file savings  



  /* Initialize prior and link matrix */
  link_matrix << 0.6,0.15,0.05,0.05,0.05,
                 0.2,0.6,0.15,0.05,0.05,
                 0.1,0.15,0.6,0.15,0.1,
                 0.05,0.05,0.15,0.6,0.2,
                 0.05,0.05,0.05,0.15,0.6;

  prior << 0.2,0.2,0.2,0.2,0.2;


  ofstream out5;
  filename = "Sight_";
  filename += (char)(max_level + '0');
  filename += "/structure.txt";
  out5.open(filename);
  out5 << "the link matrix is: " << endl;
  out5 << link_matrix << endl << endl;
  out5 << "Prior is: " << endl;
  out5 << prior << endl << endl;
  out5.close();


  // Initialize algorithms parameters
  MCTS::ComputeOptions player1_options, player2_options;
  player1_options.max_iterations = 100000;
  player1_options.verbose = false;   //to be changed back to true eventually
  player2_options.max_iterations =  100000;   //put back to 100k!!!***
  player2_options.verbose = false;   //to be changed back to true eventually

  

  // Outer loop for each game
  for (int i=0; i<games_to_play; i++){
    
    ConnectFourState state;
    moves_per_player = 0;
    while (state.has_moves()) {

      /* toggle on-off to suppress output to console */
      cout << endl << "State: " << state << endl;   

      ConnectFourState::Move move = ConnectFourState::no_move;
      if (state.player_to_move == 1) {
	
	/* We assume the opponent move first */
	vector<ConnectFourState::Move> sight_array = 
	  MCTS::sight_array(state, MAX_SIGHT, player1_options);
	TS_sight_array.push_back(sight_array);
	move = MCTS::compute_move_capped(state, player1_options);

	/* save move for hit-rate analysis */
	if (save_move) {
	  filename = "Sight_";
	  filename += (char)(max_level + '0');
	  filename += "/moves_inferred.txt";
	  out5.open(filename, std::fstream::app);	  
	  out5 << " " << move << endl;
	  out5.close();
	}

	state.do_move(move);
	moves_per_player++;
	moves_chosen.push_back(move);

	/* part for the live demo */
	cout << "Prior before the move is: [" << prior << "]" << endl;
	cout << "Sight Array is: [";
	for (int i = 0; i < MAX_SIGHT-1; i++){
	  cout << sight_array[i] << " ";
	}
	cout << sight_array[MAX_SIGHT - 1] << "]" << endl;
	cout << "The move chosen by the opponent is: " << move << endl;

	/* Probabilistic Update */
	prior = MCTS::update_prior(move, sight_array, prior, MAX_SIGHT, 
				   link_matrix);

	/* part for live demo */
	cout << "The updated posterior is: [" << prior << "]" <<  endl;	
	cout << "Enter a char to continue: ";
	cin >> a_key;


	// store time series in a matrix
	updated_post.clear();
	for (int i = 0; i < MAX_SIGHT; i++){
	  updated_post.push_back(prior(i));
	}
	TS_belief_sight.push_back(updated_post);
      }
      
      /* Part for human player, if present. Not used for DSEM but left it
	 anyways */
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
	    moves_per_player++;
	  }
	}

	/* Unconstrained or Adaptative algo part. We assume it moves second. */
	else {
	  
	  /* TOGGLE ON FOR ADAPTATIVE !!!!!!!!!!!! */
	  move = MCTS::compute_adaptative_move_UCT(state, MAX_SIGHT, 
						   updated_post,player2_options);


	  /* Old piece of algo, which used traditional MCTS, replaced by 
	     adaptative */
	  //move = MCTS::compute_move(state, player2_options);
	  /* old piece of algo replaced by adaptative */

	  state.do_move(move);
	  moves_per_player++;

	}
      }
      
      /* Toggle on-off, to check-point algorithms to analyse 
	 intermediate trees.  */
      //cout << "Press a key to continue: ";
      //cin >> a_key;
    }

    /* toggle on-off - to control output to console*/
    cout << endl << "Final state: " << state << endl;



    /* Part to signal game end to data-containing arrays */
    TS_belief_sight.push_back(game_break);
    TS_sight_array.push_back(game_break_SA);
    moves_chosen.push_back(-9999);

    // Lambda evidence
    ofstream out1;
    filename = "Sight_";
    filename += (char)(max_level + '0');
    filename += "/lambda_evidence.txt";
    out1.open(filename, std::fstream::app);
    for (unsigned int i = 0; i < MAX_SIGHT; i++){
      out1 << -9999 << " " ;
    }
    out1 << endl;
    out1.close();

    // Move inferred
    filename = "Sight_";
    filename += (char)(max_level + '0');
    filename += "/moves_inferred.txt";
    out1.open(filename, std::fstream::app);
    out1 << -9999 << endl;
    //    out1 << " " << endl;
    out1.close();



    // % win
    ofstream out7;
    filename = "Sight_";
    filename += (char)(max_level + '0');
    filename += "/TS_%_win.txt";
    out7.open(filename, std::fstream::app);
    out7 << -9999;
    out7 << endl;
    out7.close();

    // % visits
    filename = "Sight_";
    filename += (char)(max_level + '0');
    filename += "/TS_%_visits.txt";
    out7.open(filename, std::fstream::app);
    out7 << -9999;
    out7 << endl;
    out7.close();

    // Moves per player
    filename = "Sight_";
    filename += (char)(max_level + '0');
    filename += "/moves_per_player.txt";
    out7.open(filename, std::fstream::app);
    if (state.get_result(2) == 1.0) {
      out7 << "W ";
    }
    else
      out7 << "L ";
    out7 << moves_per_player;
    out7 << endl;
    out7.close();
    /* End of part to signal game end to data-containing arrays */
    

    /* Assign victory to the players. */
    if (state.get_result(2) == 1.0) {
      games_won_P1++;
      /* toggle on-off - to control output to console */
      //cout << "Player 1 wins!" << endl;
    }
    else if (state.get_result(1) == 1.0) {
      games_won_P2++;
      /* toggle on-off - to control output to console */
      //cout << "Player 2 wins!" << endl;
    }
    else {
      games_drawn++;
      /* toggle on-off - to control output to console */
      //cout << "Nobody wins!" << endl;
    }
    
    /* runtime tracker */
    cerr <<".";
    if (!(i%5))
      cerr <<i<<endl;
  }

  /* SAVE THE RELEVANT DATA */
  /* Save sight array */ 
  ofstream out;
  filename = "Sight_";
  filename += (char)(max_level + '0');
  filename += "/TS_sight_array.txt";
  out.open(filename);
  for (unsigned int i = 0; i < TS_sight_array.size(); i++){
    for (int j = 0; j < MAX_SIGHT; j++){
      out << TS_sight_array[i][j];
    }
    out << endl;
  }
  out.close();
  /* part to save sight array */
  
  
  
  /* Save TS belief sight */
  ofstream out2;
  filename = "Sight_";
  filename += (char)(max_level + '0');
  filename += "/TS_belief_sight.txt";
  out2.open(filename);
  for (unsigned int i = 0; i < TS_belief_sight.size(); i++){
    for (int j = 0; j < MAX_SIGHT; j++){
      out2 << setprecision(2) << fixed << TS_belief_sight[i][j];
      out2 << "  ";
    }
    out2 << endl;
  }
  out2.close();
  /* Save TS belief sight */
  
  

  /* Save moves_chosen */
  ofstream out1;
  filename = "Sight_";
  filename += (char)(max_level + '0');
  filename += "/moves_chosen.txt";
  out1.open(filename);
  for (unsigned int i = 0; i < moves_chosen.size(); i++){
    out1 << moves_chosen[i] << endl;
  }
  out1.close();
  /* save moves_chosen */
  
  


  /* Final Output. */
  cout << "Sight level is: " << max_level << endl;
  cout << "Player 1 is CAPPED." << endl;
  cout << "Using ADAPTATIVE algo." << endl;
  cout << "Player 1 won: " << games_won_P1 << " games."<< endl;
  cout << "Player 2 won: " << games_won_P2 << " games."<< endl;
  cout << "Drawn games: " << games_drawn << " games."<< endl;

  /* Save final output to file. */
  filename = "Sight_";
  filename += (char)(max_level + '0');
  filename += "/structure.txt";
  out5.open(filename, std::fstream::app);
  out5 << "Sight level is: " << max_level << endl << endl;
  out5 << "Player 1 is CAPPED." << endl;
  out5 << "Using ADAPTATIVE algo." << endl;
  out5 << "Total games is: " << games_to_play << endl;
  out5 << "Player 1 won: " << games_won_P1 << " games."<< endl;
  out5 << "Player 2 won: " << games_won_P2 << " games."<< endl;
  out5 << "Drawn games: " << games_drawn << " games."<< endl;
  out5.close();
  
}



/* Main program. */
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
/* END OF MAIN PROGRAM */
