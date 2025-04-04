import geopandas as gpd
import pandas as pd
import rasterio
import os
import numpy as np
from shapely.geometry import LineString
from rasterio.transform import rowcol
from scipy.interpolate import interp1d
from rasterio.transform import xy
from rasterio.warp import calculate_default_transform, reproject, Resampling, transform_geom
from rasterio.transform import array_bounds
from rasterio.crs import CRS

import matplotlib.pyplot as plt

from datetime import datetime
from sklearn.linear_model import LinearRegression



def days_difference(date1: str, date2: str) -> int:
    """
    Calculate the difference in days between two dates.

    Parameters:
    - date1 (str): The first date in 'yyyymmdd' format.
    - date2 (str): The second date in 'yyyymmdd' format.

    Returns:
    - int: The number of days between date2 and date1 (date2 - date1).
    """
    date1 = datetime.strptime(date1, "%Y%m%d")
    date2 = datetime.strptime(date2, "%Y%m%d")

    return (date2 - date1).days  # Positive if date2 is later than date1

def plot_elevation_profile(distances, elevations):
    """
    Plot an elevation profile.

    Parameters:
    - distances (list or array): The distances along the transect.
    - elevations (list or array): The corresponding elevation values.
    """
    plt.figure(figsize=(10, 5))
    plt.plot(distances, elevations, marker='o', linestyle='-', markersize=4, label='Elevation Profile')
    plt.xlabel("Distance (m)")
    plt.ylabel("Elevation (m)")
    plt.title("Elevation Profile Along Transect")
    plt.grid(True)
    plt.legend()
    plt.show()

# Example usage:
# plot_elevation_profile(profile_distances, elevation_profile)


def load_raster(raster_folder: str, raster_file: str):
    """
    Load a raster file from a given folder and return its data and metadata.

    Parameters:
    - raster_folder (str): Path to the folder containing the raster file.
    - raster_file (str): Name of the raster file (with or without extension).

    Returns:
    - numpy.ndarray: The raster data as a NumPy array.
    - dict: The metadata of the raster file.
    """
    # Ensure the file has a valid raster extension (assume .tif if no extension)
    if not raster_file.endswith((".tif", ".tiff")):
        raster_file += ".tif"

    # Construct the full path to the raster file
    raster_path = os.path.join(raster_folder, raster_file)

    # Open the raster file
    with rasterio.open(raster_path) as dataset:
        raster_data = dataset.read()  # Read the raster as a NumPy array
        metadata = dataset.meta  # Extract metadata

    return raster_data, metadata

def load_shapefile(shp_folder: str, shp_file: str) -> gpd.GeoDataFrame:
    """
    Load a shapefile from a given folder and return it as a GeoDataFrame.

    Parameters:
    - shp_folder (str): Path to the folder containing the shapefile.
    - shp_file (str): Name of the shapefile (with or without .shp extension).

    Returns:
    - gpd.GeoDataFrame: The loaded shapefile as a GeoDataFrame.
    """
    # Ensure the file has the .shp extension
    if not shp_file.endswith(".shp"):
        shp_file += ".shp"

    # Construct the full file path
    shp_path = os.path.join(shp_folder, shp_file)

    # Load and return the shapefile
    return gpd.read_file(shp_path)

def extract_transect_line(geo_df: gpd.GeoDataFrame, baseline_id: int, transect_id: int, image_id: int,\
                        baseline_col:str="BaselineId", transect_col:str="TransectId", image_col:str="ImageId") -> LineString:
    """
    Extract the transect line geometry from a GeoDataFrame based on the given IDs.

    Parameters:
    - geo_df (gpd.GeoDataFrame): The input GeoDataFrame with columns ['BaselineId', 'TransectId', 'ImageId', 'ChangeRate'].
    - baseline_id (int): The BaselineId of the transect.
    - transect_id (int): The TransectId of the transect.
    - image_id (int): The ImageId associated with the transect.

    Returns:
    - shapely.geometry.LineString: The geometry of the matched transect line.
    - None: If no matching transect is found.
    """
    # Filter the GeoDataFrame for the matching row
    filtered_df = geo_df[
        (geo_df[baseline_col] == baseline_id) &
        (geo_df[transect_col] == transect_id) &
        (geo_df[image_col] == image_id)
    ]

    # Return the geometry if a match is found, else return None
    return filtered_df if not filtered_df.empty else None

def extract_raster_profile_from_metadata(raster_array: np.ndarray, metadata: dict, transect_line: LineString, num_points=100):
    """
    Extract an elevation profile along a transect line from a raster (e.g., DEM).

    Parameters:
    - raster_array (np.ndarray): The raster data (2D NumPy array).
    - metadata (dict): Metadata from the raster (must include 'transform' and 'crs').
    - transect_line (LineString): The transect line geometry.
    - num_points (int): Number of interpolated points along the transect.

    Returns:
    - list: The extracted elevation values along the transect.
    - list: The corresponding distances along the transect.
    """
    # Retrieve transform and CRS from metadata
    transform = metadata["transform"]
    raster_crs = metadata["crs"]

    # Check if the transect_line is in the same CRS as the raster
    if transect_line.crs != raster_crs:
        transect_line = transform_geom(transect_line.crs, raster_crs, transect_line)

    # Generate interpolated points along the transect
    distances = np.linspace(0, transect_line.length, num_points)
    interpolated_points = [transect_line.interpolate(d) for d in distances]
    interpolated_coords = [(p.x, p.y) for p in interpolated_points]

    # Convert world coordinates to raster row-column indexes
    row_cols = [rowcol(transform, x, y) for x, y in interpolated_coords]

    # Fix: Ensure rowcol() output is unpacked correctly
    row_cols = [(int(row[0]) if isinstance(row, list) else int(row),
                 int(col[0]) if isinstance(col, list) else int(col))
                for row, col in row_cols]

    # Debug: Print row-col values to verify correctness
    print("Row-Column Indexes (First 5):", row_cols[:5])

    # Extract elevation values (handle out-of-bounds cases)
    elevation_values = []
    for row, col in row_cols:
        if 0 <= row < raster_array.shape[1] and 0 <= col < raster_array.shape[2]:
            elevation_values.append(raster_array[0, row, col])
        else:
            elevation_values.append(np.nan)  # Assign NaN if out of bounds

    return elevation_values, distances.tolist()

def find_elevation_intersections(elevation_values, distances, target_elevation):
    """
    Find all intersection distances where the target elevation occurs.

    Parameters:
    - elevation_values (list or array): Elevation values along the transect.
    - distances (list or array): Corresponding distances along the transect.
    - target_elevation (float): The elevation value to find intersections.

    Returns:
    - list: List of distances where the target elevation intersects.
            If no intersection is found, returns [-999999].
    """
    # Convert lists to numpy arrays for interpolation
    elevations = np.array(elevation_values)
    dists = np.array(distances)

    # Find indices where elevation crosses the target elevation
    sign_changes = np.where(np.diff(np.sign(elevations - target_elevation)))[0]

    intersection_dists = []

    for i in sign_changes:
        # Linear interpolation to find exact distance where elevation = target_elevation
        x1, x2 = dists[i], dists[i + 1]
        y1, y2 = elevations[i], elevations[i + 1]

        if y1 == y2:  # If both elevations are the same, take midpoint
            interp_dist = (x1 + x2) / 2
        else:
            interp_dist = x1 + (target_elevation - y1) * (x2 - x1) / (y2 - y1)

        intersection_dists.append(interp_dist[0])

    # If no intersection found, return [-999999]
    return intersection_dists if intersection_dists else [-999999]

def refine_intersection_points(dists, ref_dist):
    """
    Refine the intersection points to the nearest reference distance.

    Parameters:
    - dists (list): List of intersection distances.
    - ref_dist (float): Reference distance to snap the
                          intersection points to the nearest.
    """
    if len(dists) == 1 and dists[0] == -999999:
        return -999
    else:
        return min(dists, key=lambda x: abs(x - ref_dist))

def reproject_raster_to_match_shapefile(raster_array, metadata, target_crs):
    """
    Reprojects a raster to match the CRS of a shapefile.

    Parameters:
    - raster_array (np.ndarray): The raster data (NumPy array).
    - metadata (dict): Metadata from the raster.
    - target_crs (CRS): The CRS of the target shapefile (GeoDataFrame).

    Returns:
    - np.ndarray: Reprojected raster data.
    - dict: Updated metadata with the new CRS.
    """
    # Extract source CRS
    source_crs = metadata["crs"]

    # Calculate the transformation for the new CRS
    transform, width, height = calculate_default_transform(
        source_crs, target_crs, metadata["width"], metadata["height"], *metadata["bounds"]
    )

    # Create new metadata with the updated CRS
    new_metadata = metadata.copy()
    new_metadata.update({
        "crs": target_crs,
        "transform": transform,
        "width": width,
        "height": height
    })

    # Create an empty array for the reprojected raster
    reprojected_raster = np.empty((metadata["count"], height, width), dtype=raster_array.dtype)

    # Perform the reprojection
    reproject(
        source=raster_array,
        destination=reprojected_raster,
        src_transform=metadata["transform"],
        src_crs=source_crs,
        dst_transform=transform,
        dst_crs=target_crs,
        resampling=Resampling.nearest
    )

    return reprojected_raster, new_metadata

# write a function, traverse all files with the same suffix, and return the list of files.
def find_files_with_suffix(folder: str, suffix: str):
    """
    Find all files in a folder with a given suffix.

    Parameters:
    - folder (str): Path to the folder containing the files.
    - suffix (str): Suffix to match the files.

    Returns:
    - list: List of file names with the given suffix.
    """
    # Get all files in the folder
    all_files = os.listdir(folder)

    # Filter files that match the suffix
    matching_files = [f for f in all_files if f.endswith(suffix)]

    return matching_files

def build_study_site_dateinfo(matching_files):
    res = {}
    for m_f in matching_files:
        m_comps = m_f.split('.')[0].split('_')
        study_site = m_comps[1]
        dateinfo = m_comps[-1]
        year = dateinfo[:4]
        if study_site not in res:
            temp_dict = {year: dateinfo}
            res[study_site] = temp_dict
        else:
            res[study_site][year] = dateinfo
    return res

def merge_excel_files(folder: str, prefix: str, suffix: str) -> pd.DataFrame:
    """
    Reads all Excel files in the given folder that match the specified prefix and suffix,
    and merges those with the same prefix into a single DataFrame.

    Parameters:
    - folder (str): Path to the folder containing the Excel files.
    - prefix (str): Prefix of the filenames to filter.
    - suffix (str): Suffix (file extension) to filter (e.g., '.xlsx').

    Returns:
    - pd.DataFrame: A merged DataFrame containing data from all matching files.
    """
    # Get all files in the folder matching prefix and suffix
    matching_files = [f for f in os.listdir(folder) if f.startswith(prefix) and f.endswith(suffix)]
    
    if not matching_files:
        print(f"No matching files found with prefix '{prefix}' and suffix '{suffix}'.")
        return None

    # Read and merge files
    df_list = []
    for file in matching_files:
        file_path = os.path.join(folder, file)
        df = pd.read_csv(file_path)  # Read the Excel file
        df_list.append(df)

    # Concatenate all DataFrames
    merged_df = pd.concat(df_list, ignore_index=True)

    return merged_df

def reproject_raster(raster_array, metadata, input_epsg, output_epsg):
    """
    Reprojects a raster from one EPSG coordinate system to another.

    Parameters:
    - raster_array (np.ndarray): The raster data (NumPy array).
    - metadata (dict): Metadata from the raster.
    - input_epsg (int or str): EPSG code of the input raster.
    - output_epsg (int or str): EPSG code of the target coordinate system.

    Returns:
    - np.ndarray: Reprojected raster data.
    - dict: Updated metadata with the new CRS.
    """
    # Convert EPSG codes to CRS objects
    src_crs = CRS.from_epsg(int(input_epsg))
    dst_crs = CRS.from_epsg(int(output_epsg))

    # Extract source metadata
    transform = metadata["transform"]
    width, height = metadata["width"], metadata["height"]
    bounds = array_bounds(height, width, transform) 
    # Compute the transformation for the new CRS
    new_transform, new_width, new_height = calculate_default_transform(
        src_crs, dst_crs, width, height, *bounds
    )

    # Create new metadata with the updated CRS
    new_metadata = metadata.copy()
    new_metadata.update({
        "crs": dst_crs,
        "transform": new_transform,
        "width": new_width,
        "height": new_height
    })

    # Create an empty array for the reprojected raster
    reprojected_raster = np.empty((metadata["count"], new_height, new_width), dtype=raster_array.dtype)

    # Perform the reprojection
    reproject(
        source=raster_array,
        destination=reprojected_raster,
        src_transform=transform,
        src_crs=src_crs,
        dst_transform=new_transform,
        dst_crs=dst_crs,
        resampling=Resampling.nearest
    )

    return reprojected_raster, new_metadata  

def get_verified_value(df: pd.DataFrame, input_date: str) -> float:
    """
    Matches a given date (yyyymmdd) with the first row in the DataFrame where 'Date' matches
    and returns the corresponding value in the 'Verified (m)' column.

    Parameters:
    - df (pd.DataFrame): The input DataFrame containing 'Date' and 'Verified (m)' columns.
    - input_date (str): The target date in 'yyyymmdd' format.

    Returns:
    - float: The value from the 'Verified (m)' column if a match is found.
    - None: If no match is found.
    """
    # Convert 'Date' column to datetime format
    df['Date'] = pd.to_datetime(df['Date'], format='%Y/%m/%d')

    # Convert input_date to datetime
    target_date = pd.to_datetime(input_date, format='%Y%m%d')

    # Find the first matching row
    matching_row = df[df['Date'] == target_date]

    if not matching_row.empty:
        return matching_row.iloc[0]['Verified (m)']  # Return the first match
    else:
        return None  # No match found

def calculate_rate(time_diffs, distances):
    """
    Calculate the rate of distance change based on time differences.
    
    Parameters:
    - time_diffs (list or array): Time differences.
    - distances (list or array): Corresponding distances.

    Returns:
    - float: The rate of distance change.

    Raises:
    - ValueError: If the length of the input lists is less than 2.
    """
    if len(time_diffs) < 2 or len(distances) < 2:
        raise ValueError("At least two data points are required to calculate the rate.")

    if len(time_diffs) == 2:
        # Simple slope calculation for two points
        rate = (distances[1] - distances[0]) / (time_diffs[1] - time_diffs[0])
    else:
        # Use linear regression for more than two points
        X = np.array(time_diffs).reshape(-1, 1)  # Reshape for sklearn
        y = np.array(distances)

        model = LinearRegression()
        model.fit(X, y)
        rate = model.coef_[0]  # Slope of the regression line

    return rate

if __name__ == "__main__":
    lake_name = "LakeMichigan"
    lake_dir = "/media/weiwang/easystore/NAIP/%s/"%lake_name
    
    
    zip_files = find_files_with_suffix(lake_dir, "ZIP")
    site_date_info = build_study_site_dateinfo(zip_files)
    
    
    site_num = 4108617
    shp_folder = '/media/weiwang/easystore/NAIP/ErosionFiles_v3/LakeMichigan/%d/'%site_num
    transect_shp_file = '%d_transect.shp'%site_num
    intersect_shp_file = '%d_intersection.shp'%site_num
    transects = load_shapefile(shp_folder, transect_shp_file)
    intersection = load_shapefile(shp_folder, intersect_shp_file)
    
    
    raster_folder = '/media/weiwang/easystore/NAIP/Topybathy_LIDAR_DEM/Lake_Michigan_2020/usace2020_lake_mich_dem/'
    raster_file = 'usace2020_lake_mich_dem_J1137436.tif'
    raster_data, metadata = load_raster(raster_folder, raster_file)
    new_raster_data, new_meta_data = reproject_raster(raster_data, metadata, '6345', '26916')
    
    
    water_level_folder = '/media/weiwang/easystore/NAIP/Waterlevel/'
    water_level_prefix = 'MIC'
    water_level_data = merge_excel_files(water_level_folder, water_level_prefix, '.csv')
    
    
    line_to_analysis = extract_transect_line(transects, 0, 10, site_num)
    elev_vals, dists =extract_raster_profile_from_metadata(new_raster_data, new_meta_data, line_to_analysis, num_points=100)
    
    
    intersecton_info = extract_transect_line(intersection, 0, 10, site_num, image_col="ImageID")
    refined_dists, refined_time_intervals = [],[]
    for i in range(len(intersecton_info)):
        temp_intersect = intersecton_info.iloc[i]
        temp_year = temp_intersect['Year']
        temp_dist = temp_intersect['Dist']
        site_date = site_date_info[str(site_num)].get(str(temp_year))
        site_val = float(get_verified_value(water_level_data, site_date))
        target_dists = find_elevation_intersections(elev_vals, dists, site_val)
        target_dists = [300-x for x in target_dists]
        refined_dist = refine_intersection_points(target_dists, temp_dist)
        refined_dists.append(refined_dist)
        refined_time_intervals.append(days_difference('20000101', site_date))
        
    water_level_rate = calculate_rate(refined_time_intervals, refined_dists)
    orig_rate = line_to_analysis.ChangeRate.iloc[0]
    calibrate_rate = orig_rate - water_level_rate
