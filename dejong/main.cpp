#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <memory.h>
#include <pthread.h>
#include <thread>
#include <unistd.h>
#include <vector>

// globals-ish - deal with it
#define SCREEN_WIDTH 12000
#define SCREEN_HEIGHT 9600
//#define SCREEN_QUALITY 16384
#define SCREEN_EXPOSURE 1.0f
#define SCREEN_OVERSAMPLE 0.3f

// #define ITERATION_COUNT ((SCREEN_WIDTH * SCREEN_HEIGHT) * SCREEN_QUALITY)
#define ITERATION_COUNT 3##141##592##653##589

// zoom factor : 1.0 = standard, 2.0 = 2x zoom
#define X_ZOOM 1.0f
#define Y_ZOOM 1.0f

#define X_RANGE_MIN (-2.1f / X_ZOOM)
#define X_RANGE_MAX (2.1f / X_ZOOM)

#define Y_RANGE_MIN (-2.1f / Y_ZOOM)
#define Y_RANGE_MAX (2.1f / Y_ZOOM)

#define A  1.37029f
#define B (-5.03617f)
#define C (-1.74762f)
#define D  2.45849f

time_t startTime;
float **Screen;

// lock this when changing iterations
pthread_mutex_t mutexIter;
int64_t iterations = ITERATION_COUNT;
int64_t progress = 0;
#define LOOP_PER_THREAD 10##000##000
/*
#define A -2.25270f
#define B -4.45858f
#define C  2.93583f
#define D  0.34251f
*/

char *commaprint(int64_t n) {
  static int comma = ',';
  static char retbuf[30];
  char *p = &retbuf[sizeof(retbuf)-1];
  int i = 0;

  *p = '\0';

  do {
    if(i%3 == 0 && i != 0) {
      *--p = comma;
    }
    *--p = '0' + n % 10;
    n /= 10;
    i++;
  } while(n != 0);

  return p;
}

float frand(float randmin, float randmax) {
  // determine width of range
  float floatrange = randmax - randmin;
  // get random value
  float retval = (float) rand();
  // normalize to within floatrange
  retval = retval / ((float) RAND_MAX / floatrange);
  // offset by minimum to constrain to randmin <= X <= randmax;
  return  retval + randmin;
}

inline uint64_t scaleToRange(float val, float rMin, float rMax, uint64_t intMax) {
  // total width of range
  float valRange = rMax - rMin;

  // distance from max to val
  float valDist = rMax - val;

  float rangePercent = 1.0 - (valDist / valRange);
  return (uint64_t) ( (float)intMax * rangePercent);
}

void * mycalloc ( size_t num, size_t size ) {
  void *retVal = calloc(num, size);
  if(retVal == nullptr) {
    printf("FAILED\n");
  }
  return retVal;
}

float **allocateFractal(size_t width, size_t height) {
  float **retPointer = (float **)mycalloc(height, sizeof(float *));
  if(retPointer != nullptr) {
    for(size_t i = 0; i < height; i++) {
      retPointer[i] = (float *)mycalloc(width, sizeof(float));
      if(retPointer[i] == nullptr) {
        exit(1);
      }
      for(size_t j = 0; j < width; j++) {
        retPointer[i][j] = 0.0;
      }
    }
  }
  return retPointer;
}

inline void plot(float fx, float fy) {
  // scale x and y (both range +/- 2) to fit screen
  int y = scaleToRange(fy, Y_RANGE_MIN, Y_RANGE_MAX, SCREEN_HEIGHT);
  float *row = Screen[y];
  int x = scaleToRange(fx, X_RANGE_MIN, X_RANGE_MAX, SCREEN_WIDTH);
  row[x] += SCREEN_EXPOSURE;
}

void normalize(float **screen, int rangemax) {
  printf("Finding maxima and zero-weighted average\n");
  float maxima = -1.0;
  double avg_total = 0.0;
  uint64_t avg_count = 0;
  for(int y = 0; y < SCREEN_HEIGHT; y++) {
    float *row = screen[y];
    for(int x = 0; x < SCREEN_WIDTH; x++) {
      float pixel = row[x];
      if(pixel != 0) {
        avg_count ++;
        avg_total += (double)pixel;
        if(pixel > maxima) {
          maxima = pixel;
        }
      }
    }
  }

  float average = (float)(avg_total / (double)avg_count);

  printf("Maxima: %0.2f and average %0.2f over %ld samples\n", maxima, average, avg_count);

  printf("Normalizing 0-%0.2f to screen range 0-%d\n", maxima, rangemax);
  maxima *= SCREEN_OVERSAMPLE;
  maxima = logf(maxima) * logf(maxima);
  float scaleFactor = (float)rangemax / (float)maxima;

  for(int y = 0; y < SCREEN_HEIGHT; y++) {
    float *row = screen[y];
    for(int x = 0; x < SCREEN_WIDTH; x++) {
      if(row[x] > 0) {
        row[x] = logf(row[x]) * logf(row[x]) * scaleFactor;
      }
    }
  }
}

void dumpPPM(float **screen, char *filename) {
  FILE *outfile = fopen(filename, "wb");
  fprintf(outfile, "P6\n%d %d\n255\n", (int)SCREEN_WIDTH, (int)SCREEN_HEIGHT);

  for(int y = 0; y < SCREEN_HEIGHT; y++) {
    float *row = screen[y];
    for(int x = 0; x < SCREEN_WIDTH; x++) {
      int outVal = 255 - (int)row[x];
      if(outVal > 255) {
//        printf("overflow at %d, %d [%d]\n", x, y, outVal);
        outVal = 255;
      }
      if(outVal < 0) {
//        printf("underflow at %d, %d [%d]\n", x, y, outVal);
        outVal = 0;
      }
      fputc(outVal, outfile);
      fputc(outVal, outfile);
      fputc(outVal, outfile);
    }
  }
  fclose(outfile);
}

void iterate(uint64_t loopCount) {
  float oldX = frand(-2.0, 2.0);
  float oldY = frand(-2.0, 2.0);
  float newX, newY;

  for(uint64_t i = 0; i < loopCount; i++) {
    newX = std::sin(A * oldY) - std::cos(B * oldX);
    newY = std::sin(C * oldX) - std::cos(D * oldY);
    plot(newX, newY);

    oldX = std::sin(A * newY) - std::cos(B * newX);
    oldY = std::sin(C * newX) - std::cos(D * newY);
    plot(oldX, oldY);
  }
}

void *threadWorker(void* x) {
  size_t whichThread = (size_t)x;
  // lock iteration mutex
  // atomic get count + decrement by LOOP_PER_THREAD or remaining iterations
  // unlock mutex
  // process count
  // whilte count > 0

  uint64_t count;

  if(LOOP_PER_THREAD > iterations) {
    count = iterations;
  } else {
    count = LOOP_PER_THREAD;
  }

  while(iterations > 0) {
    iterate(count);

    if(whichThread == 0) {
      // output status message
      char *currentTime, *etaTime;
      float donePercent = (float)((double)progress / (double)ITERATION_COUNT);
      time_t nowTime = time(nullptr);
      int diffTime = nowTime - startTime;
      time_t endTime = (time_t)((float)diffTime / (float)donePercent);
      if(endTime < 0) {
        continue;
      }
      endTime += startTime;
      currentTime = strdup(ctime(&nowTime));
      currentTime[strlen(currentTime) - 1] = '\0';
      etaTime = strdup(ctime(&endTime));
      etaTime[strlen(etaTime) - 1] = '\0';
      printf("[%s] %3.2f%% loop %smm ETA [%s]\n", currentTime, donePercent * 100.000f, commaprint(progress/1000000), etaTime);
      free(currentTime);
      free(etaTime);
    }
    pthread_mutex_lock (&mutexIter);
    if(LOOP_PER_THREAD > iterations) {
      count = iterations;
    } else {
      count = LOOP_PER_THREAD;
    }
    iterations -= count;
    progress += count;
    pthread_mutex_unlock (&mutexIter);
  }
  printf("*****thread exiting*****\n");
  pthread_exit((void*)nullptr);
}

int main (int  /*argc*/, char * const  /*argv*/[]) {
  int64_t t;

  unsigned parallelism = std::thread::hardware_concurrency();

  if (parallelism < 1) {
    parallelism = sysconf(_SC_NPROCESSORS_ONLN);
  }

  if (parallelism < 1) {
    parallelism = 2;
  }

  std::vector<pthread_t> threads(parallelism);
  void *threadStatus;

  printf("Detected %d thread parallelism\n", parallelism);

  printf("starting up...");
  Screen = allocateFractal(SCREEN_WIDTH, SCREEN_HEIGHT);
  printf("DONE!\n");

  startTime = time(nullptr);
  printf("Starting %s iterations...\n", commaprint((int64_t)ITERATION_COUNT));

  pthread_mutex_init(&mutexIter, nullptr);
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  for(t = 0; t < parallelism; t++) {
    pthread_create(&threads[t], &attr, threadWorker, (void *)t);
  }
  pthread_attr_destroy(&attr);

  /* Wait on the other threads */
  for(t = 0; t < parallelism; t++) {
    pthread_join(threads[t], &threadStatus);
  }

  normalize(Screen, 255);
  dumpPPM(Screen, (char *)"outfile.ppm");

  // clean up mutex, shut down pthreads
  pthread_mutex_destroy(&mutexIter);
  pthread_exit(nullptr);
  return 0;
}
