#ifndef MCTS_HEADER_PETTER
#define MCTS_HEADER_PETTER
//
// Petter Strandmark 2013
// petter.strandmark@gmail.com
//
//
// Amended and expanded by Cataldo Azzariti 2016
// cataldo.azzariti@gmail.com
//
//
// Monte Carlo Tree Search for finite games.
//
// Originally based on Python code at
// http://mcts.ai/code/python.html
//
// Uses the "root parallelization" technique [1].
//
// [1] Chaslot, G. M. B., Winands, M. H., & van Den Herik, H. J. (2008).
//     Parallel monte-carlo tree search. In Computers and Games (pp. 
//     60-71). Springer Berlin Heidelberg.
//



// This game engine can play any game defined by a state like this:
/*

  class GameState
  {
  public:
  typedef int Move;
  static const Move no_move = ...

  void do_move(Move move);
  template<typename RandomEngine>
  void do_random_move(*engine);
  bool has_moves() const;
  std::vector<Move> get_moves() const;

  // Returns a value in {0, 0.5, 1}.
  // This should not be an evaluation function, because it will only be
  // called for finished games. Return 0.5 to indicate a draw.
  double get_result(int current_player_to_move) const;

  int player_to_move;

  // ...
  private:
  // ...
  };

*/


//
// See the examples for more details. Given a suitable State, the
// following function (tries to) compute the best move for the
// player to move.
//



namespace MCTS
{

  /* Struct defining parameters to make the MCTS algo run */
  struct ComputeOptions
  {
    int number_of_threads;
    int max_iterations;
    double max_time;
    bool verbose;

  ComputeOptions() :
    number_of_threads(1),  // Leave 1 to start with!!
      max_iterations(100000),
      max_time(-1.0), // default is no time limit.
      verbose(false)
    { }
  };




  /* FUNCTION DECLARATIONS - USED BY MCTS */
  template<typename State>
    typename State::Move compute_move(const State root_state,
				      const ComputeOptions options = 
				      ComputeOptions());
  template<typename State>
    typename State::Move compute_move_capped(const State root_state,
					     const ComputeOptions options =
					     ComputeOptions());

  template<typename State>
    void sight_array(const State root_state, typename State::Move* sight_array,
		     const int& max_sight, const ComputeOptions options = 
		     ComputeOptions());

  RowVectorXd update_prior(const int& observed_move, const vector<int>& 
			   sight_array, const RowVectorXd& prior, const int& 
			   max_sight, const MatrixXd& link_matrix);

  RowVectorXd set_lambda_evidence(const int& observed_move, const vector<int>&
				  sight_array, const int& max_sight);

  RowVectorXd calculate_posterior(const RowVectorXd& prior, const RowVectorXd&
				  lambda_evidence, const int& max_sight, 
				  const MatrixXd& link_matrix);

  template<typename State>
    typename State::Move compute_adaptative_move(const State root_state, const 
						 int& max_sight, vector<double> 
						 sight_belief, const 
						 ComputeOptions options = 
						 ComputeOptions());

  bool is_inferrable(vector<double> sight_belief, int& sight_inferred, 
		     const int& max_sight);

}



#include <algorithm>
#include <cstdlib>
#include <future>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <Eigen/Dense>

#ifdef USE_OPENMP
#include <omp.h>
#endif



namespace MCTS
{
  using std::cerr;
  using std::endl;
  using std::vector;
  using std::size_t;
  using namespace Eigen;
  
  void check(bool expr, const char* message);
  void assertion_failed(const char* expr, const char* file, int line);

  #define attest(expr) if (!(expr)) { ::MCTS::assertion_failed(#expr, __FILE__, __LINE__); }
  #ifndef NDEBUG
  #define dattest(expr) if (!(expr)) { ::MCTS::assertion_failed(#expr, __FILE__, __LINE__); }
  #else
  #define dattest(expr) ((void)0)
  #endif


  
  /* This Node class is used to build the game tree, it is its building block.
     The root is created by the users and
     the rest of the tree is created by add_node. */

  template<typename State>
    class Node
    {
    public:
      typedef typename State::Move Move;

      Node(const State& state);
      ~Node();

      bool has_untried_moves() const;
      template<typename RandomEngine>
	Move get_untried_move(RandomEngine* engine) const;
      Node* best_child() const;

      bool has_children() const
      {
	return ! children.empty();
      }

      Node* select_child_UCT() const;
      template<typename RandomEngine>
	Node* select_child_unif(RandomEngine* engine) const;
      Node* add_child(const Move& move, const State& state);
      void update(double result);

      std::string to_string() const;
      std::string tree_to_string(int max_depth = 1000000, int indent = 0) const;

      const Move move;
      Node* const parent;
      const int player_to_move;
	
      //std::atomic<double> wins;
      //std::atomic<int> visits;
      double wins;
      int visits;
      double score_from_below; 
      int BI_depth;  // added to break ties in Back Induction
      Move move_inferred;
      
      std::vector<Move> moves;
      std::vector<Node*> children;

    private:
      Node(const State& state, const Move& move, Node* parent);

      std::string indent_string(int indent) const;

      Node(const Node&);
      Node& operator = (const Node&);

      double UCT_score;
    };


  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////

  /**************************************************************/
  /* BEGIN OF PART FOR FUNCTION DECLARATIONS FOR THE NODE CLASS */
  /**************************************************************/

								
  /* Constructor */
  template<typename State>
    Node<State>::Node(const State& state) : 
    move(State::no_move),
    parent(nullptr),
    player_to_move(state.player_to_move),
    wins(0),
    visits(0),
    score_from_below(-1),  // CA added
    move_inferred(-1),
    moves(state.get_moves()),
    UCT_score(0)
      { }
  /* END OF FUNCTION DEFINITION */



  /* Overloaded Private Constructor */
  template<typename State>
    Node<State>::Node(const State& state, const Move& move_, Node* parent_) :
    move(move_),
    parent(parent_),
    player_to_move(state.player_to_move),
    wins(0),
    visits(0),
    score_from_below(-1),   // CA added
    BI_depth(-1),           // CA added
    move_inferred(-1),
    moves(state.get_moves()),
    UCT_score(0)
      { }
  /* END OF FUNCTION DEFINITION */



  /* Destructor - recursively delete all children */
  template<typename State>
    Node<State>::~Node()
    {
      for (auto child: children) {
	delete child;
      }
    }
  /* END OF FUNCTION DEFINITION */



  /* Function to check if a state has some moves available */
  template<typename State>
    bool Node<State>::has_untried_moves() const
    {
      return ! moves.empty();
    }
  /* END OF FUNCTION DEFINITION */



  /* Get a random move to execute from a state */  
  template<typename State>
    template<typename RandomEngine>
    typename State::Move Node<State>::get_untried_move(RandomEngine* 
						       engine) const
    {
      attest( ! moves.empty());
      std::uniform_int_distribution<std::size_t> moves_distribution(0,
							   moves.size() - 1);
      return moves[moves_distribution(*engine)];
    }
  /* END OF FUNCTION DEFINITION */



  /* Function to select the best child - based on number of visits */
  template<typename State>
    Node<State>* Node<State>::best_child() const
    {
      attest(moves.empty());
      attest( ! children.empty() );

      return *std::max_element(children.begin(), children.end(),
			       [](Node* a, Node* b) { 
				 return a->visits < b->visits; 
			       });;
    }
  /* END OF FUNCTION DEFINITION */




  /* Function to implement UCT tree-selection policy. */
  template<typename State>
    Node<State>* Node<State>::select_child_UCT() const
    {
      attest( ! children.empty() );
      for (auto child: children) {
	child->UCT_score = double(child->wins) / double(child->visits) +
	  std::sqrt(2.0 * std::log(double(this->visits)) / child->visits);
      }

      return *std::max_element(children.begin(), children.end(),
			       [](Node* a, Node* b) { 
				 return a->UCT_score < b->UCT_score; });
    }
  /* END OF FUNCTION DEFINITION */




  /* NEW FUNCTION - To select a child uniformly as opposed with UCT when 
     descending the tree */
  template<typename State>
    template<typename RandomEngine>
    Node<State>* Node<State>::select_child_unif(RandomEngine* engine) const
    {
      std::uniform_int_distribution<std::size_t> moves_distribution(0, 
						       children.size() - 1);
      return children[moves_distribution(*engine)];
    }
  /* END OF FUNCTION DEFINITION */



  /* Function to add a child to a Node */
  template<typename State>
    Node<State>* Node<State>::add_child(const Move& move, const State& state)
    {
      auto node = new Node(state, move, this);
      children.push_back(node);
      attest( ! children.empty());

      auto itr = moves.begin();
      for (; itr != moves.end() && *itr != move; ++itr);
      attest(itr != moves.end());
      moves.erase(itr);
      return node;
    }
  /* END OF FUNCTION DEFINITION */



  /* Function to backpropagate the result of a random playout */
  template<typename State>
    void Node<State>::update(double result)
    {
      visits++;
      wins += result;
    }
  /* END OF FUNCTION DEFINITION */



  /* Helper function to convert a Node to a printable format, used to print
     the tree */
  template<typename State>
    std::string Node<State>::to_string() const
    {
      std::stringstream sout;
      sout << "["
	   << "P" << 3 - player_to_move << " "
	   << "M:" << move << " "
	   << "W/V: " << wins << "/" << visits << " "
	   << "%_win: " << setprecision(2) << fixed << wins/visits << ", " 
	   << "SFB: " << setprecision(2) << fixed << score_from_below << ", "
	   << "MI: " << move_inferred << ", "
	   << "U: " << moves.size() << "]\n";
      return sout.str();
    }
  /* END OF FUNCTION DEFINITION */



  /* Function to turn a tree to a string to print it */
  template<typename State>
    std::string Node<State>::tree_to_string(int max_depth, int indent) const
    {
      if (indent >= max_depth) {
	return "";
      }

      std::string s = indent_string(indent) + to_string();
      for (auto child: children) {
	s += child->tree_to_string(max_depth, indent + 1);
      }
      return s;
    }
  /* END OF FUNCTION DEFINITION */



  /* Helper function to create indents when printing the tree */
  template<typename State>
    std::string Node<State>::indent_string(int indent) const
    {
      std::string s = "";
      for (int i = 1; i <= indent; ++i) {
	s += "| ";
      }
      return s;
    }
  /* END OF FUNCTION DEFINITION */


  /**************************************************************/
  /* END OF PART FOR FUNCTION DECLARATIONS FOR THE NODE CLASS */
  /**************************************************************/

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////


  /* Function to compute the tree with the MCTS algorithm. 
     Used by compute_move.
     Unconstrained version. */
  template<typename State>
    std::unique_ptr<Node<State>>  compute_tree(const State root_state,
					       const ComputeOptions options,
					       std::mt19937_64::result_type 
					       initial_seed)
    {

      std::random_device rd;
      std::mt19937_64 random_engine(rd());
      // Alternative Fixed Seed if provided as a parameter
      //std::mt19937_64 random_engine(initial_seed);

      attest(options.max_iterations >= 0 || options.max_time >= 0);
      if (options.max_time >= 0) {
      #ifndef USE_OPENMP
	throw std::runtime_error("ComputeOptions::max_time requires OpenMP.");
      #endif
      }

      // Will support more players later.
      attest(root_state.player_to_move == 1 || root_state.player_to_move == 2);
      auto root = std::unique_ptr<Node<State>>(new Node<State>(root_state));

      #ifdef USE_OPENMP
        double start_time = ::omp_get_wtime();
        double print_time = start_time;
      #endif

      /* MCTS cycle - selection, expansion, simulation, backpropagation */
      for (int iter = 1; iter <= options.max_iterations || 
	     options.max_iterations < 0; ++iter) {
	
	auto node = root.get();
	State state = root_state;

	// SELECTION - Select a path through the tree to a leaf node.
	while (!node->has_untried_moves() && node->has_children()) {
	  node = node->select_child_UCT();
	  state.do_move(node->move);
	}

	// EXPANSION - If we are not already at the final state, expand the
	// tree with a new node and move there.
	if (node->has_untried_moves()) {
	  auto move = node->get_untried_move(&random_engine);
	  state.do_move(move);
	  node = node->add_child(move, state);
	}

	// SIMULATION - We now play randomly until the game ends.
	while (state.has_moves()) {
	  state.do_random_move(&random_engine);
	}

	// BACKPROPAGATION - We have now reached a final state. 
	// Backpropagate the result up the tree to the root node.
	while (node != nullptr) {
	  node->update(state.get_result(node->player_to_move));
	  node = node->parent;

	}


        #ifdef USE_OPENMP
	if (options.verbose || options.max_time >= 0) {
	  double time = ::omp_get_wtime();
	  if (options.verbose && (time - print_time >= 1.0 || 
				  iter == options.max_iterations)) {
	    std::cerr << iter << " games played (";
	    std::cerr << double(iter) / (time - start_time) << " / second).";
	    std::cerr << endl;
	    print_time = time;
          }
	     
	  if (time - start_time >= options.max_time) {
	    break;
	  }
        }
        #endif

      } //closes for 100k iter MCTS cycle
	
      
      /* Part to print tree */
      /*std::ofstream out;
      out.open("TreeFull.txt");
      out << root->tree_to_string(3,0);
      out.close();*/
      /* Part to print tree */

      return root;
    }
  /* END OF FUNCTION DEFINITION */



  /* Function to compute the tree with the MCTS algorithm. 
     Used by compute_move_capped.
     Capped version - cap set by max_level. */
  template<typename State>
    std::unique_ptr<Node<State>> compute_tree_capped(const State root_state,
						     const ComputeOptions 
						     options,
						     std::mt19937_64::result_type
						     initial_seed)
    {
      // to keep track how deep we are in the tree
      int level_counter = 0;

      std::random_device rd;
      std::mt19937_64 random_engine(rd());
      //TO BE REINTEGRATED POTENTIALLY
      //std::mt19937_64 random_engine(initial_seed);

      attest(options.max_iterations >= 0 || options.max_time >= 0);
      if (options.max_time >= 0) {
      #ifndef USE_OPENMP
	throw std::runtime_error("ComputeOptions::max_time requires OpenMP.");
      #endif
      }

      // Will support more players later.
      attest(root_state.player_to_move == 1 || root_state.player_to_move == 2);
      auto root = std::unique_ptr<Node<State>>(new Node<State>(root_state));

      #ifdef USE_OPENMP
        double start_time = ::omp_get_wtime();
        double print_time = start_time;
      #endif


      /* MCTS cycle - selection, expansion, simulation, backpropagation */
      for (int iter = 1; iter <= options.max_iterations || 
	     options.max_iterations < 0; ++iter) {
	auto node = root.get();
	State state = root_state;
	level_counter = 0; //restart from root;

	// Select a path through the tree to a leaf node.
	while (!node->has_untried_moves() && node->has_children() && 
	       level_counter < max_level) {
	  node = node->select_child_UCT();
	  state.do_move(node->move);
	  level_counter++;
	}

	// If we are not already at the final state, expand the
	// tree with a new node and move there.
	if (node->has_untried_moves() && level_counter < max_level) {
	  auto move = node->get_untried_move(&random_engine);
	  state.do_move(move);
	  node = node->add_child(move, state);
	  level_counter++;
	}

	// We now play randomly until the game ends.
	while (state.has_moves()) {
	  state.do_random_move(&random_engine);
	}

	// We have now reached a final state. Backpropagate the result
	// up the tree to the root node.
	while (node != nullptr) {
	  node->update(state.get_result(node->player_to_move));
	  node = node->parent;
	}

	
	#ifdef USE_OPENMP
	if (options.verbose || options.max_time >= 0) {
          double time = ::omp_get_wtime();
          if (options.verbose && (time - print_time >= 1.0 || 
				  iter == options.max_iterations)) {
            std::cerr << iter << " games played (";
	    std::cerr << double(iter) / (time - start_time);
	    std::cerr << " / second)." << endl;
	    print_time = time;
          }
	

	  if (time - start_time >= options.max_time) {
            break;
	  }
        }
	#endif
	
      }


      /* Part to print the tree */
      /*std::ofstream out;
      string filename = "Sight_";
      filename += (char)(max_level + '0');
      filename += "/Tree_capped.txt";
      out.open(filename);
      out << root->tree_to_string(8,0);
      out.close();*/
      /* Part to print the tree */


      return root;
    }
  /* END OF FUNCTION DEFINITION */



  /* Function to compute the tree with the MCTS algorithm. 
     Used by compute_move_adapt.
     ADAPTATIVE version - Uses sight inferred to infer moves of opponent */
  template<typename State>
    std::unique_ptr<Node<State>> compute_tree_adapt(const State root_state,
	                                            const ComputeOptions options,
	                                            std::mt19937_64::result_type
	                                            initial_seed,
						    const int sight_inferred,
						    const int max_sight)
    {
      int level_counter = 0;

      std::random_device rd;
      std::mt19937_64 random_engine(rd());
      //TO BE REINTEGRATED POTENTIALLY
      //std::mt19937_64 random_engine(initial_seed);

      attest(options.max_iterations >= 0 || options.max_time >= 0);
      if (options.max_time >= 0) {
      #ifndef USE_OPENMP
	throw std::runtime_error("ComputeOptions::max_time requires OpenMP.");
      #endif
      }

      // Will support more players later.
      attest(root_state.player_to_move == 1 || root_state.player_to_move == 2);
      auto root = std::unique_ptr<Node<State>>(new Node<State>(root_state));

      #ifdef USE_OPENMP
        double start_time = ::omp_get_wtime();
        double print_time = start_time;
      #endif


      /* MCTS cycle - selection, expansion, simulation, backpropagation */
      for (int iter = 1; iter <= options.max_iterations || 
	     options.max_iterations < 0; ++iter) {
	auto node = root.get();
	State state = root_state;
	level_counter = 0; //restart from root;
	
	// Select a path through the tree to a leaf node.
	// This time also taking into account opponent's inferred moves
	// and deleting those children that do not correspond to those moves
	while (!node->has_untried_moves() && node->has_children()) {
	  auto parent_node = node;
	  node = node->select_child_UCT();
	  if (level_counter == 1) {
	    
	    // Infer move and set it for parent if not yet
	    if (parent_node->move_inferred == -1) {
	      vector<typename State::Move> subtree_sight_arr = sight_array(state, 
							     max_sight, options);
	      typename State::Move move_inf = subtree_sight_arr[sight_inferred
								- 1];
	      parent_node->move_inferred = move_inf;
	    }

	    // Delete unwanted children - indirect tree pruning.
	    if (node->move != parent_node->move_inferred){
	      delete node;
	      auto child_iter = parent_node->children.begin();
	      for (;child_iter != parent_node->children.end(); ++ child_iter){
		if (*child_iter == node) {
		  parent_node->children.erase(child_iter);
		  break;
		}
	      }
	      node = root.get();
	      level_counter = 0;
	      state = root_state;
	      continue;
	    }
	  }
	  
	  state.do_move(node->move);
	  level_counter++;
	}


	// If we are not already at the final state, expand the
	// tree with a new node and move there.
	if (node->has_untried_moves()) {
	  auto move = node->get_untried_move(&random_engine);
	  state.do_move(move);
	  node = node->add_child(move, state);
	}

	// We now play randomly until the game ends.
	while (state.has_moves()) {
	  state.do_random_move(&random_engine);
	}

	// We have now reached a final state. Backpropagate the result
	// up the tree to the root node.
	while (node != nullptr) {
	  node->update(state.get_result(node->player_to_move));
	  node = node->parent;
	}

	
	#ifdef USE_OPENMP
	if (options.verbose || options.max_time >= 0) {
	   double time = ::omp_get_wtime();
	   if (options.verbose && (time - print_time >= 1.0 || 
				   iter == options.max_iterations)) {
             std::cerr << iter << " games played (";
	     std::cerr << double(iter) / (time - start_time);
	     std::cerr << " / second)." << endl;
	     print_time = time;
	   }
	

	   if (time - start_time >= options.max_time) {
	     break;
	   }
	}
	#endif
	
      }


      /* Part to print the tree */
      /*std::ofstream out;
      string filename = "Sight_";
      filename += (char)(max_level + '0');
      filename += "/Tree_capped.txt";
      out.open(filename);
      out << root->tree_to_string(8,0);
      out.close();*/
      /* Part to print the tree */


      return root;
    }
  /* END OF FUNCTION DEFINITION */




  /* Function to compute a tree to evaluate the opponent with uniform node 
     selection as opposed to UCT */
  /* Function to compute the tree with the MCTS algorithm. 
     Used by compute_move_unif.
     UNIFORM version - Uses Uniform tree policy when descending the tree */
  template<typename State>
    std::unique_ptr<Node<State>>  compute_tree_unif(const State root_state,
						    const ComputeOptions options,
						    std::mt19937_64::result_type 
						    initial_seed)
    {

      std::random_device rd;
      std::mt19937_64 random_engine(rd());
      //TO BE REINTEGRATED POTENTIALLY
      //std::mt19937_64 random_engine(initial_seed);

      attest(options.max_iterations >= 0 || options.max_time >= 0);
      if (options.max_time >= 0) {
      #ifndef USE_OPENMP
	throw std::runtime_error("ComputeOptions::max_time requires OpenMP.");
      #endif
      }

      // Will support more players later.
      attest(root_state.player_to_move == 1 || root_state.player_to_move == 2);
      auto root = std::unique_ptr<Node<State>>(new Node<State>(root_state));

      #ifdef USE_OPENMP
        double start_time = ::omp_get_wtime();
        double print_time = start_time;
      #endif


      for (int iter = 1; iter <= options.max_iterations || options.max_iterations < 0; ++iter) {
	auto node = root.get();
	State state = root_state;

	// Select a path through the tree to a leaf node.
	while (!node->has_untried_moves() && node->has_children()) {
	  node = node->select_child_unif(&random_engine);
	  state.do_move(node->move);
	}

	// If we are not already at the final state, expand the
	// tree with a new node and move there.
	if (node->has_untried_moves()) {
	  auto move = node->get_untried_move(&random_engine);
	  state.do_move(move);
	  node = node->add_child(move, state);
	}

	// We now play randomly until the game ends.
	while (state.has_moves()) {
	  state.do_random_move(&random_engine);
	}

	// We have now reached a final state. Backpropagate the result
	// up the tree to the root node.
	while (node != nullptr) {
	  node->update(state.get_result(node->player_to_move));
	  node = node->parent;

	}


	   #ifdef USE_OPENMP
	   if (options.verbose || options.max_time >= 0) {
	     double time = ::omp_get_wtime();
	     if (options.verbose && (time - print_time >= 1.0 || 
				     iter == options.max_iterations)) {
	       std::cerr << iter << " games played (";
	       std::cerr << double(iter) / (time - start_time);
	       std::cerr << " / second)." << endl;
	       print_time = time;
	     }
	     
	     if (time - start_time >= options.max_time) {
	       break;
	     }
	   }
           #endif


      } //closes for 100k iter
	
      
      /* Part to print tree */
      /*std::ofstream out;
      out.open("TreeFull.txt");
      out << root->tree_to_string(3,0);
      out.close();*/
      /* Part to print tree */

      return root;
    }
  /* END OF FUNCTION DEFINITION */




  /* Function to compute move the move the algorithm will make
     UNCONSTRAINED version. */
  template<typename State>
    typename State::Move compute_move(const State root_state,
				      const ComputeOptions options)
    {
      using namespace std;

      // Will support more players later.
      attest(root_state.player_to_move == 1 || root_state.player_to_move == 2);

      auto moves = root_state.get_moves();
      attest(moves.size() > 0);
      if (moves.size() == 1) {
	return moves[0];
      }

      #ifdef USE_OPENMP
        double start_time = ::omp_get_wtime();
      #endif

      // Start all jobs to compute trees.
      vector<future<unique_ptr<Node<State>>>> root_futures;
      ComputeOptions job_options = options;
      job_options.verbose = false;
      for (int t = 0; t < options.number_of_threads; ++t) {
	auto func = [t,&root_state,&job_options]()->std::unique_ptr<Node<State>>
	  {
	    return compute_tree(root_state, job_options, 1012411 * t + 12515);
	  };

	root_futures.push_back(std::async(std::launch::async, func));
      }

      // Collect the results.
      vector<unique_ptr<Node<State>>> roots;
      for (int t = 0; t < options.number_of_threads; ++t) {
	roots.push_back(std::move(root_futures[t].get()));
      }

      /* Part to print tree */
      /*std::ofstream out;
      string filename = "Sight_";
      filename += (char)(max_level + '0');
      filename += "/TreeFullCM.txt";
      out.open(filename);
      out << roots[0].get()->tree_to_string(3,0);
      out.close();*/
      /* Part to print tree */

      // Merge the children of all root nodes.
      map<typename State::Move, int> visits;
      map<typename State::Move, double> wins;
      long long games_played = 0;
      for (int t = 0; t < options.number_of_threads; ++t) {
	auto root = roots[t].get();
	games_played += root->visits;
	for (auto child = root->children.cbegin(); 
	     child != root->children.cend(); ++child) {
	  visits[(*child)->move] += (*child)->visits;
	  wins[(*child)->move]   += (*child)->wins;
	}
      }

      // Find the node with the most visits.
      double best_score = -1;
      typename State::Move best_move = typename State::Move();
      for (auto itr: visits) {
	auto move = itr.first;
	double v = itr.second;
	double w = wins[move];
	// Expected success rate assuming a uniform prior (Beta(1, 1)).
	// https://en.wikipedia.org/wiki/Beta_distribution
	double expected_success_rate = (w + 1) / (v + 2);
	if (expected_success_rate > best_score) {
	  best_move = move;
	  best_score = expected_success_rate;
	}
	
	
	if (options.verbose) {
	  cerr << "Move: " << itr.first
	  << " (" << setw(2) << right 
	       << int(100.0 * v / double(games_played) + 0.5) << "% visits)"
	  << " (" << setw(2) << right << int(100.0 * w / v + 0.5)    
	       << "% wins)" << endl;
	  }
      }


      if (options.verbose) {
	auto best_wins = wins[best_move];
	auto best_visits = visits[best_move];
	cerr << "----" << endl;
	cerr << "Best: " << best_move
	     << " (" << 100.0 * best_visits / double(games_played) <<"% visits)"
	     << " (" << 100.0 * best_wins / best_visits << "% wins)" <<endl;
      }


      

      #ifdef USE_OPENMP
      if (options.verbose) {
      double time = ::omp_get_wtime();
      std::cerr << games_played << " games played in " 
		<< double(time - start_time) << " s. " 
		<< "(" << double(games_played) / (time - start_time) 
		<< " / second, "
		<< options.number_of_threads << " parallel jobs)." << endl;
    }
     #endif

      return best_move;
    }
  /* END OF FUNCTION DEFINIION */




  /* Function to compute move the move the algorithm will make
     CAPPED version. */
  template<typename State>
    typename State::Move compute_move_capped(const State root_state,
					     const ComputeOptions options)
    {
      using namespace std;

      // Will support more players later.
      attest(root_state.player_to_move == 1 || root_state.player_to_move == 2);

      auto moves = root_state.get_moves();
      attest(moves.size() > 0);
      if (moves.size() == 1) {
	return moves[0];
      }

      #ifdef USE_OPENMP
      double start_time = ::omp_get_wtime();
      #endif

      // Start all jobs to compute trees.
      vector<future<unique_ptr<Node<State>>>> root_futures;
      ComputeOptions job_options = options;
      job_options.verbose = false;
      for (int t = 0; t < options.number_of_threads; ++t) {
	auto func = [t,&root_state,&job_options]()->std::unique_ptr<Node<State>>
	  {
	    return compute_tree_capped(root_state, job_options, 
				       1012411 * t + 12515);
	  };

	root_futures.push_back(std::async(std::launch::async, func));
      }

      // Collect the results.
      vector<unique_ptr<Node<State>>> roots;
      for (int t = 0; t < options.number_of_threads; ++t) {
	roots.push_back(std::move(root_futures[t].get()));
      }

      // Merge the children of all root nodes.
      map<typename State::Move, int> visits;
      map<typename State::Move, double> wins;
      long long games_played = 0;
      for (int t = 0; t < options.number_of_threads; ++t) {
	auto root = roots[t].get();
	games_played += root->visits;
	for (auto child = root->children.cbegin(); 
	     child != root->children.cend(); ++child) {
	  visits[(*child)->move] += (*child)->visits;
	  wins[(*child)->move]   += (*child)->wins;
	}
      }

      // Find the node with the most visits.
      double best_score = -1;
      typename State::Move best_move = typename State::Move();
      for (auto itr: visits) {
	auto move = itr.first;
	double v = itr.second;
	double w = wins[move];
	// Expected success rate assuming a uniform prior (Beta(1, 1)).
	// https://en.wikipedia.org/wiki/Beta_distribution
	double expected_success_rate = (w + 1) / (v + 2);
	if (expected_success_rate > best_score) {
	  best_move = move;
	  best_score = expected_success_rate;
	}

	if (options.verbose) {
	  cerr << "Move: " << itr.first
	       << " (" << setw(2) << right 
	       << int(100.0 * v / double(games_played) + 0.5) << "% visits)"
	       << " (" << setw(2) << right << int(100.0 * w / v + 0.5)    
	       << "% wins)" << endl;
	}
      }

      // Moved the auto declared variables out of options.verbose to 
      // store anomalies
      auto best_wins = wins[best_move];
      auto best_visits = visits[best_move];
      if (options.verbose) {
	cerr << "----" << endl;
	cerr << "Best: " << best_move
	     << " (" << 100.0 * best_visits / double(games_played) <<"% visits)"
	     << " (" << 100.0 * best_wins / best_visits << "% wins)" << endl;
      }


      /* Part to store time series of % win of best node to analyse anomalies */
      ofstream out_anom;
      string filename = "Sight_";
      filename += (char)(max_level + '0');
      filename += "/TS_%_win.txt";
      out_anom.open(filename, std::fstream::app);
      out_anom << 100.0 * best_wins / best_visits;
      out_anom << endl;
      out_anom.close();      

      filename = "Sight_";
      filename += (char)(max_level + '0');
      filename += "/TS_%_visits.txt";
      out_anom.open(filename, std::fstream::app);
      out_anom << 100.0 * best_visits / double(games_played);
      out_anom << endl;
      out_anom.close();      
      /* END OF Part to store time series of % win of best node to analise 
	 anomalies */




      #ifdef USE_OPENMP
      if (options.verbose) {
      double time = ::omp_get_wtime();
      std::cerr << games_played << " games played in " 
		<< double(time - start_time) << " s. " 
		<< "(" << double(games_played) / (time - start_time) 
		<< " / second, "
		<< options.number_of_threads << " parallel jobs)." << endl;
      }
      #endif

      return best_move;
    }
  /* END OF FUNCTION DEFINITION */




  /* Function to compute move the move the algorithm will make
     ADAPTATIVE version. */
  template<typename State>
    typename State::Move compute_adaptative_move_UCT(const State root_state, 
						    const int& max_sight,
						    vector<double> sight_belief, 
						    const ComputeOptions options
						    = ComputeOptions())
  
    {
      using namespace std;
      int sight_inferred = -1;

      
      // if belief is not strong enough, compute move normally.
      if (!is_inferrable(sight_belief, sight_inferred, max_sight)) {
	save_move = false;
	return compute_move(root_state, options);
      }

      // flag to save moves
      save_move = true;


      /* Otherwise, use the adaptative algorithm */
 
      // Will support more players later.
      attest(root_state.player_to_move == 1 || root_state.player_to_move == 2);

      auto moves = root_state.get_moves();
      attest(moves.size() > 0);
      if (moves.size() == 1) {
	return moves[0];
      }


      #ifdef USE_OPENMP
        double start_time = ::omp_get_wtime();
      #endif

      // Start all jobs to compute trees.
      vector<future<unique_ptr<Node<State>>>> root_futures;
      ComputeOptions job_options = options;
      job_options.verbose = false;
      for (int t = 0; t < options.number_of_threads; ++t) {
	auto func = [t, &root_state, &job_options, &sight_inferred, &max_sight] () 
	  -> std::unique_ptr<Node<State>>
	  {
	    return compute_tree_adapt(root_state, job_options, 
				      1012411 * t + 12515, sight_inferred, 
				      max_sight);
	  };

	root_futures.push_back(std::async(std::launch::async, func));
      }

      // Collect the results.
      vector<unique_ptr<Node<State>>> roots;
      for (int t = 0; t < options.number_of_threads; ++t) {
	roots.push_back(std::move(root_futures[t].get()));
      }

      /* Part to print tree */
      /*std::ofstream out;
      string filename = "Sight_";
      filename += (char)(max_level + '0');
      filename += "/TreeFullAdapt.txt";
      out.open(filename);
      out << roots[0].get()->tree_to_string(4,0);
      out.close(); */
      /* Part to print tree */

      // Merge the children of all root nodes.
      map<typename State::Move, int> visits;
      map<typename State::Move, double> wins;
      long long games_played = 0;
      for (int t = 0; t < options.number_of_threads; ++t) {
	auto root = roots[t].get();
	games_played += root->visits;
	for (auto child = root->children.cbegin(); 
	     child != root->children.cend(); ++child) {
	  visits[(*child)->move] += (*child)->visits;
	  wins[(*child)->move]   += (*child)->wins;
	}
      }

      // Find the node with the most visits.
      double best_score = -1;
      typename State::Move best_move = typename State::Move();
      for (auto itr: visits) {
	auto move = itr.first;
	double v = itr.second;
	double w = wins[move];
	// Expected success rate assuming a uniform prior (Beta(1, 1)).
	// https://en.wikipedia.org/wiki/Beta_distribution
	double expected_success_rate = (w + 1) / (v + 2);
	if (expected_success_rate > best_score) {
	  best_move = move;
	  best_score = expected_success_rate;
	}
	
	
	if (options.verbose) {
	  cerr << "Move: " << itr.first
	  << " (" << setw(2) << right 
	       << int(100.0 * v / double(games_played) + 0.5) << "% visits)"
	  << " (" << setw(2) << right << int(100.0 * w / v + 0.5)    
	       << "% wins)" << endl;
	}
      }


      if (options.verbose) {
	auto best_wins = wins[best_move];
	auto best_visits = visits[best_move];
	cerr << "----" << endl;
	cerr << "Best: " << best_move
	     << " (" << 100.0 * best_visits / double(games_played) 
	     << "% visits)"
	     << " (" << 100.0 * best_wins / best_visits << "% wins)" 
	     << endl;
      }


      

      #ifdef USE_OPENMP
      if (options.verbose) {
      double time = ::omp_get_wtime();
      std::cerr << games_played << " games played in " 
		<< double(time - start_time) << " s. " 
		<< "(" << double(games_played) / (time - start_time) 
		<< " / second, "
		<< options.number_of_threads << " parallel jobs)." 
		<< endl;
      }
      #endif



      /* Part to calculate hit rate */
      typename State:: Move counter_move = -1;
      auto root = roots[0].get();
      for (auto child = root->children.cbegin(); 
	   child != root->children.cend(); ++child) {
        if ((*child)->move == best_move) {
          if ((*child)->children.size() > 0 ){
            counter_move = (*child)->children[0]->move;	  
          }
        break;
        }	
      }      
      
      /* save the predicted counter-move */
      std::ofstream out;
      string filename = "Sight_";
      filename += (char)(max_level + '0');
      filename += "/moves_inferred.txt";
      out.open(filename, std::fstream::app);
      out << counter_move;
      out.close();
      

      return best_move;
    }
  /* END OF FUNCTION DEFINITION */



  /* Function to determine if the belief of one sight is strong enough to apply
   the adaptative algorith, */
  bool is_inferrable(vector<double> sight_belief, int& sight_inferred, 
		     const int& max_sight) {
    
    bool is_inferrable = false;

    for (int sight_level = 1; sight_level <= max_sight; sight_level++){
      if (sight_belief[sight_level - 1] >= 0.98) {
	sight_inferred = sight_level;
	is_inferrable = true;
	break;
      }
    }

    return is_inferrable;
  }
  /* END OF FUNCTION DEFINITION */


  

  /* Function to calculate the sight array */  
  template<typename State>
    vector<typename State::Move> sight_array(const State root_state, const int&
					     max_sight, const ComputeOptions 
					     options){

    // Initialize sight array
    vector<typename State::Move> sight_array;
    sight_array.resize(max_sight, -1);

    // Compute the tree
    ComputeOptions job_options = options;
    job_options.verbose = false;

    // Uses UNIFORM tree policy
    auto root = compute_tree_unif(root_state, job_options, 1943); 
    //does not matter the seed is fixed as it is altered with RD


    /* Part to print tree */
    /*std::ofstream out;
    string filename = "Sight_";
    filename += (char)(max_level + '0');
    filename += "/TreeOppEval.txt";
    out.open(filename);
    out << root->tree_to_string(6,0);
    out.close();*/
    /* Part to print tree */
    
          
    // Compute the sight array
    Node<State>* root_naked = root.get();
    for (int sight_level = 1; sight_level <= max_sight; sight_level++){
      // Uses TIEBREAK rule
      sight_array[sight_level - 1] = backward_induction_tiebreak(root_naked, 
								 sight_level);   
    }
  
    return sight_array;
  }



  /* Function to calculate the backward induction values of a give tree
     PLAIN version. */
  template<typename State>
    typename State::Move backward_induction(Node<State>* root, int depth){
    
    // Print ree to check
    /* Part to print tree */
    /*std::ofstream out;
    string file_name = "Sight_";
    file_name += (char)(max_level + '0');
    file_name += "/TreeBI_";
    file_name += (char)(depth + '0');
    file_name += ".txt";
    out.open(file_name);
    out << root->tree_to_string(depth + 1,0);
    out.close();*/
    /* Part to print tree */
    
    double BI_value = backward_induction_helper(root, depth);
    

    /* Part to print tree */
    /*file_name = "Sight_";
    file_name += (char)(max_level + '0');
    file_name += "/TreeBI_";
    file_name += (char)(depth + '0');
    file_name += "_completed.txt";
    out.open(file_name);
    out << root->tree_to_string(depth + 1,0);
    out.close();*/
    /* Part to print tree */


    auto child = root->children.cbegin();
    for (; child != root->children.cend(); ++child) {
      // Rounding is needed or sometimes won't catch it
      if (round(100000*(*child)->score_from_below) == round(100000*(1.0 - 
								    BI_value))){
	break;
      }
    }    
    
    return (*child)->move;	
  }


  /* Function to calculate the backward induction values of a tree.
     TIEBREAK version */
  template<typename State>
    typename State::Move backward_induction_tiebreak(Node<State>* root, 
						     int depth){
    
    // Print ree to check
    /* Part to print tree */
    /*std::ofstream out;
    string file_name = "Sight_";
    file_name += (char)(max_level + '0');
    file_name += "/TreeBI_";
    file_name += (char)(depth + '0');
    file_name += ".txt";
    out.open(file_name);
    out << root->tree_to_string(depth + 1,0);
    out.close();*/
    /* Part to print tree */
    
    double BI_value = backward_induction_helper(root, depth, 0);
    

    /* Part to print tree */
    /*file_name = "Sight_";
    file_name += (char)(max_level + '0');
    file_name += "/TreeBI_";
    file_name += (char)(depth + '0');
    file_name += "_completed.txt";
    out.open(file_name);
    out << root->tree_to_string(depth + 1,0);
    out.close();*/
    /* Part to print tree */


    if (root->children.size() != 0 ){
      auto child = root->children.cbegin();
      Node<State>* best_BI_child = NULL; 
      for (; child != root->children.cend(); ++child) {
	if ((round(100000*(*child)->score_from_below) == 
	     round(100000*(1.0 - BI_value))) && (best_BI_child == NULL)){
	  best_BI_child = *child;
	}
	else if ((round(100000*(*child)->score_from_below) == 
		  round(100000*(1.0 - BI_value))) && (best_BI_child != NULL)){
	  if (((*child)->BI_depth) < (best_BI_child->BI_depth)){
	    best_BI_child = *child;
	  }
	}
      }    
    
      return best_BI_child->move;	
    }
   
    return -1;
  }
  /* END OF FUNCTION DEFINITION */




  /* Recursive helper function to calculate the backward induction */
  template<typename State>
    double backward_induction_helper(Node<State>* root, int depth, int level){
    
    if ((depth == 0) || !(root->has_children())) {
      root->score_from_below = root->wins / root->visits;
      root->BI_depth = level;   // ADDED FOR TIEBREAKS
      return root->wins / root->visits;
    }
      
    double best_value = -1;
    double value = -1;
    for (auto child = root->children.begin(); child != root->children.end(); 
	 ++child) {
      value = backward_induction_helper((*child), depth - 1, level + 1);
      best_value = max(best_value, value);
    }    

    root->score_from_below = 1 - best_value;
    return 1 - best_value;
  }
  /* END OF FUNCTION DEFINITION */




  /* Recursive helper function to calculate the backward induction,
     TIEBREAK version */
  template<typename State>
    double backward_induction_helper_adapt(Node<State>* root, int depth, 
					   int level){
    
    // check if it has some kids which are not fully explored
    bool flag_interrupt = false;
    if (root->has_children()){
      for (auto child_check = root->children.begin(); 
	   child_check != root->children.end(); ++child_check) {
	if ((*child_check)->moves.size() > 0){
	  flag_interrupt = true;
	  break;
	}
      }
    }
    

    if ((depth == 0) || !(root->has_children()) || flag_interrupt) {
      root->score_from_below = root->wins / root->visits;
      root->BI_depth = level;   // ADDED FOR TIEBREAKS
      return root->wins / root->visits;
    }
      
    double best_value = -1;
    double value = -1;
    for (auto child = root->children.begin(); child != root->children.end(); 
	 ++child) {
      value = backward_induction_helper_adapt((*child), depth - 1, level + 1);
      best_value = max(best_value, value);
    }    

    root->score_from_below = 1 - best_value;
    return 1 - best_value;
  }
  /* END OF FUNCTION DEFINITION */




  /* Function to calculate the posterior given a prior, link matrix, sight array
     and move chosen */
  RowVectorXd update_prior(const int& observed_move, const vector<int>& 
			   sight_array, const RowVectorXd& prior, const int& 
			   max_sight, const MatrixXd& link_matrix){

    RowVectorXd lambda_evidence = set_lambda_evidence(observed_move, 
						      sight_array, max_sight);

    //cout << "lamda_evidence is: [" << lambda_evidence << "]" <<  endl;

    /* save lambda evidence */
    ofstream out1;
    string filename = "Sight_";
    filename += (char)(max_level + '0');
    filename += "/lambda_evidence.txt";
    out1.open(filename, std::fstream::app);
    for (unsigned int i = 0; i < max_sight; i++){
      out1 << lambda_evidence[i] << " " ;
    }
    out1 << endl;
    out1.close();
    /* save moves_chosen */

    //cout << "prior is: [" << prior << "]" <<  endl;
    RowVectorXd posterior = calculate_posterior(prior, lambda_evidence, 
						max_sight, link_matrix);
    //cout << "posterior is: [" << posterior << "]" <<  endl;

    return posterior;
  } 
  /* END OF FUNCTION DEFINITION */




  /* Function that given a move and a sight array, sets the lambda evidence 
     for the Bayesian network */
  RowVectorXd set_lambda_evidence(const int& observed_move, const vector<int>& 
				  sight_array, const int& max_sight){


    // Initialize vector
    RowVectorXd lambda_evidence(max_sight);
    for (int i=0; i < max_sight; i++){
      lambda_evidence(i) = 0;
    }


    // Set the lamda_evidece vector according to observed_move
    for (int sight = 1; sight <= max_sight; sight++){
      if (sight_array[sight - 1] == observed_move){
	lambda_evidence(sight - 1) = 1;
      }
    } 

    
    // check there is at least one sight = observed_move, otherwise set 
    // to 'no evidence'
    double sum = 0;
    for (int sight = 1; sight <= max_sight; sight++){
      sum += lambda_evidence(sight - 1);
    }
    if (sum == 0){
      for (int sight = 1; sight <= max_sight; sight++){
	lambda_evidence(sight - 1) = 1;
      }
    }

    return lambda_evidence;
  }
  /* END OF FUNCTION DEFINITION */
  


  /* Function that given a prior, link matrix and lambda evidence updates
     the prior into the posterior */
  RowVectorXd calculate_posterior(const RowVectorXd& prior, const RowVectorXd& lambda_evidence,
				  const int& max_sight, const MatrixXd& link_matrix){
    
    // Initialize vector
    RowVectorXd posterior(max_sight);
    for (int i=0; i < max_sight; i++){
      posterior(i) = -1;
    }
    
    RowVectorXd lambda_message = lambda_evidence * link_matrix;  // Matrix multiplication
    
    for (int sight = 1; sight <= max_sight; sight++) {
      posterior(sight - 1) = prior(sight - 1) * lambda_message(sight - 1);   //adjust by prior
    }
    
    /* Normalization of the posterior */
    double sum = 0;
    for (int sight = 1; sight <= max_sight; sight++) {
      sum += posterior(sight - 1);
    }
    for (int sight = 1; sight <= max_sight; sight++) {
      posterior(sight - 1) = posterior(sight - 1)/sum;
    }

    return posterior;
  }
  /* END OF FUNCTION DEFINITION */




  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////


  void check(bool expr, const char* message)
  {
    if (!expr) {
      throw std::invalid_argument(message);
    }
  }

  void assertion_failed(const char* expr, const char* file_cstr, int line)
  {
    using namespace std;

    // Extract the file name only.
    string file(file_cstr);
    auto pos = file.find_last_of("/\\");
    if (pos == string::npos) {
      pos = 0;
    }
    file = file.substr(pos + 1);  // Returns empty string if pos + 1 == length.

    stringstream sout;
    sout << "Assertion failed: " << expr << " in " <<file<< ":" << line << ".";
    throw runtime_error(sout.str().c_str());
  }


}

#endif
