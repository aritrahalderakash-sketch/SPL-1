
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <gdal.h>
#include <cpl_conv.h>

#define IMAGE_COUNT 8

int main(void)
{
GDALAllRegister();

const char *filenames[IMAGE_COUNT] = {
"dataset/S2_2019-01-09_B02.tif",
"dataset/S2_2019-01-09_B03.tif",
"dataset/S2_2019-01-09_B04.tif",
"dataset/S2_2019-01-09_B08.tif",
"dataset/S2_2019-01-14_B02.tif",
"dataset/S2_2019-01-14_B03.tif",
"dataset/S2_2019-01-14_B04.tif",
"dataset/S2_2019-01-14_B08.tif"
};

GDALDatasetH datasets[IMAGE_COUNT] = {NULL};
float *pixels[IMAGE_COUNT] = {NULL};

int width = 0;
int height = 0;
int success = 0;

double referenceTransform[6];
char *referenceProjection = NULL;

/* Step 1: Open and validate all images */

for (int i = 0; i < IMAGE_COUNT; i++)
{
datasets[i] = GDALOpen(filenames[i], GA_ReadOnly);

if (datasets[i] == NULL)
{
printf("INVALID: Cannot open %s\n", filenames[i]);
goto cleanup;
}

printf("Opened: %s\n", filenames[i]);

if (GDALGetRasterCount(datasets[i]) != 1)
{
printf("INVALID: Expected one band in %s\n",
filenames[i]);
goto cleanup;
}

int currentWidth = GDALGetRasterXSize(datasets[i]);
int currentHeight = GDALGetRasterYSize(datasets[i]);

if (i == 0)
{
width = currentWidth;
height = currentHeight;

if (GDALGetGeoTransform(
datasets[i], referenceTransform) != CE_None)
{
printf("INVALID: Missing georeferencing information.\n");
goto cleanup;
}

const char *projection =
GDALGetProjectionRef(datasets[i]);

if (projection == NULL || projection[0] == '\0')
{
printf("INVALID: Missing coordinate reference system.\n");
goto cleanup;
}

referenceProjection = CPLStrdup(projection);
}
else
{
/* Check image dimensions */

if (currentWidth != width || currentHeight != height)
{
printf("INVALID: Image dimensions do not match: %s\n",
filenames[i]);
goto cleanup;
}

/* Check geographic origin and pixel resolution */

double currentTransform[6];

if (GDALGetGeoTransform(
datasets[i], currentTransform) != CE_None)
{
printf("INVALID: Missing georeferencing: %s\n",
filenames[i]);
goto cleanup;
}

for (int j = 0; j < 6; j++)
{
if (fabs(currentTransform[j] -
referenceTransform[j]) > 0.000001)
{
printf("INVALID: Images are not aligned: %s\n",
filenames[i]);
goto cleanup;
}
}

/* Check coordinate reference system */

const char *projection =
GDALGetProjectionRef(datasets[i]);

if (projection == NULL ||
strcmp(projection, referenceProjection) != 0)
{
printf("INVALID: Coordinate systems do not match: %s\n",
filenames[i]);
goto cleanup;
}
}
}

printf("\nInput validation successful!\n");
printf("All 8 images have matching dimensions and grid metadata.\n");
printf("Width: %d pixels\n", width);
printf("Height: %d pixels\n", height);

/* Step 2: Allocate memory and read pixel values */

size_t totalPixels = (size_t)width * height;

for (int i = 0; i < IMAGE_COUNT; i++)
{
pixels[i] = malloc(totalPixels * sizeof(float));

if (pixels[i] == NULL)
{
printf("ERROR: Memory allocation failed.\n");
goto cleanup;
}

GDALRasterBandH band =
GDALGetRasterBand(datasets[i], 1);

CPLErr result = GDALRasterIO(
band,
GF_Read,
0, 0,
width, height,
pixels[i],
width, height,
GDT_Float32,
0, 0
);

if (result != CE_None)
{
printf("ERROR: Could not read pixels from %s\n",
filenames[i]);
goto cleanup;
}

printf("Pixels read and stored: %s\n", filenames[i]);
}

printf("\nSUCCESS: All 8 images were read and stored in memory.\n");
printf("Total pixel values stored: %zu\n",
totalPixels * IMAGE_COUNT);

success = 1;

cleanup:

for (int i = 0; i < IMAGE_COUNT; i++)
{
free(pixels[i]);

if (datasets[i] != NULL)
GDALClose(datasets[i]);
}

CPLFree(referenceProjection);
GDALDestroyDriverManager();

return success ? 0 : 1;
}