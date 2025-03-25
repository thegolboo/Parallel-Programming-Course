#include <chrono>
#include <iostream>
#include <omp.h>

using namespace std::chrono;
using namespace std;

int m, n, k;
int solutions = 0;

void makeBoard(char** board) {
#pragma omp parallel for collapse(2) schedule(static)
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            board[i][j] = '_';
        }
    }
}

void displayBoard(char** board) {
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            cout << " " << board[i][j] << " ";
        }
        cout << "\n";
    }
    cout << "\n";
}

void attack(int i, int j, char a, char** board) {
    const int moves[8][2] = {
        {2, -1}, {2, 1}, {-2, -1}, {-2, 1},
        {1, 2}, {1, -2}, {-1, 2}, {-1, -2}
    };
    for (int move = 0; move < 8; ++move) {
        int ni = i + moves[move][0];
        int nj = j + moves[move][1];
        if (ni >= 0 && ni < m && nj >= 0 && nj < n) {
            board[ni][nj] = a;
        }
    }
}

bool canPlace(int i, int j, char** board) {
    return board[i][j] == '_';
}

void place(int i, int j, char k, char a, char** board, char** new_board) {
#pragma omp parallel for collapse(2)
    for (int y = 0; y < m; y++) {
        for (int z = 0; z < n; z++) {
            new_board[y][z] = board[y][z];
        }
    }
    new_board[i][j] = k;
    attack(i, j, a, new_board);
}

void kkn(int k, int sti, int stj, char** board) {
    if (k == 0) {
#pragma omp critical
        {
            displayBoard(board);
            solutions++;
        }
    }
    else {
#pragma omp parallel for schedule(dynamic, 1) collapse(2)
        for (int i = sti; i < m; i++) {
            for (int j = (i == sti ? stj : 0); j < n; j++) {
                if (canPlace(i, j, board)) {
                    char** new_board = new char* [m];
                    for (int x = 0; x < m; x++) {
                        new_board[x] = new char[n];
                    }
                    place(i, j, 'K', 'A', board, new_board);
                    kkn(k - 1, i, j, new_board);
                    for (int x = 0; x < m; x++) {
                        delete[] new_board[x];
                    }
                    delete[] new_board;
                }
            }
        }
    }
}

int main() {
    m = 4, n = 4, k = 4;

    char** board = new char* [m];
    for (int i = 0; i < m; i++) {
        board[i] = new char[n];
    }

    auto start = high_resolution_clock::now();

    makeBoard(board);
    kkn(k, 0, 0, board);

    auto stop = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(stop - start);

    cout << endl << "Total number of solutions : " << solutions;
    cout << endl << "Time (milliseconds): " << duration.count() << endl;

    for (int i = 0; i < m; i++) {
        delete[] board[i];
    }
    delete[] board;

    return 0;
}
