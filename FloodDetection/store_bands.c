
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gdal.h>
#include <cpl_conv.h>

#define DATE_COUNT 2
#define BAND_COUNT 4

enum { BLUE, GREEN, RED, NIR };

const char *bandNames[BAND_COUNT] = {
"B02 (Blue)",
"B03 (Green)",
"B04 (Red)",
"B08 (NIR)"
};

const char *filenames[DATE_COUNT][BAND_COUNT] = {
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

int width = 0, height = 0;
double referenceTransform[6];
char *referenceProjection = NULL;
int success = 0;

for (int d = 0; d < DATE_COUNT; d++)
{
for (int b = 0; b < BAND_COUNT; b++)
{
datasets[d][b] =
GDALOpen(filenames[d][b], GA_ReadOnly);

if (datasets[d][b] == NULL)
{
printf("INVALID: Cannot open %s\n",
filenames[d][b]);
goto cleanup;
}

if (GDALGetRasterCount(datasets[d][b]) != 1)
{
printf("INVALID: Expected one band in %s\n",
filenames[d][b]);
goto cleanup;
}

int w = GDALGetRasterXSize(datasets[d][b]);
int h = GDALGetRasterYSize(datasets[d][b]);

if (d == 0 && b == 0)
{
width = w;
height = h;

if (GDALGetGeoTransform(
datasets[d][b],
referenceTransform) != CE_None)
{
printf("INVALID: Missing georeferencing.\n");
goto cleanup;
}

const char *projection =
GDALGetProjectionRef(datasets[d][b]);

if (projection == NULL || projection[0] == '\0')
{
printf("INVALID: Missing coordinate system.\n");
goto cleanup;
}

referenceProjection = CPLStrdup(projection);
}
else
{
if (w != width || h != height)
{
printf("INVALID: Dimensions do not match: %s\n",
filenames[d][b]);
goto cleanup;
}

double transform[6];

if (GDALGetGeoTransform(
datasets[d][b], transform) != CE_None)
{
printf("INVALID: Missing georeferencing: %s\n",
filenames[d][b]);
goto cleanup;
}

for (int i = 0; i < 6; i++)
{
if (fabs(transform[i] - referenceTransform[i])
> 0.000001)
{
printf("INVALID: Images are not aligned: %s\n",
filenames[d][b]);
goto cleanup;
}
}

const char *projection =
GDALGetProjectionRef(datasets[d][b]);

if (projection == NULL ||
strcmp(projection, referenceProjection) != 0)
{
printf("INVALID: Coordinate systems do not match: %s\n",
filenames[d][b]);
goto cleanup;
}
}
}
}

printf("Input validation successful!\n");
printf("Image dimensions: %d x %d\n\n", width, height);

size_t totalPixels = (size_t)width * height;

for (int d = 0; d < DATE_COUNT; d++)
{
printf("Reading image date: %s\n",
d == 0 ? "2019-01-09" : "2019-01-14");

for (int b = 0; b < BAND_COUNT; b++)
{
bandData[d][b] =
malloc(totalPixels * sizeof(float));

if (bandData[d][b] == NULL)
{
printf("ERROR: Memory allocation failed.\n");
goto cleanup;
}

GDALRasterBandH band =
GDALGetRasterBand(datasets[d][b], 1);

if (GDALRasterIO(
band, GF_Read,
0, 0, width, height,
bandData[d][b], width, height,
GDT_Float32, 0, 0) != CE_None)
{
printf("ERROR: Could not read %s\n",
filenames[d][b]);
goto cleanup;
}

printf("Stored: %s\n", bandNames[b]);
}

printf("\n");
}

size_t sample = 0;

printf("Sample pixel at row 0, column 0:\n");

for (int d = 0; d < DATE_COUNT; d++)
{
printf("%s:\n",
d == 0 ? "2019-01-09" : "2019-01-14");

for (int b = 0; b < BAND_COUNT; b++)
{
printf(" %s = %.2f\n",
bandNames[b],
bandData[d][b][sample]);
}
}

printf("\nSUCCESS: All 8 bands are stored in memory.\n");
printf("Total pixel values stored: %zu\n",
totalPixels * DATE_COUNT * BAND_COUNT);

success = 1;

cleanup:

for (int d = 0; d < DATE_COUNT; d++)
{
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
GDALDestroyDriverManager();

return success ? 0 : 1;
}