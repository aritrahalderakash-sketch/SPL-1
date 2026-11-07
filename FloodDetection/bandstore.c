
#include <stdio.h>
#include <stdlib.h>
#include "gdal.h"
#include "cpl_conv.h"

#define DATE_COUNT 2
#define BAND_COUNT 4
#define WIDTH 512
#define HEIGHT 512

enum { BLUE, GREEN, RED, NIR };

const char *dates[DATE_COUNT] = {
"2019-01-09",
"2019-01-14"
};

const char *bandNames[BAND_COUNT] = {
"B02 (Blue)",
"B03 (Green)",
"B04 (Red)",
"B08 (NIR)"
};

const char *files[DATE_COUNT][BAND_COUNT] = {
{
"dataset/S2_2019-01-09_B02.tif",
"dataset/S2_2019-01-09_B03.tif",
"dataset/S2_2019-01-09_B04.tif",
"dataset/S2_2019-01-09_B08.tif"
},
{
"dataset/S2_2019-01-14_B02.tif",
"dataset/S2_2019-01-14_B03.tif",
"dataset/S2_2019-01-14_B04.tif",
"dataset/S2_2019-01-14_B08.tif"
}
};

int main(void)
{
GDALAllRegister();

const size_t totalPixels = (size_t)WIDTH * HEIGHT;

float *bandData[DATE_COUNT][BAND_COUNT] = {NULL};

for (int d = 0; d < DATE_COUNT; d++) {

printf("\nReading date: %s\n", dates[d]);

for (int b = 0; b < BAND_COUNT; b++) {

GDALDatasetH dataset =
GDALOpen(files[d][b], GA_ReadOnly);

if (dataset == NULL) {
fprintf(stderr, "Cannot open: %s\n", files[d][b]);
goto cleanup_error;
}

if (GDALGetRasterXSize(dataset) != WIDTH ||
GDALGetRasterYSize(dataset) != HEIGHT ||
GDALGetRasterCount(dataset) != 1) {

fprintf(stderr, "Invalid raster: %s\n", files[d][b]);
GDALClose(dataset);
goto cleanup_error;
}

bandData[d][b] =
malloc(totalPixels * sizeof(float));

if (bandData[d][b] == NULL) {
fprintf(stderr, "Memory allocation failed.\n");
GDALClose(dataset);
goto cleanup_error;
}

GDALRasterBandH band =
GDALGetRasterBand(dataset, 1);

if (GDALRasterIO(
band, GF_Read,
0, 0, WIDTH, HEIGHT,
bandData[d][b],
WIDTH, HEIGHT,
GDT_Float32, 0, 0) != CE_None) {

fprintf(stderr, "Cannot read pixels: %s\n",
files[d][b]);

GDALClose(dataset);
goto cleanup_error;
}

printf("%s stored successfully.\n", bandNames[b]);

GDALClose(dataset);
}
}

printf("\nAll 8 bands stored successfully in memory.\n");

printf("Total pixel values stored: %zu\n",
totalPixels * DATE_COUNT * BAND_COUNT);

int row = 0;
int col = 0;

size_t sampleIndex = (size_t)row * WIDTH + col;

printf("\nSample Pixel Values:\n");
printf("Row: %d, Column: %d\n", row, col);

for (int d = 0; d < DATE_COUNT; d++) {

printf("\nDate: %s\n", dates[d]);

for (int b = 0; b < BAND_COUNT; b++) {

printf("%s = %.2f\n",
bandNames[b],
bandData[d][b][sampleIndex]);
}
}

for (int d = 0; d < DATE_COUNT; d++) {
for (int b = 0; b < BAND_COUNT; b++) {
free(bandData[d][b]);
}
}

GDALDestroyDriverManager();

return EXIT_SUCCESS;

cleanup_error:

for (int d = 0; d < DATE_COUNT; d++) {
for (int b = 0; b < BAND_COUNT; b++) {
free(bandData[d][b]);
}
}

GDALDestroyDriverManager();

return EXIT_FAILURE;
}