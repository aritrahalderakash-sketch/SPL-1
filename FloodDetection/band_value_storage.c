#include <stdio.h>
#include <stdlib.h>
#include <gdal.h>
#include <cpl_conv.h>

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
"B02 Blue", "B03 Green", "B04 Red", "B08 NIR"
};

const char *filePaths[DATE_COUNT][BAND_COUNT] = {
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

float *bandData[DATE_COUNT][BAND_COUNT] = {{NULL}};
GDALDatasetH datasets[DATE_COUNT][BAND_COUNT] = {{NULL}};

int success = 0;

for (int d = 0; d < DATE_COUNT; d++) {
for (int b = 0; b < BAND_COUNT; b++) {
datasets[d][b] = GDALOpen(filePaths[d][b], GA_ReadOnly);

if (datasets[d][b] == NULL) {
fprintf(stderr, "Cannot open: %s\n",
filePaths[d][b]);
goto cleanup;
}

if (GDALGetRasterXSize(datasets[d][b]) != WIDTH ||
GDALGetRasterYSize(datasets[d][b]) != HEIGHT ||
GDALGetRasterCount(datasets[d][b]) != 1) {
fprintf(stderr, "Invalid raster: %s\n",
filePaths[d][b]);
goto cleanup;
}

bandData[d][b] =
malloc(totalPixels * sizeof(float));

if (bandData[d][b] == NULL) {
fprintf(stderr, "Memory allocation failed.\n");
goto cleanup;
}

GDALRasterBandH band =
GDALGetRasterBand(datasets[d][b], 1);

if (GDALRasterIO(
band, GF_Read, 0, 0, WIDTH, HEIGHT,
bandData[d][b], WIDTH, HEIGHT,
GDT_Float32, 0, 0) != CE_None) {
fprintf(stderr, "Cannot read: %s\n",
filePaths[d][b]);
goto cleanup;
}

printf("Stored %s for %s: %zu pixel values\n",
bandNames[b], dates[d], totalPixels);
}
}

printf("\nSample pixel (row 0, column 0):\n");

for (int d = 0; d < DATE_COUNT; d++) {
printf("\nDate: %s\n", dates[d]);

for (int b = 0; b < BAND_COUNT; b++) {
printf("%s = %.2f\n",
bandNames[b], bandData[d][b][0]);
}
}

printf("\nSUCCESS: All band pixel arrays stored in memory.\n");
success = 1;

cleanup:
for (int d = 0; d < DATE_COUNT; d++) {
for (int b = 0; b < BAND_COUNT; b++) {
free(bandData[d][b]);

if (datasets[d][b] != NULL)
GDALClose(datasets[d][b]);
}
}

return success ? EXIT_SUCCESS : EXIT_FAILURE;
}