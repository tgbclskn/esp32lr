#ifndef LOGISTICREGRESSION_MATRIX_H
#define LOGISTICREGRESSION_MATRIX_H

//#include <stdio.h>
//#include <stdlib.h>

typedef struct Matrices {
    double **data;
    size_t rows;
    size_t cols;
} Matrix;

Matrix create_matrix(size_t rows, size_t cols, const double *data);

Matrix zeros(size_t rows, size_t cols);
Matrix ones(size_t rows, size_t cols);

double sum(Matrix A);
double mean(Matrix A);
double std(Matrix A);

Matrix transpose(Matrix A);
Matrix mat_add(Matrix A, Matrix B);
Matrix mat_sub(Matrix A, Matrix B);
Matrix mat_mul(Matrix A, Matrix B);
Matrix mat_dot(Matrix A, Matrix B);
Matrix scalar_multiply(Matrix A, double n);
Matrix scalar_divide(Matrix A, double n);
Matrix mat_power(Matrix A, double n);
Matrix mat_log(Matrix A);
Matrix sigmoid(Matrix A);
Matrix sub_from_scalar(double n, Matrix A);
void shuffle(Matrix X);

void dispose(Matrix A);


#endif //LOGISTICREGRESSION_MATRIX_H
