// Petter Strandmark 2013
// petter.strandmark@gmail.com

#include <iostream>
#include <Eigen/Dense>
using namespace std;
using namespace Eigen;

int max_level = 3;

#include <mcts.h>


#include "connect_four.h"




void main_program()
{
  using namespace std;

  /* toggle true-false */
  bool human_player = false;   //toggle on-off
  int games_won_P1 = 0;
  int games_won_P2 = 0;
  int games_drawn = 0;
  int games_to_play = 100;    //toggle back to 1
  const int MAX_SIGHT = 5;
  //ConnectFourState::Move sight_array[MAX_SIGHT];
  //vector<ConnectFourState::Move> sight_array;
  vector<vector<ConnectFourState::Move>> TS_sight_array; 
  vector<ConnectFourState::Move> moves_chosen;
  RowVectorXd lambda_evidence(MAX_SIGHT);
  RowVectorXd prior(MAX_SIGHT);

  char a_key; // to make algos wait    // toggle on-off
  MatrixXd link_matrix(MAX_SIGHT, MAX_SIGHT);
  vector<vector<double>> TS_belief_sight;
  vector<double> game_break;
  game_break.push_back(-9999);
  game_break.push_back(-9999);
  game_break.push_back(-9999);
  game_break.push_back(-9999);
  game_break.push_back(-9999);



  /* Initialize prior and link matrix */
  link_matrix << 0.6,0.15,0.05,0.05,0.05,
                 0.2,0.6,0.15,0.05,0.05,
                 0.1,0.15,0.6,0.15,0.1,
                 0.05,0.05,0.15,0.6,0.2,
                 0.05,0.05,0.05,0.15,0.6;
  cout << "the link matrix is: " << endl;
  cout << link_matrix << endl;

  prior << 0.2,0.2,0.2,0.2,0.2;
  cout << "Prior is: " << endl;
  cout << prior << endl;

  cout << "the product of the two is: " << prior*link_matrix << endl;
  //cout << "enter a key to continue: ";
  /* trials to learn matrix library */
    
  //cin >> a_key; // toggle on-off


  MCTS::ComputeOptions player1_options, player2_options;
  player1_options.max_iterations = 100000;
  player1_options.verbose = false;   //to be changed back to true eventually
  player2_options.max_iterations =  10000;
  player2_options.verbose = false;//to be changed back to true eventually

  
  for (int i=0; i<games_to_play; i++){
    
    ConnectFourState state;
    while (state.has_moves()) {
      /* toggle on-off */
      //cout << endl << "State: " << state << endl;   

      ConnectFourState::Move move = ConnectFourState::no_move;
      if (state.player_to_move == 1) {
	/* uncheck if capped !! */
	vector<ConnectFourState::Move> sight_array = MCTS::sight_array(state, MAX_SIGHT, player1_options);
	//cout << endl << "State: " << state << endl;   // CHECK NOT ALTER STATE
	TS_sight_array.push_back(sight_array);
	move = MCTS::compute_move_capped(state, player1_options);
	state.do_move(move);
	/* UNCHECK IF NON CAPPED */
	moves_chosen.push_back(move);

	/* UNCHECK IF NON CAPPED */
	prior = MCTS::update_prior(move, sight_array, prior, MAX_SIGHT, link_matrix);
	// store time series in a matrix
	vector<double> updated_post;
	for (int i = 0; i < MAX_SIGHT; i++){
	  updated_post.push_back(prior(i));
	}
	TS_belief_sight.push_back(updated_post);
      }
      else {
	if (human_player) {
	  //vector<ConnectFourState::Move> sight_array = MCTS::sight_array(state, MAX_SIGHT, player1_options);
	  //TS_sight_array.push_back(sight_array);
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
	  // compute the sight vector
	  //vector<ConnectFourState::Move> sight_array = MCTS::sight_array(state, MAX_SIGHT, player1_options);
	  //TS_sight_array.push_back(sight_array);
	  move = MCTS::compute_move(state, player2_options);
	  state.do_move(move);
	  //moves_chosen.push_back(move);
	  
	  /* toggle on-off */
	  /*cout << "Chosen Move is: " << move << endl;
	  cout << "Sight array is: ";
	  for (unsigned int i=0; i<MAX_SIGHT; i++){
	    cout << TS_sight_array[TS_sight_array.size()-1][i];
	  }
	  cout << endl; */
	  /* toggle on-off */
	  
	  
	  // update prior of sight with the move	  
	  //prior = MCTS::update_prior(move, sight_array, prior, MAX_SIGHT, link_matrix);
	  // store time series in a matrix
	  //vector<double> updated_post;
	  //for (int i = 0; i < MAX_SIGHT; i++){
	  //  updated_post.push_back(prior(i));
	  //}
	  //TS_belief_sight.push_back(updated_post);
	}
      }
      
      /* Toggle on-off */
      //cout << "Press a key to continue: ";
      //cin >> a_key;
    }

    /* toggle on-off */
    //cout << endl << "Final state: " << state << endl;
    
    

    /* print sight array */
    ofstream out;
    out.open("TS_sight_array.txt");
    for (unsigned int i = 0; i < TS_sight_array.size(); i++){
      for (int j = 0; j < MAX_SIGHT; j++){
	out << TS_sight_array[i][j];
      }
      out << endl;
    }
    out.close();
    /* part to print sight array */



    /* Save TS belief sight */
    TS_belief_sight.push_back(game_break);
    ofstream out2;
    out2.open("TS_belief_sight.txt");
    for (unsigned int i = 0; i < TS_belief_sight.size(); i++){
      for (int j = 0; j < MAX_SIGHT; j++){
	out2 << setprecision(2) << fixed << TS_belief_sight[i][j];
	out2 << "  ";
      }
      out2 << endl;
    }
    out2.close();
    /* Save TS belief sight */



    /* save moves_chosen */
    ofstream out1;
    out1.open("moves_chosen.txt");
    for (unsigned int i = 0; i < moves_chosen.size(); i++){
      out1 << moves_chosen[i] << endl;
    }
    out1.close();
    /* save moves_chosen */


    if (state.get_result(2) == 1.0) {
      games_won_P1++;
      /* toggle on-off */
      //cout << "Player 1 wins!" << endl;
    }
    else if (state.get_result(1) == 1.0) {
      games_won_P2++;
      /* toggle on-off */
      //cout << "Player 2 wins!" << endl;
    }
    else {
      games_drawn++;
      /* toggle on-off */
      //cout << "Nobody wins!" << endl;
    }
    
    /* runtime tracker */
    cerr <<".";
    if (!(i%5))
      cerr <<i<<endl;
  }
  
  cout << "Sight level is: " << max_level << endl;
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
