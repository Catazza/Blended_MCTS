// Petter Strandmark 2013
// petter.strandmark@gmail.com

#include <algorithm>
#include <iostream>
using namespace std;

#include <mcts.h>

class ConnectFourState
{
 public:
  typedef int Move;
  static const Move no_move = -1;

  static const char player_markers[3]; 

 ConnectFourState(int num_rows_ = 6, int num_cols_ = 7)
   : player_to_move(1),
    num_rows(num_rows_),
    num_cols(num_cols_),
    last_col(-1),
    last_row(-1)
      { 
	board.resize(num_rows, vector<char>(num_cols, player_markers[0]));
      }

  void do_move(Move move)
  {
    attest(0 <= move && move < num_cols);
    attest(board[0][move] == player_markers[0]);
    check_invariant();

    int row = num_rows - 1;
    while (board[row][move] != player_markers[0]) row--;
    board[row][move] = player_markers[player_to_move];
    last_col = move;
    last_row = row;

    player_to_move = 3 - player_to_move;
  }

  template<typename RandomEngine>
    void do_random_move(RandomEngine* engine)
    {
      dattest(has_moves());
      check_invariant();
      std::uniform_int_distribution<Move> moves(0, num_cols - 1);

      while (true) {
	auto move = moves(*engine);
	if (board[0][move] == player_markers[0]) {
	  do_move(move);
	  return;
	}
      }
    }

  bool has_moves() const
  {
    check_invariant();

    char winner = get_winner();
    if (winner != player_markers[0]) {
      return false;
    }

    for (int col = 0; col < num_cols; ++col) {
      if (board[0][col] == player_markers[0]) {
	return true;
      }
    }
    return false;
  }

  std::vector<Move> get_moves() const
    {
      check_invariant();

      std::vector<Move> moves;
      if (get_winner() != player_markers[0]) {
	return moves;
      }

      moves.reserve(num_cols);

      for (int col = 0; col < num_cols; ++col) {
	if (board[0][col] == player_markers[0]) {
	  moves.push_back(col);
	}
      }
      return moves;
    }

  char get_winner() const
  {
    if (last_col < 0) {
      return player_markers[0];
    }

    // We only need to check around the last piece played.
    auto piece = board[last_row][last_col];

    // X X X X
    int left = 0, right = 0;
    for (int col = last_col - 1; col >= 0 && board[last_row][col] == piece; --col) left++;
    for (int col = last_col + 1; col < num_cols && board[last_row][col] == piece; ++col) right++;
    if (left + 1 + right >= 4) {
      return piece;
    }

    // X
    // X
    // X
    // X
    int up = 0, down = 0;
    for (int row = last_row - 1; row >= 0 && board[row][last_col] == piece; --row) up++;
    for (int row = last_row + 1; row < num_rows && board[row][last_col] == piece; ++row) down++;
    if (up + 1 + down >= 4) {
      return piece;
    }

    // X
    //  X
    //   X
    //    X
    up = 0;
    down = 0;
    for (int row = last_row - 1, col = last_col - 1; row >= 0 && col >= 0 && board[row][col] == piece; --row, --col) up++;
    for (int row = last_row + 1, col = last_col + 1; row < num_rows && col < num_cols && board[row][col] == piece; ++row, ++col) down++;
    if (up + 1 + down >= 4) {
      return piece;
    }

    //    X
    //   X
    //  X
    // X
    up = 0;
    down = 0;
    for (int row = last_row + 1, col = last_col - 1; row < num_rows && col >= 0 && board[row][col] == piece; ++row, --col) up++;
    for (int row = last_row - 1, col = last_col + 1; row >= 0 && col < num_cols && board[row][col] == piece; --row, ++col) down++;
    if (up + 1 + down >= 4) {
      return piece;
    }

    return player_markers[0];
  }

  double get_result(int current_player_to_move) const
  {
    dattest( ! has_moves());
    check_invariant();

    auto winner = get_winner();
    if (winner == player_markers[0]) {
      return 0.5;
    }

    if (winner == player_markers[current_player_to_move]) {
      return 0.0;
    }
    else {
      return 1.0;
    }
  }

  void print(ostream& out) const
  {
    out << endl;
    out << " ";
    for (int col = 0; col < num_cols - 1; ++col) {
      out << col << ' ';
    }
    out << num_cols - 1 << endl;
    for (int row = 0; row < num_rows; ++row) {
      out << "|";
      for (int col = 0; col < num_cols - 1; ++col) {
	out << board[row][col] << ' ';
      }
      out << board[row][num_cols - 1] << "|" << endl;
    }
    out << "+";
    for (int col = 0; col < num_cols - 1; ++col) {
      out << "--";
    }
    out << "-+" << endl;
    out << player_markers[player_to_move] << " to move " << endl << endl;
  }

  vector<vector<char>> getBoard(){
    return board;
  }




  int player_to_move;
  
 private:

  void check_invariant() const
  {
    attest(player_to_move == 1 || player_to_move == 2);
  }

  int num_rows, num_cols;
  vector<vector<char>> board;
  int last_col;
  int last_row;
};

ostream& operator << (ostream& out, const ConnectFourState& state)
{
  state.print(out);
  return out;
}

const char ConnectFourState::player_markers[3] = {'.', 'X', 'O'}; 






//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------

// MINIMAX VERSION TRIAL


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

const int width = 7;
const int height = 6;
const int orangeWins = 1000000;
const int yellowWins = -orangeWins;

int g_maxDepth = 7;

enum Mycell {
    Orange=1,
    Yellow=-1, // See starting comment below in ScoreBoard
    Barren=0
};

struct Board {
    Mycell _slots[height][width];
    Board() {
        memset(_slots, 0, sizeof(_slots));
    }
};

bool inside(int y, int x)
{
    return y>=0 && y<height && x>=0 && x<width;
}

int ScoreBoard(const Board& board)
{
    int counters[9] = {0,0,0,0,0,0,0,0,0};

    // C++ does not need a "translation stage", like this one:
    //int scores[height][width];
    //
    //for(int y=0; y<height; y++)
    //    for(int x=0; x<width; x++)
    //        switch(board._slots[y][x]) {
    //        case Orange:
    //            scores[y][x] = 1; break;
    //        case Yellow:
    //            scores[y][x] = -1; break;
    //        case Barren:
    //            scores[y][x] = 0; break;
    //        }
    //
    // Instead, enumerants can be accessed as integers:
    typedef Mycell EnumsAreIntegersInC[height][width];
    const EnumsAreIntegersInC& scores = board._slots;

    // Horizontal spans
    for(int y=0; y<height; y++) {
        int score = scores[y][0] + scores[y][1] + scores[y][2];
        for(int x=3; x<width; x++) {
            assert(inside(y,x));
            score += scores[y][x];
            counters[score+4]++;
            assert(inside(y,x-3));
            score -= scores[y][x-3];
        }
    }
    // Vertical spans
    for(int x=0; x<width; x++) {
        int score = scores[0][x] + scores[1][x] + scores[2][x];
        for(int y=3; y<height; y++) {
            assert(inside(y,x));
            score += scores[y][x];
            counters[score+4]++;
            assert(inside(y-3,x));
            score -= scores[y-3][x];
        }
    }
    // Down-right (and up-left) diagonals
    for(int y=0; y<height-3; y++) {
        for(int x=0; x<width-3; x++) {
            int score = 0;
            for(int idx=0; idx<4; idx++) {
                int yy = y + idx;
                int xx = x + idx;
                assert(inside(yy,xx));
                score += scores[yy][xx];
            }
            counters[score+4]++;
        }
    }
    // up-right (and down-left) diagonals
    for(int y=3; y<height; y++) {
        for(int x=0; x<width-3; x++) {
            int score = 0;
            for(int idx=0; idx<4; idx++) {
                int yy = y - idx;
                int xx = x + idx;
                assert(inside(yy,xx));
                score += scores[yy][xx];
            }
            counters[score+4]++;
        }
    }
/*
For down-right diagonals, I also tried this incremental version of the 
diagonal scores calculations... It is doing less computation than
the alternative above, but unfortunately, the use of the tuple array
makes the overall results worse in my Celeron E3400... I suspect
because the access to the array triggers cache misses.

    static const char dl[][2] = { {0,3},{0,4},{0,5},{0,6},{1,6},{2,6} };
    for(int idx=0; idx<6; idx++) {
        int y = dl[idx][0];
        int x = dl[idx][1];
        assert(inside(y,x));
        int score = scores[y][x] + scores[y+1][x-1] + scores[y+2][x-2];
        while (((y+3)<height) && (x-3)>=0) {
            assert(inside(y+3,x-3));
            score += scores[y+3][x-3];
            counters[score+4]++;
            score -= scores[y][x];
            y++; x--;
        }
    }
*/
    if (counters[0] != 0)
        return yellowWins;
    else if (counters[8] != 0)
        return orangeWins;
    else
        return
            counters[5] + 2*counters[6] + 5*counters[7] -
            counters[3] - 2*counters[2] - 5*counters[1];
}

int dropDisk(Board& board, int column, Mycell color)
{
    for (int y=height-1; y>=0; y--)
        if (board._slots[y][column] == Barren) {
            board._slots[y][column] = color;
            return y;
        }
    return -1;
}

int g_debug = 0;

Board loadBoard(int argc, char *argv[]) 
{
    Board newBoard;
    for(int i=1; i<argc; i++)
        if (argv[i][0] == 'o' || argv[i][0] == 'y')
            newBoard._slots[argv[i][1]-'0'][argv[i][2]-'0'] = (argv[i][0] == 'o')?Orange:Yellow;
        else if (!strcmp(argv[i], "-debug"))
            g_debug = 1;
        else if (!strcmp(argv[i], "-level"))
            g_maxDepth = atoi(argv[i+1]);
    return newBoard;
}

void abMinimax(bool maximizeOrMinimize, Mycell color, int depth, Board& board, int& move, int& score)
{
    int bestScore=maximizeOrMinimize?-10000000:10000000;
    int bestMove=-1;
    for (int column=0; column<width; column++) {
        if (board._slots[0][column]!=Barren) continue;
        int rowFilled = dropDisk(board, column, color);
        if (rowFilled == -1)
            continue;
        int s = ScoreBoard(board);
        if (s == (maximizeOrMinimize?orangeWins:yellowWins)) {
            bestMove = column;
            bestScore = s;
            board._slots[rowFilled][column] = Barren;
            break;
        }
        int moveInner, scoreInner;
        if (depth>1)
            abMinimax(!maximizeOrMinimize, color==Orange?Yellow:Orange, depth-1, board, moveInner, scoreInner);
        else {
            moveInner = -1;
            scoreInner = s;
        }
        board._slots[rowFilled][column] = Barren;
        /* when loss is certain, avoid forfeiting the match, by shifting scores by depth... */
        if (scoreInner == orangeWins || scoreInner == yellowWins)
            scoreInner -= depth * (int)color;
        if (depth == g_maxDepth && g_debug)
            printf("Depth %d, placing on %d, score:%d\n", depth, column, scoreInner);
        if (maximizeOrMinimize) {
            if (scoreInner>=bestScore) {
                bestScore = scoreInner;
                bestMove = column;
            }
        } else {
            if (scoreInner<=bestScore) {
                bestScore = scoreInner;
                bestMove = column;
            }
        }
    }
    move = bestMove;
    score = bestScore;
}
