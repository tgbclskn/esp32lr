#include "scaler.h"

static void fit(Matrix X, double *mean, double *std) {
    for (size_t i = 0; i < X.cols; i++) {
        double sum = 0;
        for (size_t j = 0; j < X.rows; j++) {
            sum += X.data[j][i];
        }
        mean[i] = sum / X.rows;
    }
    for (size_t i = 0; i < X.cols; i++) {
        double sum = 0;
        for (size_t j = 0; j < X.rows; j++) {
            sum += pow(X.data[j][i] - mean[i], 2);
        }
        std[i] = sqrt(sum / (double) X.rows);
    }
}

StandardScaler fit_and_scale(Matrix X) {
    double *mean = malloc(X.cols * sizeof *mean);
    double *std = malloc(X.cols * sizeof *std);
    fit(X, mean, std);
    StandardScaler ss = {mean, std};
    scale(ss, X);
    return ss;
}

void scale(StandardScaler ss, Matrix X) {
    for (size_t i = 0; i < X.cols; i++) {
        for (size_t j = 0; j < X.rows; j++) {
            X.data[j][i] = (X.data[j][i] - ss.mean[i]) / ss.std[i];
        }
    }
}
