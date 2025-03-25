#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define TOKEN_SIZE 10
#define PAWN   "pawn"
#define KNIGHT "knight"
#define ROOK   "rook"
#define QUEEN  "queen"
#define KING   "king"
#define MOVE_SIZE 6
const char CHESS_TOKEN[] = "CHESS";

char* createBlackToken(char* name)
{
    char* tokenHolder = (char*)malloc(sizeof(char) * TOKEN_SIZE);
    if (tokenHolder == NULL)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < TOKEN_SIZE; ++i)
    {
        tokenHolder[i] = rand() % 255;
    }
    return tokenHolder;
}

char* createWhiteToken(char* previousToken)
{
    char* currentToken = (char*)malloc(sizeof(char) * TOKEN_SIZE);
    if (currentToken == NULL)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    int i = 0;
    if (previousToken != NULL)
    {
        while (1)
        {
            if (i >= TOKEN_SIZE)
            {
                currentToken[TOKEN_SIZE - 1] = '\0';
                return currentToken;
            }
            else if (i < strlen(CHESS_TOKEN))
                currentToken[i] = CHESS_TOKEN[i];
            else
                currentToken[i] = previousToken[i] + 1;
            i++;
        }
    }
    free(currentToken);
    return NULL;
}

char* initFirstMove(char* whiteToken)
{
    if (strncmp(whiteToken, CHESS_TOKEN, strlen(CHESS_TOKEN)) != 0)
        return whiteToken;

    int choice;
    printf(
        "0: A King move\n"
        "1: A Queen move\n"
        "2: A Rook move\n"
        "3: A Knight move\n"
        "4: A Pawn move\n"
        "White's turn, enter the first move: ");
    if (scanf("%d", &choice) != 1 || choice < 0 || choice > 4) {
        printf("Invalid input. Defaulting to Pawn move.\n");
        choice = 4;
    }
    whiteToken = (char*)(malloc(sizeof(char) * MOVE_SIZE));
    switch (choice)
    {
    case(0):
        strncpy(whiteToken, KING, MOVE_SIZE - 1);
        break;
    case(1):
        strncpy(whiteToken, QUEEN, MOVE_SIZE - 1);
        break;
    case(2):
        strncpy(whiteToken, ROOK, MOVE_SIZE - 1);
        break;
    case(3):
        strncpy(whiteToken, KNIGHT, MOVE_SIZE - 1);
        break;
    case(4):
        strncpy(whiteToken, PAWN, MOVE_SIZE - 1);
        break;
    }
    whiteToken[MOVE_SIZE - 1] = '\0'; 
    return whiteToken;
}

int main(int argc, char* argv[])
{
    srand((unsigned int)time(0));

    char* blackToken = createBlackToken(argv[1]);
    printf("Black's Token: ");
    for (int i = 0; i < TOKEN_SIZE; i++) printf("%c", blackToken[i]);
    printf("\n");

    char* whiteToken = createWhiteToken(blackToken);
    printf("White's Token: %s\n", whiteToken);

    char* firstMove = initFirstMove(whiteToken);
    printf("White's First Move: %s\n", firstMove);

    // Clean up
    free(blackToken);
    free(whiteToken);
    free(firstMove);

    return EXIT_SUCCESS;
}
