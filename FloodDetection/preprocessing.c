#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "gdal.h"
#include "cpl_conv.h"

#define DATE_COUNT 2
#define BAND_COUNT 4
#define WIDTH 512
#define HEIGHT 512

enum { BLUE, GREEN, RED, NIR };

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

const char *dates[DATE_COUNT] = {
"2019-01-09",
"2019-01-14"
};

/* Feature 1: NoData and invalid pixel validation */
int validPixel(float value, double noData, int hasNoData)
{
if (!isfinite(value))
return 0;

if (hasNoData) {
if (isnan(noData))
return 1;

if (value == noData)
return 0;
}

return 1;
}

/* Feature 2: Validate index calculation */
int calculateIndex(float a, float b, double *index)
{
if (!isfinite(a) || !isfinite(b))
return 0;

double denominator = (double)a + (double)b;
double numerator = (double)a - (double)b;

if (!isfinite(denominator) || denominator == 0.0)
return 0;

double result = numerator / denominator;

if (!isfinite(result))
return 0;

*index = result;
return 1;
}

int main(void)
{
GDALAllRegister();

const size_t total = (size_t)WIDTH * HEIGHT;
float *pixels = malloc(total * sizeof(float));

if (pixels == NULL) {
fprintf(stderr, "Memory allocation failed.\n");
return EXIT_FAILURE;
}

for (int d = 0; d < DATE_COUNT; d++) {
printf("\nDate: %s\n", dates[d]);

for (int b = 0; b < BAND_COUNT; b++) {
GDALDatasetH dataset =
GDALOpen(files[d][b], GA_ReadOnly);

if (dataset == NULL) {
fprintf(stderr, "Cannot open: %s\n",
files[d][b]);
free(pixels);
return EXIT_FAILURE;
}

if (GDALGetRasterXSize(dataset) != WIDTH ||
GDALGetRasterYSize(dataset) != HEIGHT ||
GDALGetRasterCount(dataset) != 1) {
fprintf(stderr, "Invalid raster dimensions/bands: %s\n",
files[d][b]);
GDALClose(dataset);
free(pixels);
return EXIT_FAILURE;
}

GDALRasterBandH band = GDALGetRasterBand(dataset, 1);
int hasNoData = 0;
double noData =
GDALGetRasterNoDataValue(band, &hasNoData);

if (GDALRasterIO(
band, GF_Read, 0, 0, WIDTH, HEIGHT,
pixels, WIDTH, HEIGHT, GDT_Float32,
0, 0) != CE_None) {
fprintf(stderr, "Cannot read: %s\n",
files[d][b]);
GDALClose(dataset);
free(pixels);
return EXIT_FAILURE;
}

size_t valid = 0, invalid = 0;

for (size_t i = 0; i < total; i++) {
if (validPixel(pixels[i], noData, hasNoData))
valid++;
else
invalid++;
}

printf("%s: valid=%zu, invalid=%zu\n",
files[d][b], valid, invalid);

GDALClose(dataset);
}
}

/* Demonstrate pixel-value validation for both indices. */
double index;

printf("\nIndex validation examples:\n");

if (calculateIndex(20.0f, 10.0f, &index))
printf("Valid NDWI-style calculation: %.6f\n", index);
else
printf("Invalid NDWI-style calculation\n");

if (calculateIndex(10.0f, -10.0f, &index))
printf("Valid index calculation: %.6f\n", index);
else
printf("Invalid index: denominator is zero\n");

printf("\nPreprocessing demonstration completed.\n");

free(pixels);
GDALDestroyDriverManager();

return EXIT_SUCCESS;
}