from osgeo import ogr, osr
import os

shapefile_paths = []


def add_shapefiles_from_folder(folder):
    for item in os.listdir(folder):
        full_path = os.path.join(folder, item)
        if os.path.isdir(full_path):
            # Recursive call for subfolders
            shapefile_paths.append(add_shapefiles_from_folder(full_path))
        elif item.endswith(".shp") and "transect" in item:
            return full_path


def reproject_layer(layer, target_srs):
    """Reproject a layer to the target spatial reference system (SRS)"""
    # Create a new layer in memory
    mem_driver = ogr.GetDriverByName('Memory')
    mem_ds = mem_driver.CreateDataSource('out')
    reprojected_layer = mem_ds.CreateLayer(layer.GetName(),
                                           srs=target_srs,
                                           geom_type=layer.GetGeomType())

    # Reproject each feature
    for feature in layer:
        geom = feature.GetGeometryRef()
        geom.TransformTo(target_srs)
        feature.SetGeometry(geom)
        reprojected_layer.CreateFeature(feature)

    return reprojected_layer


def merge_shapefiles(shapefiles, output_path, target_epsg):
    """Merge shapefiles with reprojection to a target EPSG"""
    driver = ogr.GetDriverByName('ESRI Shapefile')
    target_srs = osr.SpatialReference()
    target_srs.ImportFromEPSG(target_epsg)

    # Create the output shapefile
    output_ds = driver.CreateDataSource(output_path)
    output_layer = None

    for shp in shapefiles:
        if not shp:
            continue
        ds = ogr.Open(shp)
        layer = ds.GetLayer()
        epsg_code = int(layer.GetSpatialRef().GetAuthorityCode(None))

        # Reproject the layer
        if epsg_code != target_epsg:
            # Create a new layer in memory
            mem_driver = ogr.GetDriverByName('Memory')
            mem_ds = mem_driver.CreateDataSource('out')
            target_layer = mem_ds.CreateLayer(layer.GetName(),
                                              srs=target_srs,
                                              geom_type=layer.GetGeomType())

            layer_defn = layer.GetLayerDefn()
            for i in range(layer_defn.GetFieldCount()):
                field_defn = layer_defn.GetFieldDefn(i)
                target_layer.CreateField(field_defn)

            # Reproject each feature
            for feature in layer:
                geom = feature.GetGeometryRef()
                geom.TransformTo(target_srs)
                new_feature = ogr.Feature(target_layer.GetLayerDefn())
                new_feature.SetGeometry(geom)
                for i in range(layer_defn.GetFieldCount()):
                    new_feature.SetField(i, feature.GetField(i))
                target_layer.CreateFeature(new_feature)
                new_feature = None
            # target_layer = reproject_layer(layer, target_srs)
        else:
            target_layer = layer

        if output_layer is None:
            # Create output layer
            output_layer = output_ds.CreateLayer(
                target_layer.GetName(),
                srs=target_srs,
                geom_type=target_layer.GetGeomType())
            for i in range(target_layer.GetLayerDefn().GetFieldCount()):
                field_defn = target_layer.GetLayerDefn().GetFieldDefn(i)
                output_layer.CreateField(field_defn)

        # Copy features to the output layer
        for feature in target_layer:
            output_feature = ogr.Feature(output_layer.GetLayerDefn())
            output_feature.SetFrom(feature)
            output_layer.CreateFeature(output_feature)
            output_feature = None

        ds = None  # Close the file

    output_ds = None  # Close the output file


# List of shapefile paths
add_shapefiles_from_folder("ErosionFiles/LakeErie")

# Output path for the merged shapefile
output_shapefile = 'MergeShp/LakeErie.shp'

# # Target EPSG code (e.g., WGS 84 is 4326)
target_epsg = 26917

# # Merge the shapefiles
merge_shapefiles(shapefile_paths, output_shapefile, target_epsg)
