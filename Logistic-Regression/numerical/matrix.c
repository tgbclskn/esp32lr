#include "matrix.h"
#include <math.h>
#ifdef DEBUG
extern unsigned long tsc_sigmoid;
#endif
//----------------------- Matrix Implementation ----------------------//
static Matrix init_matrix(size_t rows, size_t cols) {
    Matrix matrix;
    double **x;
    x = malloc(rows * sizeof *x);
    for (size_t i = 0; i < rows; i++) {
        x[i] = malloc(cols * sizeof *x[i]);
    }
    matrix.data = x;
    matrix.rows = rows;
    matrix.cols = cols;
    return matrix;
}

Matrix create_matrix(size_t rows, size_t cols, const double *data) {
    Matrix matrix;
    double **x;
    size_t counter = 0;
    x = malloc(rows * sizeof *x);
    for (size_t i = 0; i < rows; i++) {
        x[i] = malloc(cols * sizeof *x[i]);
        for (size_t j = 0; j < cols; j++) {
            x[i][j] = *(data + counter);
            counter++;
        }
    }
    matrix.data = x;
    matrix.rows = rows;
    matrix.cols = cols;
    return matrix;
}

//-------------------- Zeros ----------------------//
Matrix zeros(size_t rows, size_t cols) {
    Matrix x = init_matrix(rows, cols);
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            x.data[i][j] = 0.0;
        }
    }
    return x;
}

//------------------ Ones ---------------------//
Matrix ones(size_t rows, size_t cols) {
    Matrix x = init_matrix(rows, cols);
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            x.data[i][j] = 1.0;
        }
    }
    return x;
}

//-------------------- Sum ---------------------//
double sum(Matrix A) {
    double sum = 0.0;
    for (size_t i = 0; i < A.rows; i++) {
        for (size_t j = 0; j < A.cols; ++j) {
            sum += A.data[i][j];
        }
    }
    return sum;
}

//---------------------- Mean ---------------------//
double mean(Matrix A) {
    double sum = 0.0;
    double num_elements = (double) (A.rows * A.cols);
    for (size_t i = 0; i < A.rows; i++) {
        for (size_t j = 0; j < A.cols; ++j) {
            sum += A.data[i][j];
        }
    }
    return sum / num_elements;
}

//---------------------- STD ---------------------//
double std(Matrix A) {
    double sum = 0.0;
    double num_elements = (double) (A.rows * A.cols);
    for (size_t i = 0; i < A.rows; i++) {
        for (size_t j = 0; j < A.cols; ++j) {
            sum += pow(A.data[i][j] - mean(A), 2);
        }
    }
    return sqrt(sum / (num_elements - 1));
}

//-------------------------- Transpose -------------------------//
Matrix transpose(Matrix A) {
    size_t new_rows = A.cols;
    size_t new_cols = A.rows;
    Matrix T = init_matrix(new_rows, new_cols);
    for (size_t i = 0; i < A.cols; i++) {
        for (size_t j = 0; j < A.rows; ++j) {
            T.data[i][j] = A.data[j][i];
        }
    }
    T.rows = new_rows;
    T.cols = new_cols;
    return T;
}

//------------------------ Matrix Addition ------------------------//
Matrix mat_add(Matrix A, Matrix B) {
    if (A.rows != B.rows || A.cols != B.cols) {
        perror("MAT_ADD_ERROR --> MATRIX DIMENSIONS MUST MATCH");
        exit(EXIT_FAILURE);
    }
    Matrix M = init_matrix(A.rows, B.cols);
    for (size_t i = 0; i < A.rows; i++) {
        for (size_t j = 0; j < A.cols; ++j) {
            M.data[i][j] = A.data[i][j] + B.data[i][j];
        }
    }
    return M;
}

//------------------------- Matrix Subtraction --------------------------//
Matrix mat_sub(Matrix A, Matrix B) {
    if (A.rows != B.rows || A.cols != B.cols) {
        perror("MAT_SUB_ERROR --> MATRIX DIMENSIONS MUST MATCH");
        exit(EXIT_FAILURE);
    }
    Matrix M = init_matrix(A.rows, B.cols);
    for (size_t i = 0; i < A.rows; i++) {
        for (size_t j = 0; j < A.cols; ++j) {
            M.data[i][j] = A.data[i][j] - B.data[i][j];
        }
    }
    return M;
}

//---------------------- Matrix Multiplication ---------------------//
Matrix mat_mul(Matrix A, Matrix B) {
    if (A.cols != B.rows) {
        perror("MAT_MUL_ERROR --> INCORRECT DIMENSION");
        exit(EXIT_FAILURE);
    }
    Matrix M = zeros(A.rows, B.cols);
    for (size_t i = 0; i < A.rows; i++) {
        for (size_t j = 0; j < B.cols; ++j) {
            for(size_t k = 0; k < B.rows; k++) {
                M.data[i][j] += (A.data[i][k] * B.data[k][j]);
                //printf("M.data[%u][%u] (%lf) += A.data[i][k] (%lf) * B.data[k][j] (%lf)\n",i,j, M.data[i][j], A.data[i][k], B.data[k][j]);
            }
        }
    }
    return M;
}

//---------------------- Matrix Elementwise Multiplication ---------------------//
Matrix mat_dot(Matrix A, Matrix B) {
    if (A.rows != B.rows) {
        perror("MAT_DOT_ERROR --> MATRIX DIMENSIONS MUST MATCH");
        exit(EXIT_FAILURE);
    }
    Matrix M = init_matrix(A.rows, B.cols);
    for (size_t i = 0; i < A.rows; i++) {
        for (size_t j = 0; j < A.cols; ++j) {
            if (B.cols == 1) {
                M.data[i][j] = A.data[i][j] * B.data[i][0];
            } else {
                M.data[i][j] = A.data[i][j] * B.data[i][j];
            }
        }
    }
    return M;
}

//-------------------- Scalar Multiplication -------------------//
Matrix scalar_multiply(Matrix A, double n) {
    Matrix M = init_matrix(A.rows, A.cols);
    for (size_t i = 0; i < A.rows; i++) {
        for (size_t j = 0; j < A.cols; ++j) {
            M.data[i][j] = A.data[i][j] * n;
        }
    }
    return M;
}

//-------------------- Scalar division -------------------//
Matrix scalar_divide(Matrix A, double n) {
    Matrix D = init_matrix(A.rows, A.cols);
    for (size_t i = 0; i < A.rows; i++) {
        for (size_t j = 0; j < A.cols; ++j) {
            D.data[i][j] = A.data[i][j] / n;
        }
    }
    return D;
}

//---------------------- Matrix Power ---------------------//
Matrix mat_power(Matrix A, double n) {
    Matrix P = init_matrix(A.rows, A.cols);
    for (size_t i = 0; i < A.rows; i++) {
        for (size_t j = 0; j < A.cols; ++j) {
            P.data[i][j] = pow(A.data[i][j], n);
        }
    }
    return P;
}

//---------------------- Matrix Log ---------------------//
Matrix mat_log(Matrix A) {
    Matrix L = init_matrix(A.rows, A.cols);
    for (size_t i = 0; i < A.rows; i++) {
        for (size_t j = 0; j < A.cols; ++j) {
            L.data[i][j] = log(A.data[i][j]);
        }
    }
    return L;
}

//------------------------- Sigmoid -----------------------//
Matrix sigmoid(Matrix A) {
#ifdef DEBUG
unsigned long tsc_sigmoid_start = __builtin_ia32_rdtsc();
#endif
    Matrix S = init_matrix(A.rows, A.cols);
    for (size_t i = 0; i < A.rows; i++) {
        for (size_t j = 0; j < A.cols; ++j) {
            S.data[i][j] = 1 / (1 + exp(-A.data[i][j]));
        }
    }
#ifdef DEBUG
tsc_sigmoid += __builtin_ia32_rdtsc() - tsc_sigmoid_start;
#endif
    return S;
}

//---------------------- Sub from scalar -------------------//
Matrix sub_from_scalar(double n, Matrix A) {
    Matrix S = init_matrix(A.rows, A.cols);
    for (size_t i = 0; i < A.rows; i++) {
        for (size_t j = 0; j < A.cols; ++j) {
            S.data[i][j] = n - A.data[i][j];
        }
    }
    return S;
}

//---------------------- Matrix Shuffle -------------------//
void shuffle(Matrix X) {
    for (size_t i = 0; i < X.rows; i++) {
        size_t r = i + rand() / (RAND_MAX / (X.rows - i) + 1);
        for (size_t j = 0; j < X.cols; ++j) {
            double temp = X.data[r][j];
            X.data[r][j] = X.data[i][j];
            X.data[i][j] = temp;
        }
    }
}

//--------------------- Dispose -------------------//
void dispose(Matrix A) {
    for (size_t i = 0; i < A.rows; i ++) {
        free(A.data[i]);
    }
    free(A.data);
}
