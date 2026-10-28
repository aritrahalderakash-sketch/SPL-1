

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gdal.h"
#include "cpl_conv.h"

#define DATE_COUNT 2
#define BAND_COUNT 4

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

int main(void)
{
GDALAllRegister();

GDALDatasetH datasets[DATE_COUNT][BAND_COUNT] = {{NULL}};

float *bandData[DATE_COUNT][BAND_COUNT] = {{NULL}};
float *ndwiData[DATE_COUNT] = {NULL};
float *ndviData[DATE_COUNT] = {NULL};

int width = 0;
int height = 0;
int referenceWidth = 0;
int referenceHeight = 0;

double referenceTransform[6] = {0};
double currentTransform[6] = {0};

char *referenceProjection = NULL;

size_t totalPixels = 0;
size_t totalStoredValues = 0;

int status = EXIT_FAILURE;

/* STEP 1: INPUT VALIDATION */

printf("Starting input validation...\n\n");

for (int d = 0; d < DATE_COUNT; d++)
{
for (int b = 0; b < BAND_COUNT; b++)
{
datasets[d][b] =
GDALOpen(filePaths[d][b], GA_ReadOnly);

if (datasets[d][b] == NULL)
{
fprintf(stderr,
"ERROR: Cannot open file: %s\n",
filePaths[d][b]);
goto cleanup;
}

int currentWidth =
GDALGetRasterXSize(datasets[d][b]);

int currentHeight =
GDALGetRasterYSize(datasets[d][b]);

if (GDALGetRasterCount(datasets[d][b]) != 1)
{
fprintf(stderr,
"ERROR: Expected one raster band: %s\n",
filePaths[d][b]);
goto cleanup;
}

if (d == 0 && b == 0)
{
referenceWidth = currentWidth;
referenceHeight = currentHeight;

if (GDALGetGeoTransform(
datasets[d][b],
referenceTransform) != CE_None)
{
fprintf(stderr,
"ERROR: Cannot read geotransform.\n");
goto cleanup;
}

const char *projection =
GDALGetProjectionRef(datasets[d][b]);

if (projection == NULL || projection[0] == '\0')
{
fprintf(stderr,
"ERROR: Reference CRS is missing.\n");
goto cleanup;
}

referenceProjection = CPLStrdup(projection);

if (referenceProjection == NULL)
{
fprintf(stderr,
"ERROR: Cannot store reference CRS.\n");
goto cleanup;
}
}
else
{
if (currentWidth != referenceWidth ||
currentHeight != referenceHeight)
{
fprintf(stderr,
"ERROR: Image dimensions do not match: %s\n",
filePaths[d][b]);
goto cleanup;
}

if (GDALGetGeoTransform(
datasets[d][b],
currentTransform) != CE_None)
{
fprintf(stderr,
"ERROR: Cannot read geotransform: %s\n",
filePaths[d][b]);
goto cleanup;
}

for (int i = 0; i < 6; i++)
{
if (fabs(currentTransform[i] -
referenceTransform[i]) > 1e-8)
{
fprintf(stderr,
"ERROR: Image alignment mismatch: %s\n",
filePaths[d][b]);
goto cleanup;
}
}

const char *projection =
GDALGetProjectionRef(datasets[d][b]);

if (projection == NULL ||
strcmp(projection, referenceProjection) != 0)
{
fprintf(stderr,
"ERROR: CRS mismatch: %s\n",
filePaths[d][b]);
goto cleanup;
}
}
}
}

width = referenceWidth;
height = referenceHeight;
totalPixels = (size_t)width * (size_t)height;

printf("Input validation successful!\n");
printf("Image dimensions: %d x %d\n\n", width, height);

/* STEP 2: STORE ALL BAND PIXELS IN MEMORY */

for (int d = 0; d < DATE_COUNT; d++)
{
printf("Reading image date: %s\n", dates[d]);

for (int b = 0; b < BAND_COUNT; b++)
{
bandData[d][b] =
malloc(totalPixels * sizeof(float));

if (bandData[d][b] == NULL)
{
fprintf(stderr,
"ERROR: Memory allocation failed.\n");
goto cleanup;
}

GDALRasterBandH rasterBand =
GDALGetRasterBand(datasets[d][b], 1);

if (GDALRasterIO(
rasterBand,
GF_Read,
0, 0,
width, height,
bandData[d][b],
width, height,
GDT_Float32,
0, 0) != CE_None)
{
fprintf(stderr,
"ERROR: Cannot read pixels from %s\n",
filePaths[d][b]);
goto cleanup;
}

totalStoredValues += totalPixels;

printf("Stored: %s\n", bandNames[b]);
}

printf("\n");
}

printf("SUCCESS: All 8 bands are stored in memory.\n");
printf("Total pixel values stored: %zu\n\n",
totalStoredValues);

/* STEP 3: CALCULATE NDWI AND NDVI FOR EVERY PIXEL */

printf("Starting NDWI and NDVI calculations...\n\n");

for (int d = 0; d < DATE_COUNT; d++)
{
ndwiData[d] = malloc(totalPixels * sizeof(float));
ndviData[d] = malloc(totalPixels * sizeof(float));

if (ndwiData[d] == NULL || ndviData[d] == NULL)
{
fprintf(stderr,
"ERROR: NDWI/NDVI memory allocation failed.\n");
goto cleanup;
}

size_t validNDWI = 0;
size_t validNDVI = 0;

for (size_t i = 0; i < totalPixels; i++)
{
float green = bandData[d][GREEN][i];
float red = bandData[d][RED][i];
float nir = bandData[d][NIR][i];

float ndwiDenominator = green + nir;
float ndviDenominator = nir + red;

/* NDWI = (Green - NIR) / (Green + NIR) */

if (ndwiDenominator != 0.0f)
{
ndwiData[d][i] =
(green - nir) / ndwiDenominator;

validNDWI++;
}
else
{
ndwiData[d][i] = 0.0f;
}

/* NDVI = (NIR - Red) / (NIR + Red) */

if (ndviDenominator != 0.0f)
{
ndviData[d][i] =
(nir - red) / ndviDenominator;

validNDVI++;
}
else
{
ndviData[d][i] = 0.0f;
}
}

printf("Date: %s\n", dates[d]);
printf("NDWI calculated for %zu pixels\n", validNDWI);
printf("NDVI calculated for %zu pixels\n\n", validNDVI);
}

/* STEP 4: DISPLAY SAMPLE PIXEL VALUES */

{
size_t sample = 0;

printf("Sample pixel at row 0, column 0:\n\n");

for (int d = 0; d < DATE_COUNT; d++)
{
printf("Date: %s\n", dates[d]);

printf("Blue = %.2f\n",
bandData[d][BLUE][sample]);

printf("Green = %.2f\n",
bandData[d][GREEN][sample]);

printf("Red = %.2f\n",
bandData[d][RED][sample]);

printf("NIR = %.2f\n",
bandData[d][NIR][sample]);

printf("NDWI = %.6f\n",
ndwiData[d][sample]);

printf("NDVI = %.6f\n\n",
ndviData[d][sample]);
}
}

printf("SUCCESS: All band values, NDWI and NDVI "
"are calculated and stored in memory.\n");

status = EXIT_SUCCESS;

cleanup:

/* STEP 5: FREE MEMORY AND CLOSE FILES */

for (int d = 0; d < DATE_COUNT; d++)
{
free(ndwiData[d]);
free(ndviData[d]);

for (int b = 0; b < BAND_COUNT; b++)
{
free(bandData[d][b]);

if (datasets[d][b] != NULL)
{
GDALClose(datasets[d][b]);
}
}
}

CPLFree(referenceProjection);

return status;
}