#include "split.h"

TrainTestSplit split_train_test(Matrix X, Matrix y, float test_percentage) {
    printf("splitting the train/test with the ratio... < %0.1f | %0.1f >\n",
           1 - test_percentage, test_percentage);
    size_t length = X.rows;
    size_t features = X.cols;
    int train_length = (int) (length * (1 - test_percentage));
    int test_length = (int) (length * test_percentage);

    printf("calculating train set length... ( %d )\ncalculating test set length... ( %d )\n",
           train_length, test_length);

    Matrix TrainX = zeros(train_length, features);
    Matrix TrainY = zeros(train_length, 1);
    Matrix TestX = zeros(test_length, features);
    Matrix TestY = zeros(test_length, 1);

    for (size_t i = 0; i < length; i++) {
        for (size_t j = 0; j < features; j++) {
            if (i < train_length) {
                TrainX.data[i][j] = X.data[i][j];
            } else {
                TestX.data[i - train_length][j] = X.data[i][j];
            }
        }
    }

    for (size_t i = 0; i < length; i++) {
        for (size_t j = 0; j < features; j++) {
            if (i < train_length) {
                TrainY.data[i][0] = y.data[i][0];
            } else {
                TestY.data[i - train_length][0] = y.data[i][0];
            }
        }
    }

    TrainTestSplit tts = {
            TrainX,
            TrainY,
            TestX,
            TestY
    };

    return tts;
}