#include <Arduino.h>
#include <stdint.h>
#include <math.h>
#include "knn.h"

t_training_data training_data = {
    {2, 3, 1}, {
        {0.17571, 0.17572, 0.17566, 0.1757, 0.17551, 0.17458, 0.17268, 0.17147, 0.1703, 0.16876, 0.16527, 0.15937, 0.1531, 0.1508, 0.14792, 0.14425, 0.1386, 0.13568, 0.1332, 0.13013, 0.12619, 0.12274, 0.11892, 0.11778, 0.11518, 0.11365, 0.11165, 0.11031, 0.10805, 0.10606, 0.10474, 0.10357, 0.10224, 0.10108, 0.10006, 0.098344, 0.097635, 0.097253, 0.096734, 0.096205, 0.095019, 0.09487, 0.094542, 0.093684, 0.092445, 0.091174, 0.090179, 0.089745, 0.089215, 0.088273, 0.087309, 0.086906, 0.086038, 0.085127, 0.08429, 0.083644, 0.083327, 0.08357, 0.083316, 0.082988, 0.082469, 0.082024, 0.082088, 0.082511, 0.081728, 0.081442, 0.080372, 0.080203, 0.079271, 0.078699, 0.078635, 0.078106, 0.077968, 0.077343, 0.077142},
        {0.10314, 0.10338, 0.10359, 0.10384, 0.10413, 0.1044, 0.1046, 0.10483, 0.10513, 0.1054, 0.10569, 0.10601, 0.10635, 0.10656, 0.10694, 0.10749, 0.10763, 0.10786, 0.10815, 0.10885, 0.10904, 0.10925, 0.10966, 0.10992, 0.11007, 0.11019, 0.11042, 0.1108, 0.11102, 0.11141, 0.11177, 0.11226, 0.11259, 0.11294, 0.11384, 0.1142, 0.11451, 0.11484, 0.11544, 0.11574, 0.11612, 0.11643, 0.11686, 0.11731, 0.11768, 0.118, 0.11847, 0.11878, 0.11909, 0.11954, 0.12048, 0.12091, 0.1214, 0.12175, 0.12194, 0.12214, 0.12247, 0.12278, 0.12307, 0.12333, 0.12354, 0.12383, 0.12402, 0.12425, 0.12461, 0.1252, 0.12556, 0.12609, 0.1264, 0.12677, 0.12703, 0.12734, 0.12759, 0.1279, 0.12835},
        {0.13628, 0.13632, 0.13637, 0.13637, 0.13645, 0.13647, 0.13651, 0.13651, 0.13647, 0.13645, 0.13649, 0.13656, 0.13658, 0.1366, 0.13662, 0.13668, 0.13675, 0.1367, 0.13677, 0.13679, 0.13677, 0.13637, 0.13481, 0.13437, 0.1338, 0.13361, 0.13333, 0.13274, 0.13234, 0.13188, 0.13182, 0.13167, 0.13182, 0.13167, 0.13131, 0.13068, 0.12864, 0.12504, 0.11942, 0.11744, 0.11295, 0.11051, 0.10842, 0.10695, 0.10417, 0.10341, 0.10236, 0.10158, 0.10053, 0.098422, 0.097706, 0.096063, 0.094631, 0.091978, 0.089325, 0.087809, 0.085956, 0.084671, 0.084313, 0.083829, 0.083492, 0.083218, 0.08265, 0.082186, 0.081513, 0.079807, 0.078501, 0.076754, 0.076775, 0.076733, 0.076143, 0.075743, 0.075301, 0.075006, 0.07448}
    } 
};

uint8_t knn(t_training_data* training, uint8_t k, float *X0, uint8_t X0_len, uint8_t mode)
{
    float d, prod1, prod2, prod3, root1, root2, denom, *vd, *distk;
    uint8_t max_occ = 0, max_label = 0, count = 0, *labelk;

    if(WINDOW_SIZE != X0_len) {
        return 255; // length of trainingset != length of testset
    }
    if(k > M_SIZE) {
        return 255; // to less trainingsets for k neighbors
    }

    labelk = (uint8_t *) calloc(k,sizeof(uint8_t));
    distk  = (float *)   malloc(sizeof(float)*k);
    for(uint8_t i=0;i<k;i++) {
        distk[i]  = INFINITY;
    }

    for (uint8_t i=0; i<M_SIZE; i++) {
        switch(mode) {
            case 1:
                prod1 = multiply(training->M[i], WINDOW_SIZE, X0, X0_len);
                prod2 = multiply(training->M[i], WINDOW_SIZE, training->M[i], WINDOW_SIZE);
                prod3 = multiply(X0, X0_len, X0, X0_len);
                root1 = sqrt(prod2);
                root2 = sqrt(prod3);
                denom = root1 * root2;
                d = 1 - (prod1/denom);
                break;
            case 2:
                vd = (float *) malloc(sizeof(float)*WINDOW_SIZE);
                for(uint8_t j=0; j<WINDOW_SIZE; j++) {
                    vd[j] = training->M[i][j] - X0[j];
                }
                d = sqrt(multiply(vd, WINDOW_SIZE, vd, WINDOW_SIZE));
                free(vd);
                break;
            case 3:
                vd = (float *) malloc(sizeof(float)*WINDOW_SIZE);
                for(uint8_t j=0; j<WINDOW_SIZE; j++) {
                    vd[j] = training->M[i][j] - X0[j];
                }
                d = multiply(vd, WINDOW_SIZE, vd, WINDOW_SIZE);
                free(vd);
                break;
            default:
                return 255; // unknown mode
        }
        for(uint8_t j=0; j<k; j++) {
            if(d < distk[j]) {
                for(uint8_t p=k-1; p>j; p-=1) {
                    distk[p] = distk[p-1];
                    labelk[p] = labelk[p-1];
                }
                distk[j] = d;
                labelk[j] = training->labels[i];
            }
        }
    }

    // find label with most occurence
    for(uint8_t i=0;i<k;i++) {
        count = 0;
        for(uint8_t j=0;j<k;j++) {
            if(labelk[i] == labelk[j])
                count++;
        }
        if(count > max_occ) {
            max_occ = count;
            max_label = labelk[i];
        }
    }

    free(labelk);
    free(distk);

    return max_label;
}

float multiply(float *m1, uint8_t m1_len, float* m2, uint8_t m2_len)
{
    if(m1_len != m2_len)
        return 0;
    float result = 0;
    for(uint8_t i=0; i<m1_len; i++) {
        result += m1[i] * m2[i];
    }
    return result;
}
