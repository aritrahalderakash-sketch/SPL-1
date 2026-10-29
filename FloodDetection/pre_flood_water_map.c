#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "gdal.h"
#include "cpl_conv.h"

#define WIDTH 512
#define HEIGHT 512
#define WATER_THRESHOLD 0.0

int main(void)
{
GDALAllRegister();

const char *greenPath = "dataset/S2_2019-01-09_B03.tif";
const char *nirPath = "dataset/S2_2019-01-09_B08.tif";
const char *outputPath = "outputs/2019-01-09_water_map.tif";

GDALDatasetH greenDS = NULL;
GDALDatasetH nirDS = NULL;
GDALDatasetH outputDS = NULL;

float *green = NULL;
float *nir = NULL;
unsigned char *waterMap = NULL;

int success = EXIT_FAILURE;

greenDS = GDALOpen(greenPath, GA_ReadOnly);
nirDS = GDALOpen(nirPath, GA_ReadOnly);

if (greenDS == NULL || nirDS == NULL) {
fprintf(stderr, "ERROR: Cannot open input images.\n");
goto cleanup;
}

if (GDALGetRasterXSize(greenDS) != WIDTH ||
GDALGetRasterYSize(greenDS) != HEIGHT ||
GDALGetRasterXSize(nirDS) != WIDTH ||
GDALGetRasterYSize(nirDS) != HEIGHT) {
fprintf(stderr, "ERROR: Unexpected image dimensions.\n");
goto cleanup;
}

const size_t totalPixels = (size_t)WIDTH * HEIGHT;

green = malloc(totalPixels * sizeof(float));
nir = malloc(totalPixels * sizeof(float));
waterMap = malloc(totalPixels * sizeof(unsigned char));

if (green == NULL || nir == NULL || waterMap == NULL) {
fprintf(stderr, "ERROR: Memory allocation failed.\n");
goto cleanup;
}

if (GDALRasterIO(
GDALGetRasterBand(greenDS, 1), GF_Read,
0, 0, WIDTH, HEIGHT, green,
WIDTH, HEIGHT, GDT_Float32, 0, 0) != CE_None ||
GDALRasterIO(
GDALGetRasterBand(nirDS, 1), GF_Read,
0, 0, WIDTH, HEIGHT, nir,
WIDTH, HEIGHT, GDT_Float32, 0, 0) != CE_None) {
fprintf(stderr, "ERROR: Cannot read pixel values.\n");
goto cleanup;
}

int greenHasNoData = 0, nirHasNoData = 0;

double greenNoData = GDALGetRasterNoDataValue(
GDALGetRasterBand(greenDS, 1), &greenHasNoData);

double nirNoData = GDALGetRasterNoDataValue(
GDALGetRasterBand(nirDS, 1), &nirHasNoData);

size_t waterCount = 0;
size_t nonWaterCount = 0;
size_t invalidCount = 0;

for (size_t i = 0; i < totalPixels; i++) {
int valid = isfinite(green[i]) && isfinite(nir[i]);

if (greenHasNoData && !isnan(greenNoData) &&
green[i] == (float)greenNoData)
valid = 0;

if (nirHasNoData && !isnan(nirNoData) &&
nir[i] == (float)nirNoData)
valid = 0;

double denominator = (double)green[i] + nir[i];

if (!valid || !isfinite(denominator) ||
denominator == 0.0) {
waterMap[i] = 255;
invalidCount++;
continue;
}

double ndwi = ((double)green[i] - nir[i]) / denominator;

if (!isfinite(ndwi)) {
waterMap[i] = 255;
invalidCount++;
} else if (ndwi > WATER_THRESHOLD) {
waterMap[i] = 1;
waterCount++;
} else {
waterMap[i] = 0;
nonWaterCount++;
}
}

GDALDriverH driver = GDALGetDriverByName("GTiff");

if (driver == NULL) {
fprintf(stderr, "ERROR: GeoTIFF driver unavailable.\n");
goto cleanup;
}

outputDS = GDALCreate(
driver, outputPath, WIDTH, HEIGHT,
1, GDT_Byte, NULL);

if (outputDS == NULL) {
fprintf(stderr,
"ERROR: Cannot create output. Check that outputs/ exists.\n");
goto cleanup;
}

double transform[6];

if (GDALGetGeoTransform(greenDS, transform) == CE_None)
GDALSetGeoTransform(outputDS, transform);

GDALSetProjection(
outputDS, GDALGetProjectionRef(greenDS));

GDALRasterBandH outputBand = GDALGetRasterBand(outputDS, 1);
GDALSetRasterNoDataValue(outputBand, 255);

if (GDALRasterIO(
outputBand, GF_Write,
0, 0, WIDTH, HEIGHT, waterMap,
WIDTH, HEIGHT, GDT_Byte, 0, 0) != CE_None) {
fprintf(stderr, "ERROR: Cannot write water map.\n");
goto cleanup;
}

GDALFlushCache(outputDS);

printf("\nJanuary 9 Water Map created successfully!\n");
printf("NDWI threshold: %.2f\n", WATER_THRESHOLD);
printf("Water pixels: %zu\n", waterCount);
printf("Non-water pixels: %zu\n", nonWaterCount);
printf("Invalid pixels: %zu\n", invalidCount);
printf("Output: %s\n", outputPath);
printf("Pixel values: 1=water, 0=non-water, 255=NoData\n");

success = EXIT_SUCCESS;

cleanup:
if (outputDS != NULL) GDALClose(outputDS);
if (greenDS != NULL) GDALClose(greenDS);
if (nirDS != NULL) GDALClose(nirDS);

free(green);
free(nir);
free(waterMap);

return success;
}