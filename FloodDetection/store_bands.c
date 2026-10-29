


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <float.h>
#include "gdal.h"
#include "cpl_conv.h"

#define DATE_COUNT 2
#define BAND_COUNT 4
#define EXPECTED_WIDTH 512
#define EXPECTED_HEIGHT 512
#define GEO_TOLERANCE 1e-8

enum
{
BLUE,
GREEN,
RED,
NIR
};

const char *dates[DATE_COUNT] =
{
"2019-01-09",
"2019-01-14"
};

const char *bandNames[BAND_COUNT] =
{
"B02 (Blue)",
"B03 (Green)",
"B04 (Red)",
"B08 (NIR)"
};

const char *filePaths[DATE_COUNT][BAND_COUNT] =
{
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

int closeEnough(double a, double b)
{
if (!isfinite(a) || !isfinite(b))
return 0;

return fabs(a - b) <= GEO_TOLERANCE;
}

int sameProjection(
GDALDatasetH reference,
GDALDatasetH current)
{
const char *referenceProjection =
GDALGetProjectionRef(reference);

const char *currentProjection =
GDALGetProjectionRef(current);

if (referenceProjection == NULL ||
currentProjection == NULL)
return 0;

if (referenceProjection[0] == '\0' ||
currentProjection[0] == '\0')
return 0;

return strcmp(referenceProjection,
currentProjection) == 0;
}

int readTransform(
GDALDatasetH dataset,
double transform[6])
{
if (GDALGetGeoTransform(dataset, transform) != CE_None)
return 0;

for (int i = 0; i < 6; i++)
{
if (!isfinite(transform[i]))
return 0;
}

double determinant =
transform[1] * transform[5] -
transform[2] * transform[4];

if (!isfinite(determinant) ||
fabs(determinant) < 1e-15)
return 0;

return 1;
}

int sameTransform(
const double reference[6],
const double current[6])
{
for (int i = 0; i < 6; i++)
{
if (!closeEnough(reference[i], current[i]))
return 0;
}

return 1;
}

void calculateCorner(
const double transform[6],
double pixel,
double line,
double *x,
double *y)
{
*x = transform[0] +
pixel * transform[1] +
line * transform[2];

*y = transform[3] +
pixel * transform[4] +
line * transform[5];
}

int sameExtent(
const double referenceTransform[6],
const double currentTransform[6],
int width,
int height)
{
double pixelCorners[4] =
{
0.0, (double)width, (double)width, 0.0
};

double lineCorners[4] =
{
0.0, 0.0, (double)height, (double)height
};

for (int i = 0; i < 4; i++)
{
double referenceX;
double referenceY;
double currentX;
double currentY;

calculateCorner(
referenceTransform,
pixelCorners[i],
lineCorners[i],
&referenceX,
&referenceY);

calculateCorner(
currentTransform,
pixelCorners[i],
lineCorners[i],
&currentX,
&currentY);

if (!closeEnough(referenceX, currentX) ||
!closeEnough(referenceY, currentY))
return 0;
}

return 1;
}

int validateDimensions(
GDALDatasetH dataset,
const char *path)
{
int width = GDALGetRasterXSize(dataset);
int height = GDALGetRasterYSize(dataset);

if (width != EXPECTED_WIDTH ||
height != EXPECTED_HEIGHT)
{
fprintf(stderr,
"INVALID: Expected %dx%d: %s "
"(actual %dx%d)\n",
EXPECTED_WIDTH,
EXPECTED_HEIGHT,
path,
width,
height);

return 0;
}

return 1;
}

int validateBandCount(
GDALDatasetH dataset,
const char *path)
{
if (GDALGetRasterCount(dataset) != 1)
{
fprintf(stderr,
"INVALID: Expected one raster band: %s\n",
path);

return 0;
}

return 1;
}

int validateMetadata(
GDALDatasetH reference,
GDALDatasetH current,
const double referenceTransform[6],
const char *path,
int width,
int height)
{
double currentTransform[6];

if (!sameProjection(reference, current))
{
fprintf(stderr,
"INVALID: CRS mismatch: %s\n",
path);

return 0;
}

if (!readTransform(current, currentTransform))
{
fprintf(stderr,
"INVALID: Missing or invalid geotransform: %s\n",
path);

return 0;
}

if (!sameTransform(
referenceTransform,
currentTransform))
{
fprintf(stderr,
"INVALID: Geotransform mismatch: %s\n",
path);

return 0;
}

if (!sameExtent(
referenceTransform,
currentTransform,
width,
height))
{
fprintf(stderr,
"INVALID: Geographic extent mismatch: %s\n",
path);

return 0;
}

return 1;
}

int readBandPixels(
GDALDatasetH dataset,
float *buffer,
int width,
int height,
const char *path)
{
GDALRasterBandH rasterBand =
GDALGetRasterBand(dataset, 1);

if (rasterBand == NULL)
{
fprintf(stderr,
"INVALID: Cannot access raster band: %s\n",
path);

return 0;
}

CPLErr result = GDALRasterIO(
rasterBand,
GF_Read,
0,
0,
width,
height,
buffer,
width,
height,
GDT_Float32,
0,
0);

if (result != CE_None)
{
fprintf(stderr,
"INVALID: Cannot read pixel data: %s\n",
path);

return 0;
}

return 1;
}

int getNoDataInformation(
GDALDatasetH dataset,
double *noDataValue,
int *hasNoData,
const char *path)
{
GDALRasterBandH rasterBand =
GDALGetRasterBand(dataset, 1);

if (rasterBand == NULL)
{
fprintf(stderr,
"INVALID: Cannot access band metadata: %s\n",
path);

return 0;
}

int success = 0;

*noDataValue =
GDALGetRasterNoDataValue(rasterBand, &success);

*hasNoData = success;

if (!success)
{
printf("NoData metadata not defined: %s\n", path);
}
else
{
printf("NoData value for %s: ", path);

if (isnan(*noDataValue))
printf("NaN\n");
else
printf("%.12g\n", *noDataValue);
}

return 1;
}

int isValidSample(
float value,
double noDataValue,
int hasNoData)
{
if (!isfinite(value))
return 0;

if (hasNoData)
{
if (isnan(noDataValue))
return 1;

if (value == (float)noDataValue)
return 0;
}

return 1;
}

int calculateIndex(
float first,
float second,
double *result)
{
if (!isfinite(first) || !isfinite(second))
return 0;

double denominator =
(double)first + (double)second;

if (!isfinite(denominator) || denominator == 0.0)
return 0;

double numerator =
(double)first - (double)second;

double value = numerator / denominator;

if (!isfinite(value) || fabs(value) > FLT_MAX)
return 0;

*result = value;

return 1;
}
int createWaterMap(
GDALDatasetH referenceDataset,
const float *ndwiData,
const unsigned char *ndwiValid,
size_t totalPixels,
int width,
int height,
const char *outputPath,
double threshold)
{
GDALDriverH driver = GDALGetDriverByName("GTiff");

if (driver == NULL) {
fprintf(stderr, "ERROR: GeoTIFF driver unavailable.\n");
return 0;
}

unsigned char *waterMap =
malloc(totalPixels * sizeof(unsigned char));

if (waterMap == NULL) {
fprintf(stderr, "ERROR: Water-map allocation failed.\n");
return 0;
}

size_t waterCount = 0;
size_t nonWaterCount = 0;
size_t invalidCount = 0;

for (size_t i = 0; i < totalPixels; i++) {
if (!ndwiValid[i] || !isfinite(ndwiData[i])) {
waterMap[i] = 255;
invalidCount++;
} else if (ndwiData[i] > threshold) {
waterMap[i] = 1;
waterCount++;
} else {
waterMap[i] = 0;
nonWaterCount++;
}
}

GDALDatasetH outputDataset = GDALCreate(
driver,
outputPath,
width,
height,
1,
GDT_Byte,
NULL
);

if (outputDataset == NULL) {
fprintf(stderr,
"ERROR: Cannot create water-map output: %s\n",
outputPath);
free(waterMap);
return 0;
}

double transform[6];

if (GDALGetGeoTransform(referenceDataset, transform) != CE_None ||
GDALSetGeoTransform(outputDataset, transform) != CE_None) {
fprintf(stderr, "ERROR: Cannot preserve geotransform.\n");
GDALClose(outputDataset);
free(waterMap);
return 0;
}

const char *projection =
GDALGetProjectionRef(referenceDataset);

if (projection == NULL || projection[0] == '\0' ||
GDALSetProjection(outputDataset, projection) != CE_None) {
fprintf(stderr, "ERROR: Cannot preserve CRS.\n");
GDALClose(outputDataset);
free(waterMap);
return 0;
}

GDALRasterBandH outputBand =
GDALGetRasterBand(outputDataset, 1);

GDALSetRasterNoDataValue(outputBand, 255);

CPLErr result = GDALRasterIO(
outputBand,
GF_Write,
0, 0,
width, height,
waterMap,
width, height,
GDT_Byte,
0, 0
);

GDALClose(outputDataset);
free(waterMap);

if (result != CE_None) {
fprintf(stderr, "ERROR: Failed to write water-map pixels.\n");
return 0;
}

printf("\nWater map created: %s\n", outputPath);
printf("NDWI threshold: %.2f\n", threshold);
printf("Water pixels: %zu\n", waterCount);
printf("Non-water pixels: %zu\n", nonWaterCount);
printf("Invalid pixels: %zu\n", invalidCount);

return 1;
}
int main(void)
{
GDALAllRegister();

GDALDatasetH datasets[DATE_COUNT][BAND_COUNT] = {{NULL}};

float *bandData[DATE_COUNT][BAND_COUNT] = {{NULL}};

unsigned char *bandValid[DATE_COUNT][BAND_COUNT] = {{NULL}};

double noDataValues[DATE_COUNT][BAND_COUNT] = {{0}};
int hasNoData[DATE_COUNT][BAND_COUNT] = {{0}};

float *ndwiData[DATE_COUNT] = {NULL};
float *ndviData[DATE_COUNT] = {NULL};

unsigned char *ndwiValid[DATE_COUNT] = {NULL};
unsigned char *ndviValid[DATE_COUNT] = {NULL};

int width = 0;
int height = 0;

size_t totalPixels = 0;
size_t totalStoredValues = 0;

double referenceTransform[6] = {0};

int status = EXIT_FAILURE;
int validationPassed = 0;

printf("Starting input validation...\n\n");

for (int d = 0; d < DATE_COUNT; d++)
{
for (int b = 0; b < BAND_COUNT; b++)
{
const char *path = filePaths[d][b];

datasets[d][b] =
GDALOpen(path, GA_ReadOnly);

if (datasets[d][b] == NULL)
{
fprintf(stderr,
"INVALID: Cannot open file: %s\n",
path);

goto cleanup;
}

if (!validateDimensions(datasets[d][b], path))
goto cleanup;

if (!validateBandCount(datasets[d][b], path))
goto cleanup;
}
}

width = GDALGetRasterXSize(datasets[0][0]);
height = GDALGetRasterYSize(datasets[0][0]);

if (!readTransform(
datasets[0][0],
referenceTransform))
{
fprintf(stderr,
"INVALID: Reference geotransform is missing "
"or invalid.\n");

goto cleanup;
}

const char *referenceProjection =
GDALGetProjectionRef(datasets[0][0]);

if (referenceProjection == NULL ||
referenceProjection[0] == '\0')
{
fprintf(stderr,
"INVALID: Reference CRS is missing.\n");

goto cleanup;
}

for (int d = 0; d < DATE_COUNT; d++)
{
for (int b = 0; b < BAND_COUNT; b++)
{
if (d == 0 && b == 0)
continue;

if (!validateMetadata(
datasets[0][0],
datasets[d][b],
referenceTransform,
filePaths[d][b],
width,
height))
{
goto cleanup;
}
}
}

if ((size_t)width > SIZE_MAX / (size_t)height)
{
fprintf(stderr,
"INVALID: Image dimensions overflow.\n");

goto cleanup;
}

totalPixels = (size_t)width * (size_t)height;

if (totalPixels > SIZE_MAX / sizeof(float) ||
totalPixels > SIZE_MAX / sizeof(unsigned char))
{
fprintf(stderr,
"INVALID: Pixel buffer size overflow.\n");

goto cleanup;
}

printf("\nReading raster metadata and pixel data...\n\n");

for (int d = 0; d < DATE_COUNT; d++)
{
for (int b = 0; b < BAND_COUNT; b++)
{
if (!getNoDataInformation(
datasets[d][b],
&noDataValues[d][b],
&hasNoData[d][b],
filePaths[d][b]))
{
goto cleanup;
}

bandData[d][b] =
malloc(totalPixels * sizeof(float));

bandValid[d][b] =
malloc(totalPixels * sizeof(unsigned char));

if (bandData[d][b] == NULL ||
bandValid[d][b] == NULL)
{
fprintf(stderr,
"ERROR: Band memory allocation failed.\n");

goto cleanup;
}

if (!readBandPixels(
datasets[d][b],
bandData[d][b],
width,
height,
filePaths[d][b]))
{
goto cleanup;
}

totalStoredValues += totalPixels;
}
}

validationPassed = 1;

printf("\nVALID FOR COMPARISON\n");
printf("All 8 files opened successfully.\n");
printf("Image dimensions: %d x %d\n", width, height);
printf("CRS and geotransform checks passed.\n");
printf("All raster pixel data were read successfully.\n");
printf("Total band pixel values stored: %zu\n\n",
totalStoredValues);

for (int d = 0; d < DATE_COUNT; d++)
{
size_t totalInvalidBandPixels = 0;

printf("Date: %s\n", dates[d]);

for (int b = 0; b < BAND_COUNT; b++)
{
size_t validCount = 0;
size_t invalidCount = 0;

for (size_t i = 0; i < totalPixels; i++)
{
int valid = isValidSample(
bandData[d][b][i],
noDataValues[d][b],
hasNoData[d][b]);

bandValid[d][b][i] =
(unsigned char)valid;

if (valid)
validCount++;
else
invalidCount++;
}

totalInvalidBandPixels += invalidCount;

printf("%s: valid=%zu, invalid=%zu\n",
bandNames[b],
validCount,
invalidCount);
}

printf("Total invalid band-pixel entries: %zu\n\n",
totalInvalidBandPixels);
}

for (int d = 0; d < DATE_COUNT; d++)
{
ndwiData[d] =
malloc(totalPixels * sizeof(float));

ndviData[d] =
malloc(totalPixels * sizeof(float));

ndwiValid[d] =
calloc(totalPixels, sizeof(unsigned char));

ndviValid[d] =
calloc(totalPixels, sizeof(unsigned char));

if (ndwiData[d] == NULL ||
ndviData[d] == NULL ||
ndwiValid[d] == NULL ||
ndviValid[d] == NULL)
{
fprintf(stderr,
"ERROR: NDWI/NDVI memory allocation failed.\n");

goto cleanup;
}

size_t validNDWI = 0;
size_t invalidNDWI = 0;
size_t validNDVI = 0;
size_t invalidNDVI = 0;

for (size_t i = 0; i < totalPixels; i++)
{
float green = bandData[d][GREEN][i];
float red = bandData[d][RED][i];
float nir = bandData[d][NIR][i];

double indexValue;

if (bandValid[d][GREEN][i] &&
bandValid[d][NIR][i] &&
calculateIndex(green, nir, &indexValue) &&
isfinite((float)indexValue))
{
ndwiData[d][i] = (float)indexValue;
ndwiValid[d][i] = 1;
validNDWI++;
}
else
{
ndwiData[d][i] = NAN;
ndwiValid[d][i] = 0;
invalidNDWI++;
}

if (bandValid[d][NIR][i] &&
bandValid[d][RED][i] &&
calculateIndex(nir, red, &indexValue) &&
isfinite((float)indexValue))
{
ndviData[d][i] = (float)indexValue;
ndviValid[d][i] = 1;
validNDVI++;
}
else
{
ndviData[d][i] = NAN;
ndviValid[d][i] = 0;
invalidNDVI++;
}
}

printf("Index results for %s:\n", dates[d]);
printf("NDWI valid pixels: %zu\n", validNDWI);
printf("NDWI invalid pixels: %zu\n", invalidNDWI);
printf("NDVI valid pixels: %zu\n", validNDVI);
printf("NDVI invalid pixels: %zu\n\n", invalidNDVI);
}

{
size_t sample = 0;

printf("Sample pixel at row 0, column 0:\n\n");

for (int d = 0; d < DATE_COUNT; d++)
{
printf("Date: %s\n", dates[d]);

for (int b = 0; b < BAND_COUNT; b++)
{
if (bandValid[d][b][sample])
{
printf("%s = %.2f\n",
bandNames[b],
bandData[d][b][sample]);
}
else
{
printf("%s = INVALID\n", bandNames[b]);
}
}

if (ndwiValid[d][sample])
printf("NDWI = %.6f\n", ndwiData[d][sample]);
else
printf("NDWI = INVALID\n");

if (ndviValid[d][sample])
printf("NDVI = %.6f\n", ndviData[d][sample]);
else
printf("NDVI = INVALID\n");

printf("\n");
}
}
if (!createWaterMap(
datasets[0][GREEN],
ndwiData[0],
ndwiValid[0],
totalPixels,
width,
height,
"outputs/2019-01-09_water_map.tif",
0.0))
{
fprintf(stderr, "ERROR: Water-map generation failed.\n");
goto cleanup;
}
printf("SUCCESS: Preprocessing and index calculation completed.\n");
printf("Validity masks are ready for future water-map generation.\n");

status = EXIT_SUCCESS;

cleanup:

if (!validationPassed)
fprintf(stderr, "\nINVALID FOR COMPARISON\n");

for (int d = 0; d < DATE_COUNT; d++)
{
free(ndwiData[d]);
free(ndviData[d]);

free(ndwiValid[d]);
free(ndviValid[d]);

for (int b = 0; b < BAND_COUNT; b++)
{
free(bandData[d][b]);
free(bandValid[d][b]);

if (datasets[d][b] != NULL)
GDALClose(datasets[d][b]);
}
}

return status;
}