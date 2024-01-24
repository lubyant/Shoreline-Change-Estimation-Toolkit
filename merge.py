from osgeo import ogr, osr
import os

def batch_merge(func):
    def wrapper(shapefiles, output_path, target_epsg):
        shapefiles.sort()
        num = len(shapefiles)
        size = num // 20 + 1
        for i in range(size):
            batch_shapefiles = []
            batch_output_path = f"{output_path.split('.')[0]}_{i}.shp"
            for _ in range(20):
                if not shapefile_paths:
                    break
                batch_shapefiles.append(shapefile_paths.pop())
            print(batch_shapefiles, batch_output_path)
            func(batch_shapefiles, batch_output_path, target_epsg)
    return wrapper

def add_shapefiles_from_folder(folder, shapefile_paths, field="transect"):
    for item in os.listdir(folder):
        full_path = os.path.join(folder, item)
        if os.path.isdir(full_path):
            # Recursive call for subfolders
            add_shapefiles_from_folder(full_path, shapefile_paths)
        elif item.endswith(".shp") and field in item:
            shapefile_paths.append(full_path)


def reproject_layer(layer, target_srs):
    """Reproject a layer to the target spatial reference system (SRS)"""
    # Create a new layer in memory
    mem_driver = ogr.GetDriverByName("Memory")
    mem_ds = mem_driver.CreateDataSource("out")
    reprojected_layer = mem_ds.CreateLayer(
        layer.GetName(), srs=target_srs, geom_type=layer.GetGeomType()
    )

    # Reproject each feature
    for feature in layer:
        geom = feature.GetGeometryRef()
        geom.TransformTo(target_srs)
        feature.SetGeometry(geom)
        reprojected_layer.CreateFeature(feature)

    return reprojected_layer

@batch_merge
def merge_shapefiles(shapefiles, output_path, target_epsg):
    """Merge shapefiles with reprojection to a target EPSG"""
    driver = ogr.GetDriverByName("ESRI Shapefile")
    target_srs = osr.SpatialReference()
    target_srs.ImportFromEPSG(target_epsg)

    # Create the output shapefile
    output_ds = driver.CreateDataSource(output_path)
    output_layer = None

    for shp in shapefiles:
        print(shp)
        if not shp:
            continue
        ds = ogr.Open(shp)
        layer = ds.GetLayer()
        epsg_code = int(layer.GetSpatialRef().GetAuthorityCode(None))

        # Reproject the layer
        if epsg_code != target_epsg:
            # Create a new layer in memory
            mem_driver = ogr.GetDriverByName("Memory")
            mem_ds = mem_driver.CreateDataSource("out")
            target_layer = mem_ds.CreateLayer(
                layer.GetName(), srs=target_srs, geom_type=layer.GetGeomType()
            )

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
                geom_type=target_layer.GetGeomType(),
            )
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
lakes = ["LakeMichigan", "LakeErie", "LakeSuperior", "LakeHuron", "LakeOntario"]

for i in range(5):
    shapefile_paths = []

    print(f"merge lakes: {lakes[i]}")
    add_shapefiles_from_folder(f"ErosionFiles/{lakes[i]}", shapefile_paths)

    # Output path for the merged shapefile
    output_shapefile = f"MergeShp1/{lakes[i]}.shp"

    # # Target EPSG code (e.g., WGS 84 is 4326)
    target_epsg = 26917

    # # Merge the shapefiles
    merge_shapefiles(shapefile_paths, output_shapefile, target_epsg)
