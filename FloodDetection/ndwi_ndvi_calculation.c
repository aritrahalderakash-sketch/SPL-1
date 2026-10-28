#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gdal.h>
#include <cpl_conv.h>

#define DATE_COUNT 2
#define WIDTH 512
#define HEIGHT 512

enum { GREEN, RED, NIR };

const char *dates[DATE_COUNT] = {
"2019-01-09",
"2019-01-14"
};

const char *filePaths[DATE_COUNT][3] = {
{
"dataset/S2_2019-01-09_B03.tif",
"dataset/S2_2019-01-09_B04.tif",
"dataset/S2_2019-01-09_B08.tif"
},
{
"dataset/S2_2019-01-14_B03.tif",
"dataset/S2_2019-01-14_B04.tif",
"dataset/S2_2019-01-14_B08.tif"
}
};

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

int main(void)
{
GDALAllRegister();

const size_t totalPixels = (size_t)WIDTH * HEIGHT;

float *bandData[DATE_COUNT][3] = {{NULL}};
unsigned char *ndwiValid[DATE_COUNT] = {NULL};
unsigned char *ndviValid[DATE_COUNT] = {NULL};

float *ndwiData[DATE_COUNT] = {NULL};
float *ndviData[DATE_COUNT] = {NULL};

GDALDatasetH datasets[DATE_COUNT][3] = {{NULL}};

int success = 0;

for (int d = 0; d < DATE_COUNT; d++) {
for (int b = 0; b < 3; b++) {
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

bandData[d][b] = malloc(totalPixels * sizeof(float));

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
}

ndwiData[d] = malloc(totalPixels * sizeof(float));
ndviData[d] = malloc(totalPixels * sizeof(float));

ndwiValid[d] = calloc(totalPixels, sizeof(unsigned char));
ndviValid[d] = calloc(totalPixels, sizeof(unsigned char));

if (ndwiData[d] == NULL || ndviData[d] == NULL ||
ndwiValid[d] == NULL || ndviValid[d] == NULL) {
fprintf(stderr, "Index memory allocation failed.\n");
goto cleanup;
}

int hasNoData[3] = {0};
double noData[3] = {0};

for (int b = 0; b < 3; b++) {
GDALRasterBandH band =
GDALGetRasterBand(datasets[d][b], 1);

noData[b] =
GDALGetRasterNoDataValue(band, &hasNoData[b]);
}

size_t validNDWI = 0, invalidNDWI = 0;
size_t validNDVI = 0, invalidNDVI = 0;

for (size_t i = 0; i < totalPixels; i++) {
float green = bandData[d][GREEN][i];
float red = bandData[d][RED][i];
float nir = bandData[d][NIR][i];

double denominator;
double result;

/* NDWI = (Green - NIR) / (Green + NIR) */
if (validPixel(green, noData[GREEN], hasNoData[GREEN]) &&
validPixel(nir, noData[NIR], hasNoData[NIR])) {

denominator = (double)green + nir;

if (isfinite(denominator) && denominator != 0.0) {
result = ((double)green - nir) / denominator;

if (isfinite(result) && fabs(result) <= 1.0) {
ndwiData[d][i] = (float)result;
ndwiValid[d][i] = 1;
validNDWI++;
} else {
ndwiData[d][i] = NAN;
invalidNDWI++;
}
} else {
ndwiData[d][i] = NAN;
invalidNDWI++;
}
} else {
ndwiData[d][i] = NAN;
invalidNDWI++;
}

/* NDVI = (NIR - Red) / (NIR + Red) */
if (validPixel(nir, noData[NIR], hasNoData[NIR]) &&
validPixel(red, noData[RED], hasNoData[RED])) {

denominator = (double)nir + red;

if (isfinite(denominator) && denominator != 0.0) {
result = ((double)nir - red) / denominator;

if (isfinite(result) && fabs(result) <= 1.0) {
ndviData[d][i] = (float)result;
ndviValid[d][i] = 1;
validNDVI++;
} else {
ndviData[d][i] = NAN;
invalidNDVI++;
}
} else {
ndviData[d][i] = NAN;
invalidNDVI++;
}
} else {
ndviData[d][i] = NAN;
invalidNDVI++;
}
}

printf("\nDate: %s\n", dates[d]);
printf("NDWI valid: %zu, invalid: %zu\n",
validNDWI, invalidNDWI);
printf("NDVI valid: %zu, invalid: %zu\n",
validNDVI, invalidNDVI);

if (ndwiValid[d][0])
printf("Sample NDWI: %.6f\n", ndwiData[d][0]);
else
printf("Sample NDWI: INVALID\n");

if (ndviValid[d][0])
printf("Sample NDVI: %.6f\n", ndviData[d][0]);
else
printf("Sample NDVI: INVALID\n");
}

printf("\nSUCCESS: NDWI and NDVI calculations completed.\n");
success = 1;

cleanup:
for (int d = 0; d < DATE_COUNT; d++) {
free(ndwiData[d]);
free(ndviData[d]);
free(ndwiValid[d]);
free(ndviValid[d]);

for (int b = 0; b < 3; b++) {
free(bandData[d][b]);

if (datasets[d][b] != NULL)
GDALClose(datasets[d][b]);
}
}

return success ? EXIT_SUCCESS : EXIT_FAILURE;
}