#ifndef LOGISTICREGRESSION_SCALER_H
#define LOGISTICREGRESSION_SCALER_H

//#include <stdlib.h>
//#include <math.h>
#include "../numerical/matrix.h"

typedef struct Scaler {
    double *mean;
    double *std;
} StandardScaler;

StandardScaler fit_and_scale(Matrix X);
void scale(StandardScaler ss, Matrix X);

#endif //LOGISTICREGRESSION_SCALER_H
