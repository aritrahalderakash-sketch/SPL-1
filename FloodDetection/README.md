ARITRA HALDER
	
7:03 PM (0 minutes ago)
	
	
to me
Satellite-Based Flood Detection
Project Overview

This project aims to detect potential flood-related water changes using Sentinel-2 satellite imagery and C programming.
Dataset

Source: SEN12-FLOOD

Selected acquisition dates:

    9 January 2019
    14 January 2019

Bands used:

    B02 — Blue
    B03 — Green
    B04 — Red
    B08 — Near-Infrared (NIR)

Initial Dataset Validation

    Eight GeoTIFF files collected.
    Each image is 512 × 512 pixels.
    The images have matching origins and pixel sizes.
    Pixel values were successfully read using GDAL.

Planned Implementation

    Read satellite image pixels using GDAL.
    Calculate NDWI and NDVI.
    Generate water maps.
    Compare images from the two dates.
    Identify potential new-water regions and analyze connected regions using BFS/DFS.

Current status: Initial dataset inspection completed. Flood-event suitability and change detection still need to be verified.
