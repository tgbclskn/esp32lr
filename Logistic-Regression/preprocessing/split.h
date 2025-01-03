#ifndef LOGISTICREGRESSION_SPLIT_H
#define LOGISTICREGRESSION_SPLIT_H

#include "../numerical/matrix.h"

typedef struct Split {
    Matrix TrainX;
    Matrix TrainY;
    Matrix TestX;
    Matrix TestY;
} TrainTestSplit;

TrainTestSplit split_train_test(Matrix X, Matrix y, float test_percentage);

#endif //LOGISTICREGRESSION_SPLIT_H
