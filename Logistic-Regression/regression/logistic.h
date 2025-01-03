#ifndef LOGISTICREGRESSION_LOGISTIC_H
#define LOGISTICREGRESSION_LOGISTIC_H

#include "../numerical/matrix.h"
#include "../util/printer.h"
#include "../preprocessing/scaler.h"

typedef struct Regression {
    double learning_rate;
    size_t iterations;

} LogisticRegression;

typedef struct Models {
    StandardScaler ss;
    Matrix W;
    Matrix b;
} Model;

Model logisticfit(LogisticRegression lr, Matrix X, Matrix y);
Matrix predict(Model model, Matrix X);
double get_accuracy(Matrix Y, Matrix Y_hat);

#endif //LOGISTICREGRESSION_LOGISTIC_H
