#include "utils/utils.h"

int log2_pow2(uint64_t num){
    int ret = 0;
    if(!num) return ret;
    while(num >> ret) ret++;
    return ret - 1;
}