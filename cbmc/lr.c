#include <stdint.h>
#include <stddef.h>


typedef struct
{
    double *data;
    uint32_t size;
} notamatrix_t;

typedef struct
{
    double *mean;
    double *std;
} ss_t;

typedef struct
{
    ss_t ss;
    notamatrix_t W;
} model_t;


uint8_t predict(model_t model, double* X) {
    
    for (uint32_t i = 0; i < model.W.size; i++)
        X[i] = (X[i] - model.ss.mean[i]) / model.ss.std[i];

    double mulres = 0;
    for(uint32_t i = 0; i < model.W.size; i++)
        mulres += X[i] * model.W.data[i]; 

    return ( (1 / (1 + exp(-mulres))) >= 0.5)? 1 : 0;
}