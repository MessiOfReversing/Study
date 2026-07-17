#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>

#if defined _WIN32
    #include "win.h"
#else 
    #include <unistd.h>
    #include <sys/mman.h>
#endif

typedef struct {
    int dim;
    int hidden_dim
}
