#include <stdio.h>
#include <gdal.h>

int main(void)
{
GDALAllRegister();

const char *filename = "dataset/S2_2019-01-09_B04.tif";

GDALDatasetH dataset = GDALOpen(filename, GA_ReadOnly);

if (dataset == NULL) {
printf("Error: Could not open TIFF file.\n");
GDALDestroyDriverManager();
return 1;
}

printf("TIFF opened successfully!\n");
printf("Width: %d pixels\n", GDALGetRasterXSize(dataset));
printf("Height: %d pixels\n", GDALGetRasterYSize(dataset));
printf("Number of bands: %d\n", GDALGetRasterCount(dataset));

GDALClose(dataset);
GDALDestroyDriverManager();

return 0;
}