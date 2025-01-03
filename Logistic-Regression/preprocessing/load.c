#include "load.h"

#define DATA_ROWS 410
#define DATA_COLS 3

static Matrix get_x(Matrix dataset) {
    Matrix X = zeros(dataset.rows, dataset.cols - 1);
    for (int i = 0; i < dataset.rows; i++) {
        for (int j = 0; j < dataset.cols - 1; j++) {
            X.data[i][j] = dataset.data[i][j];
        }
    }
    return X;
}

static Matrix get_y(Matrix dataset) {
    Matrix y = zeros(dataset.rows, 1);
    for (int i = 0; i < dataset.rows; i++) {
        y.data[i][0] = dataset.data[i][dataset.cols - 1];
    }
    return y;
}

Iris load_iris(char *path) {
    printf("loading data from %s...\n", path);
    FILE *stream = fopen(path, "r");
    if (stream == NULL) {
        perror("ERROR OPENING THE FILE");
        printf("%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    Matrix dataset = zeros(DATA_ROWS, DATA_COLS);

    char line[1024];
    size_t i = 0;
    while (fgets(line, 1024, stream)) {
        char *token = strtok(line, ",");
        size_t j = 0;
        while (token != NULL) {
            double x = (double) strtof(token, (char **) NULL);
            dataset.data[i][j] = x;
            token = strtok(NULL, ",");
            j++;
        }
        i++;
    }
    printf("number of training examples... ( %lu )\n", i);
    fclose(stream);
    printf("shuffling the dataset...\n");

    ////////////////////
    shuffle(dataset);
    ////////////////////
    Iris IrisData = {
            get_x(dataset),
            get_y(dataset)
    };

    dispose(dataset);

    return IrisData;
}
