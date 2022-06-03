#ifndef _KNN_HEADER_
#define _KNN_HEADER_

#ifdef __cplusplus
extern "C" {
#endif
#define M_SIZE      3
#define WINDOW_SIZE 75

    typedef struct training_data {
        uint8_t labels[M_SIZE];
        float M[M_SIZE][WINDOW_SIZE];
    } t_training_data;

    extern t_training_data training_data;

    float multiply(float *, uint8_t, float*, uint8_t);
    uint8_t knn(t_training_data*, uint8_t, float *, uint8_t, uint8_t);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
