// Petter Strandmark 2013
// petter.strandmark@gmail.com

#include <iostream>
#include <Eigen/Dense>
using namespace std;
using namespace Eigen;

int max_level = 2;

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
  vector<vector<ConnectFourState::Move>> TS_sight_array; 
  vector<ConnectFourState::Move> moves_chosen;
  RowVectorXd lambda_evidence(MAX_SIGHT);
  RowVectorXd prior(MAX_SIGHT);

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

  cout << "the link matrix is: " << endl;
  cout << link_matrix << endl;

  prior << 0.2,0.2,0.2,0.2,0.2;
  cout << "Prior is: " << endl;
  cout << prior << endl;

  cout << "The product of the two is: " << prior*link_matrix;


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

  //cout << "the product of the two is: " << prior*link_matrix << endl;
  //cout << "enter a key to continue: ";
  /* trials to learn matrix library */
    
  //cin >> a_key; // toggle on-off


  MCTS::ComputeOptions player1_options, player2_options;
  player1_options.max_iterations = 100000;
  player1_options.verbose = false;   //to be changed back to true eventually
  player2_options.max_iterations =  100000;
  player2_options.verbose = false;   //to be changed back to true eventually

  
  for (int i=0; i<games_to_play; i++){
    
    ConnectFourState state;
    while (state.has_moves()) {
      /* toggle on-off */
      //cout << endl << "State: " << state << endl;   

      ConnectFourState::Move move = ConnectFourState::no_move;
      if (state.player_to_move == 1) {
	/* uncheck if capped !! */
	vector<ConnectFourState::Move> sight_array = MCTS::sight_array(state, MAX_SIGHT, player1_options);
	TS_sight_array.push_back(sight_array);
	move = MCTS::compute_move_capped(state, player1_options);
	state.do_move(move);
	/* UNCHECK IF NON CAPPED */
	moves_chosen.push_back(move);


	/* toggle on-off */
	//cout << "Chosen Move is: " << move << endl;
	//cout << "Sight array is: " << sight_array  << endl;
	/* toggle on-off */



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

    /* Signal game end to data-containing arrays */

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
    /* Signal game end to data-containing arrays */
    

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

  /* SAVE THE RELEVANT DATA */
  /* print sight array */ 
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
  /* part to print sight array */
  
  
  
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
  
  

  /* save moves_chosen */
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
  
  


  
  cout << "Sight level is: " << max_level << endl;
  cout << "Player 1 is CAPPED." << endl;
  cout << "Player 1 won: " << games_won_P1 << " games."<< endl;
  cout << "Player 2 won: " << games_won_P2 << " games."<< endl;
  cout << "Drawn games: " << games_drawn << " games."<< endl;

  filename = "Sight_";
  filename += (char)(max_level + '0');
  filename += "/structure.txt";
  out5.open(filename, std::fstream::app);
  out5 << "Sight level is: " << max_level << endl << endl;
  out5 << "Player 1 is CAPPED." << endl;
  out5 << "Total games is: " << games_to_play << endl;
  out5 << "Player 1 won: " << games_won_P1 << " games."<< endl;
  out5 << "Player 2 won: " << games_won_P2 << " games."<< endl;
  out5 << "Drawn games: " << games_drawn << " games."<< endl;
  out5.close();
  
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
