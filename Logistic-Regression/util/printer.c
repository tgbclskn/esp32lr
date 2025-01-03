#include "printer.h"

void print_matrix(Matrix A) {
    printf("\n");
    for (int i = 0; i < A.rows; i++) {
        printf("    |");
        for (int j = 0; j < A.cols; ++j) {
            int space = 7;
            printf(" %*.2lf ", space, A.data[i][j]);
        }
        printf("|\n");
    }
    printf("\n");
}