#include "logistic.h"

Model logisticfit(LogisticRegression lr, Matrix X, Matrix y) {

    printf("\n================ Logistic Regression ===============\n");
    printf("    - learning rate           : %lf\n", lr.learning_rate);
    printf("    - number of iterations    : %zu\n\n", lr.iterations);

    size_t m = X.rows;
    size_t num_features = X.cols;

    Matrix W = zeros(num_features, 1);
    Matrix b = zeros(1, 1);

    StandardScaler ss = fit_and_scale(X);

    for (size_t i = 0; i < lr.iterations; i++) {

        Matrix Z = mat_mul(X, W);
        Matrix A = sigmoid(Z);
        Matrix E = mat_sub(A, y);

        Matrix mat_log_a = mat_log(A);
	Matrix Temp1 = mat_dot(y, mat_log_a);
	Matrix sub_1_y = sub_from_scalar(1, y);
	Matrix sub_1_A = sub_from_scalar(1, A);
	Matrix mat_log_sub_1_A = mat_log(sub_1_A);
        Matrix Temp2 = mat_dot(sub_1_y, mat_log_sub_1_A);

	Matrix mat_add_temp1_temp2 = mat_add(Temp1, Temp2);
        double cost = - sum(mat_add_temp1_temp2) / m;

	Matrix transpose_X = transpose(X);
	Matrix mat_mul_transpose_X_E = mat_mul(transpose_X, E);
        Matrix dW = scalar_divide(mat_mul_transpose_X_E, m);
        Matrix dB = zeros(1, 1);
	Matrix mat_sub_A_y = mat_sub(A, y);
        double temp = sum(mat_sub_A_y) / m;
        dB.data[0][0] = temp;

	Matrix W_old = W;
	Matrix B_old = b;
	Matrix scalar_mul_dW = scalar_multiply(dW, lr.learning_rate);
        W = mat_sub(W, scalar_mul_dW);
	Matrix scalar_mul_dB = scalar_multiply(dB, lr.learning_rate);
        b = mat_sub(b, scalar_mul_dB);

        printf("  [ %*zu ] training model... cost ---> | %*lf |\n", 3, i, 7, cost);

        dispose(Z);
        dispose(A);
        dispose(E);
        dispose(Temp1);
        dispose(Temp2);
        dispose(dW);
        dispose(dB);
	dispose(mat_log_a);
	dispose(sub_1_y);
	dispose(sub_1_A);
	dispose(mat_log_sub_1_A);
	dispose(mat_add_temp1_temp2);
	dispose(transpose_X);
	dispose(mat_mul_transpose_X_E);
	dispose(mat_sub_A_y);
	dispose(W_old);
	dispose(B_old);
	dispose(scalar_mul_dW);
	dispose(scalar_mul_dB);
    }

    printf("\n================= Training Complete ================\n");
    printf("final weights and biases...\n");

    print_matrix(W);
    print_matrix(b);

    Model model = {ss, W, b};
    return model;
}

Matrix predict(Model model, Matrix X) {
    //printf("X0: %lf X1: %lf\n",X.data[0][0], X.data[0][1]);
    Matrix Y = zeros(X.rows, 1);
    scale(model.ss, X);
    Matrix Z = mat_mul(X, model.W);
    //printf("(predict) Z: %lf\n",Z.data[0][0]);
    Matrix A = sigmoid(Z);
    printf("sigmoid: %lf\n",A.data[0][0]);

    for (size_t i = 0; i < A.rows; i++) {
        if (A.data[i][0] >= 0.5) {
            Y.data[i][0] = 1;
        } else {
            Y.data[i][0] = 0;
        }
    }

    return Y;
}

double get_accuracy(Matrix Y, Matrix Y_hat) {
    printf("calculating the model accuracy...");
    size_t m = Y.rows;
    int sum = 0;
    for (size_t i = 0; i < m; i++) {
        if (Y.data[i][0] == Y_hat.data[i][0]) {
            sum++;
        }
    }
    double accuracy = ((double) sum) / m;

    printf("\n    =================================\n");
    printf("    || Accuracy     || %*lf ||\n", 11, accuracy);
    printf("    =================================\n");

    return accuracy;
}
