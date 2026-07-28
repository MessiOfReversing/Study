#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define TRAIN_SAMPLES 4

void main()
{
  double X[TRAIN_SAMPLES][2] =
  {
      0, 0,
      0, 1, 
      1, 0, 
      1, 1
  };
  
  double Y[TRAIN_SAMPLES] =
  {
      0, 0, 0, 1
  };

  double w[3];

  for(int i = 0l i < 3; i++)
  {
    w[i] = ((double)rand() / RAND_MAX) * 0.5 - 0.25;
  }

  unsigned int epoch = 0;
  unsigned int MAX_EPOCH = 500;
  double ETHA = 0.05;

  double output;
  double x1, x2;
  double target;

  while(epoch++ < MAX_EPOCH)
  {
   double deltaW[3];

    deltaW[0] = 0.0;
    deltaW[1] = 0.0;
    deltaW[2] = 0.0;

    for(int i = 0; i < TRAIN_SAMPLES; i++)
    {
      x1 = x[i][0];
      x2 = x[i][1];
      target = Y[i];

      yhat = 1 * W[0] + x1 * W[1] + x2 * W[2];

      deltaW[0] += (target - yhat) * 1;
      deltaW[1] += (target - yhat) * x1;
      deltaW[2] += (target - yhat) * x2;
    }

    W[0] = W[0] + Etha * (deltaW[0] / TRAIN_SAMPLES);
    W[1] = W[1] + Etha * (deltaW[1] / TRAIN_SAMPLES);
    W[2] = W[2] + Etha * (deltaW[2] / TRAIN_SAMPLES);

    double loss;
    cost = 0.0;

    for(int i = 0; i < TRAIN_SAMPLES; i++)
    {
      x1 = X[i][0];
      x2 = X[i][1];
      target = Y[i];

      yhat = 1 * W[0] + x1 * W[1] + x2 * W[2];

      loss += (target - yhat) * (target - yhat);
    }

    loss = 0.5 * loss / TRAIN_SAMPLES;

    printf("%05d: loss = %10.9lf \n", epoch, loss);
  }
  printf("training done\n");
}
