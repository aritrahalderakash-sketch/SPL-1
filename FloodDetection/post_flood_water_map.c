#include <stdio.h>
#include <stdlib.h>
#include <gdal.h>
#include <cpl_conv.h>

int main()
{
GDALAllRegister();

GDALDatasetH greenDataset = GDALOpen(
"dataset/S2_2019-01-14_B03.tif",
GA_ReadOnly
);

GDALDatasetH nirDataset = GDALOpen(
"dataset/S2_2019-01-14_B08.tif",
GA_ReadOnly
);

if (greenDataset == NULL || nirDataset == NULL)
{
printf("Error: Could not open input bands.\n");
return 1;
}

int width = GDALGetRasterXSize(greenDataset);
int height = GDALGetRasterYSize(greenDataset);

if (width != GDALGetRasterXSize(nirDataset) ||
height != GDALGetRasterYSize(nirDataset))
{
printf("Error: Band dimensions do not match.\n");
GDALClose(greenDataset);
GDALClose(nirDataset);
return 1;
}

int totalPixels = width * height;

float *green = malloc(totalPixels * sizeof(float));
float *nir = malloc(totalPixels * sizeof(float));

unsigned char *waterMap = malloc(totalPixels);

if (!green || !nir || !waterMap)
{
printf("Error: Memory allocation failed.\n");
free(green);
free(nir);
free(waterMap);
GDALClose(greenDataset);
GDALClose(nirDataset);
return 1;
}

GDALRasterBandH greenBand =
GDALGetRasterBand(greenDataset, 1);

GDALRasterBandH nirBand =
GDALGetRasterBand(nirDataset, 1);

GDALRasterIO(
greenBand, GF_Read, 0, 0, width, height,
green, width, height, GDT_Float32, 0, 0
);

GDALRasterIO(
nirBand, GF_Read, 0, 0, width, height,
nir, width, height, GDT_Float32, 0, 0
);

int waterPixels = 0;
int nonWaterPixels = 0;

for (int i = 0; i < totalPixels; i++)
{
float denominator = green[i] + nir[i];

if (denominator == 0)
{
waterMap[i] = 255;
continue;
}

float ndwi = (green[i] - nir[i]) / denominator;

if (ndwi > 0.0)
{
waterMap[i] = 1;
waterPixels++;
}
else
{
waterMap[i] = 0;
nonWaterPixels++;
}
}

GDALDriverH driver = GDALGetDriverByName("GTiff");

GDALDatasetH output = GDALCreate(
driver,
"outputs/2019-01-14_post_flood_water_map.tif",
width,
height,
1,
GDT_Byte,
NULL
);

if (output == NULL)
{
printf("Error: Could not create output file.\n");
free(green);
free(nir);
free(waterMap);
GDALClose(greenDataset);
GDALClose(nirDataset);
return 1;
}

double transform[6];
GDALGetGeoTransform(greenDataset, transform);
GDALSetGeoTransform(output, transform);

GDALSetProjection(
output,
GDALGetProjectionRef(greenDataset)
);

GDALRasterBandH outputBand =
GDALGetRasterBand(output, 1);

GDALSetRasterNoDataValue(outputBand, 255);

GDALRasterIO(
outputBand, GF_Write, 0, 0, width, height,
waterMap, width, height, GDT_Byte, 0, 0
);

GDALClose(output);
GDALClose(greenDataset);
GDALClose(nirDataset);

free(green);
free(nir);
free(waterMap);

printf("Post-flood water map created successfully!\n");
printf("Water pixels: %d\n", waterPixels);
printf("Non-water pixels: %d\n", nonWaterPixels);

return 0;
}