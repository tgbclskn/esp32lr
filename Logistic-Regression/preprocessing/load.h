#ifndef LOGISTICREGRESSION_LOAD_H
#define LOGISTICREGRESSION_LOAD_H

#include "../numerical/matrix.h"
#include "../util/printer.h"
#include <string.h>
#include <errno.h>

typedef struct Dataset {
     Matrix X;
     Matrix y;
} Iris;

Iris load_iris(char *path);

#endif //LOGISTICREGRESSION_LOAD_H
