
#include <stdio.h>
#include <stdlib.h>
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

int width = 0;
int height = 0;
int success = 0;

double referenceTransform[6];
char *referenceProjection = NULL;

printf("========================================\n");
printf(" SATELLITE IMAGE VALIDATION\n");
printf("========================================\n\n");

printf("STEP 1: OPENING SATELLITE IMAGES\n");
printf("----------------------------------------\n");

for (int i = 0; i < IMAGE_COUNT; i++)
{
datasets[i] = GDALOpen(filenames[i], GA_ReadOnly);

if (datasets[i] == NULL)
{
printf("[FAILED] Cannot open: %s\n", filenames[i]);
goto cleanup;
}

printf("[OPENED] %s\n", filenames[i]);
}

printf("\nAll 8 images opened successfully.\n");

printf("\nSTEP 2: INPUT VALIDATION\n");
printf("----------------------------------------\n");

printf("[CHECK 1] Raster Band Count\n");

for (int i = 0; i < IMAGE_COUNT; i++)
{
if (GDALGetRasterCount(datasets[i]) != 1)
{
printf("[FAILED] Expected one band in %s\n",
filenames[i]);
goto cleanup;
}
}

printf("[PASSED] All images contain exactly one raster band.\n");

printf("\n[CHECK 2] Image Dimensions\n");

width = GDALGetRasterXSize(datasets[0]);
height = GDALGetRasterYSize(datasets[0]);

for (int i = 1; i < IMAGE_COUNT; i++)
{
int currentWidth = GDALGetRasterXSize(datasets[i]);
int currentHeight = GDALGetRasterYSize(datasets[i]);

if (currentWidth != width || currentHeight != height)
{
printf("[FAILED] Image dimensions do not match: %s\n",
filenames[i]);
goto cleanup;
}
}

printf("[PASSED] All images have matching dimensions.\n");
printf(" Width: %d pixels\n", width);
printf(" Height: %d pixels\n", height);

printf("\n[CHECK 3] Georeferencing Information\n");

if (GDALGetGeoTransform(
datasets[0], referenceTransform) != CE_None)
{
printf("[FAILED] Missing georeferencing information.\n");
goto cleanup;
}

printf("[PASSED] Reference image contains georeferencing information.\n");

printf("\n[CHECK 4] Grid Metadata / Image Alignment\n");

for (int i = 1; i < IMAGE_COUNT; i++)
{
double currentTransform[6];

if (GDALGetGeoTransform(
datasets[i], currentTransform) != CE_None)
{
printf("[FAILED] Missing georeferencing: %s\n",
filenames[i]);
goto cleanup;
}

for (int j = 0; j < 6; j++)
{
if (fabs(currentTransform[j] -
referenceTransform[j]) > 0.000001)
{
printf("[FAILED] Grid metadata does not match: %s\n",
filenames[i]);
goto cleanup;
}
}
}

printf("[PASSED] All images have matching grid metadata.\n");
printf(" Origin, pixel size and orientation match.\n");

printf("\n[CHECK 5] Coordinate Reference System (CRS)\n");

const char *projection =
GDALGetProjectionRef(datasets[0]);

if (projection == NULL || projection[0] == '\0')
{
printf("[FAILED] Missing CRS information in reference image.\n");
goto cleanup;
}

referenceProjection = CPLStrdup(projection);

for (int i = 1; i < IMAGE_COUNT; i++)
{
const char *currentProjection =
GDALGetProjectionRef(datasets[i]);

if (currentProjection == NULL ||
strcmp(currentProjection, referenceProjection) != 0)
{
printf("[FAILED] Coordinate systems do not match: %s\n",
filenames[i]);
goto cleanup;
}
}

printf("[PASSED] All images use the same CRS.\n");

success = 1;

cleanup:

for (int i = 0; i < IMAGE_COUNT; i++)
{
if (datasets[i] != NULL)
{
GDALClose(datasets[i]);
}
}

CPLFree(referenceProjection);
GDALDestroyDriverManager();

printf("\n========================================\n");

if (success)
{
printf(" INPUT VALIDATION SUCCESSFUL!\n");
printf("========================================\n");
printf("All 8 satellite images passed validation.\n");
printf("No pixel values were read or stored.\n");
}
else
{
printf(" INPUT VALIDATION UNSUCCESSFUL!\n");
printf("========================================\n");
printf("One or more validation checks failed.\n");
}

return success ? 0 : 1;
}