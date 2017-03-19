//
// Created by kirill on 24.10.16.
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "sp_mat.h"

const char pathInput[]  = "../../../../../../initial/INPUT.txt";
const char pathResult[] = "../../../../../../result/Kirill/RungeKutt.txt";

int init(double *, double *, double *, double *, double *, double *, int *, TYPE **);
void createSpMat(spMatrix *, TYPE, TYPE);
void final(TYPE *, const char *path);

size_t nX;

int main() {
  double xStart, xEnd;
  double sigma;
  double tStart, tFinal;
  double dt;
  int check;

  TYPE* U;


  //------------------------------------------------------------------------
  //                       Инициализация данных
  //------------------------------------------------------------------------

  if (init(&xStart, &xEnd, &sigma, &tStart, &tFinal, &dt, &check, &U) )
    return -1;

  double step = fabs(xStart - xEnd) / nX;
  size_t sizeTime = (size_t)((tFinal - tStart) / dt);
  if (2 * sigma * dt > step * step) {
    printf("Выбор шага по времени не возможен в силу условия устойчивости!\n");
    printf("%.10lf > %.10lf\n", 2 * sigma * dt, step * step);
    printf("Предлагаю взять dt = %.10lf\n", step * step / (2.0 * sigma));

    return -1;
  }

  #if ENABLE_PARALLEL
    printf("ПАРАЛЛЕЛЬНАЯ ВЕРСИЯ!\n");
  #endif
  printf("TIMESIZE = %lu; NX = %lu\n", sizeTime, nX);

  //------------------------------------------------------------------------
  //          Заполнение значений и номера столбцов матрицы
  //------------------------------------------------------------------------

  spMatrix A;
  double coeff1 = 1.0/(step*step);
  double coeff2 = -2.0*coeff1;
  createSpMat(&A, coeff1, coeff2);

  spMatrix B;
  coeff1 = dt*coeff1*0.5;
  coeff2 = 1.0 - 2.0*coeff1;
  createSpMat(&B, coeff1, coeff2);

  spMatrix C;
  coeff1 = coeff1*2.0;
  coeff2 = 1.0 - 2.0*coeff1;
  createSpMat(&C, coeff1, coeff2);

  // -----------------------------------------------------------------------
  //                         Вычисления
  //------------------------------------------------------------------------

  TYPE* UNext = (TYPE*)malloc(sizeof(TYPE) * (nX + 2));
  TYPE* k1 = (TYPE*)malloc(sizeof(TYPE) * (nX + 2));
  TYPE* k2 = (TYPE*)malloc(sizeof(TYPE) * (nX + 2));
  TYPE* k3 = (TYPE*)malloc(sizeof(TYPE) * (nX + 2));
  TYPE* k4 = (TYPE*)malloc(sizeof(TYPE) * (nX + 2));

  TYPE* tmp;

  double h = dt/6.0;

  double t0 = omp_get_wtime();
  for (int i = 1; i <= sizeTime; i++) {

    // k1 = A*U
    multMV(&k1, A, U);

    // k2 = B*k1
    multMV(&k2, B, k1);

    // k3 = B*k2
    multMV(&k3, B, k2);

    // k4 = C*k3
    multMV(&k4, C, k3);

    // UNext = U + (k1 + k2*2 + k3*2 + k4)*h;
    sumV(nX + 2, h, &UNext, U, k1, k2, k3, k4);

    tmp = U;
    U = UNext;
    UNext = tmp;
  }
  double t1 = omp_get_wtime();
  printf("\nfinish!\n\n");

  //------------------------------------------------------------------------
  //                       Вывод результатов и чистка памяти
  //------------------------------------------------------------------------

  double diffTime = t1 - t0;
  double gflop = (2*3*nX*4 + 7*(nX+2))*sizeTime*1.0/1000000000.0;
  printf("Time\t%.15lf\n", diffTime);
  printf("GFlop\t%.lf\n", gflop);
  printf("GFlop's\t%.15lf\n", gflop*1.0/diffTime);

  final(U, pathResult);

  free(U);
  free(UNext);
  free(k1);
  free(k2);
  free(k3);
  free(k4);

  freeSpMat(&A);
  freeSpMat(&B);
  freeSpMat(&C);

  return 0;

}

/*
____________________________________________________________________________

                          РЕАЛИЗАЦИЯ ФУНКЦИЙ
 ____________________________________________________________________________

*/


int init(double *xStart, double *xEnd, double *sigma, double *tStart, double *tFinal, double *dt, int *check, TYPE **U) {
  FILE *fp;
  if ((fp = fopen(pathInput, "r")) == NULL) {
    printf("Не могу найти файл!\n");
    return -2;
  };

  if ( !fscanf(fp, "XSTART=%lf\n", xStart) )
    return -1;
  if ( !fscanf(fp, "XEND=%lf\n", xEnd) )
    return -1;
  if ( !fscanf(fp, "SIGMA=%lf\n", sigma) )
    return -1;
  if ( !fscanf(fp, "NX=%lu\n", &nX) )
    return -1;
  if ( !fscanf(fp, "TSTART=%lf\n", tStart) )
    return -1;
  if ( !fscanf(fp, "TFINISH=%lf\n", tFinal) )
    return -1;
  if ( !fscanf(fp, "dt=%lf\n", dt) )
    return -1;
  if ( !fscanf(fp, "BC=%d\n", check) )
    return -1;

  *U = (TYPE*)malloc(sizeof(TYPE) * (nX + 2));

  // Заполнение функции в нулевой момент времени
  for (int i = 1; i < nX - 1; i++)
    if ( !fscanf(fp, "%lf", &(*U)[i]) )
      return -1;
  (*U)[0] = (*U)[nX + 1] = 0.0;
  fclose(fp);

  return 0;
}

void createSpMat(spMatrix *mat, TYPE coeff1, TYPE coeff2) {

  initSpMat(mat, (nX+2)*3, nX + 2);

  int j = 0;
  mat->value[0] = coeff1;  mat->col[0] = 0;
  mat->value[1] = coeff2; mat->col[1] = 1;
  mat->value[2] = coeff1;  mat->col[2] = 2;
  for (int i = 3; i < (nX+2)*3 - 3; i += 3) {
    mat->value[i] = coeff1;       mat->col[i] = j++;
    mat->value[i + 1] = coeff2;  mat->col[i + 1] = j++;
    mat->value[i + 2] = coeff1;   mat->col[i + 2] = j--;
  }
  mat->value[(nX+2)*3 - 3] = coeff1;  mat->col[(nX+2)*3 - 3] = (int)nX - 1;
  mat->value[(nX+2)*3 - 2] = coeff2; mat->col[(nX+2)*3 - 2] = (int)nX;
  mat->value[(nX+2)*3 - 1] = coeff1;  mat->col[(nX+2)*3 - 1] = (int)nX + 1;

  mat->rowIndex[0] = 0;
  for (int i = 1; i < nX + 2; i++) {
    mat->rowIndex[i] = mat->rowIndex[i - 1] + 3;
  }
  mat->rowIndex[nX + 2] = mat->rowIndex[nX + 1] + 3;
}

void final(TYPE *UFin, const char *path) {
  FILE *fp;
  fp = fopen(path, "w");

  for (int i = 1; i < nX + 1; i++)
    fprintf(fp, "%.15le\n", UFin[i]);

  fclose(fp);
}
