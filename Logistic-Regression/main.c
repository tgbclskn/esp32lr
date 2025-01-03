#include "numerical/matrix.h"
#include "preprocessing/load.h"
#include "preprocessing/split.h"
#include "regression/logistic.h"

#ifdef DEBUG
unsigned long tsc_sigmoid = 0;
unsigned long tsc_main = 0;
unsigned long tsc_load = 0;
unsigned long tsc_tts = 0;
unsigned long tsc_train = 0;
unsigned long tsc_predict = 0;
#endif

int main() {

#ifdef DEBUG
    unsigned long mainstart = __builtin_ia32_rdtsc();
    unsigned long start = __builtin_ia32_rdtsc();
#endif

//    Iris IrisDataset = load_iris("./dataset/Iris.csv");
    Iris IrisDataset = load_iris("data");

#ifdef DEBUG
   tsc_load += __builtin_ia32_rdtsc() - start;
#endif

    Matrix X = IrisDataset.X;
    Matrix y = IrisDataset.y;

#ifdef DEBUG
    start = __builtin_ia32_rdtsc();
#endif

    TrainTestSplit tts = split_train_test(X, y, 0.5F);

#ifdef DEBUG
    tsc_tts += __builtin_ia32_rdtsc() - start;
#endif

    Matrix TrainX = tts.TrainX;
    Matrix TestX = tts.TestX;
    Matrix TrainY = tts.TrainY;
    Matrix TestY = tts.TestY;

    dispose(X);
    dispose(y);

    LogisticRegression lr = {0.01, 1000};

#ifdef DEBUG
    start = __builtin_ia32_rdtsc();
#endif

    Model trained_model = logisticfit(lr, TrainX, TrainY);

#ifdef DEBUG
    tsc_train += __builtin_ia32_rdtsc() - start;
    start = __builtin_ia32_rdtsc();
#endif

    Matrix Y_hat = predict(trained_model, TestX);

#ifdef DEBUG
    tsc_predict += __builtin_ia32_rdtsc() - start;
#endif

    get_accuracy(TestY, Y_hat);

#ifdef DEBUG
    tsc_main += __builtin_ia32_rdtsc() - mainstart;

printf("\ntsc_main   :%lu\n",tsc_main);
printf("tsc_load   :%lu (%lf)\n",tsc_load,(100*((double)tsc_load/(double)tsc_main)));
printf("tsc_tts    :%lu (%lf)\n",tsc_tts,(100*((double)tsc_tts/(double)tsc_main)));
printf("tsc_train  :%lu (%lf)\n",tsc_train,(100*((double)tsc_train/(double)tsc_main)));
printf("tsc_sigmoid:%lu (%lf)\n",tsc_sigmoid,(100*((double)tsc_sigmoid/(double)tsc_main)));
printf("tsc_predict:%lu (%lf)\n",tsc_predict,(100*((double)tsc_predict/(double)tsc_main)));
#endif

    return 0;
}
