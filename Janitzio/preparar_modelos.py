"""Prepara las mallas y texturas locales para el mapa. Requiere Pillow y NumPy."""

import argparse
import struct
from pathlib import Path

import numpy as np
from PIL import Image, ImageOps


SIZE = 512


def obj_index(field, count):
    index = int(field) if field else 0
    return index-1 if index > 0 else count+index if index < 0 else -1


def obj(path):
    positions, normals, uv, faces = [], [], [], []
    group, material = "", ""
    with path.open(encoding="utf-8", errors="replace") as source:
        for line in source:
            tag, _, value = line.strip().partition(" ")
            if tag == "v":
                positions.append(tuple(map(float, value.split()[:3])))
            elif tag == "vn":
                normals.append(tuple(map(float, value.split()[:3])))
            elif tag == "vt":
                uv.append(tuple(map(float, value.split()[:2])))
            elif tag in ("g", "o"):
                group = value
            elif tag == "usemtl":
                material = value
            elif tag == "f":
                face = []
                for corner in value.split():
                    fields = (corner.split("/") + ["", ""])[:3]
                    sizes = (len(positions), len(uv), len(normals))
                    face.append(tuple(obj_index(field, count) for field, count in zip(fields, sizes)))
                faces.append((group, material, face))
    return np.asarray(positions).reshape(-1, 3), np.asarray(normals).reshape(-1, 3), uv, faces


def mesh(data, select, layers, rotation=None, bark=None):
    positions, normals, uv, faces = data
    if rotation is not None:
        positions = positions @ rotation.T
        normals = normals @ rotation.T
    faces = [item for item in faces if select(*item)]
    if not faces:
        raise ValueError("La malla no tiene caras")
    points = positions[[corner[0] for _, _, face in faces for corner in face]]
    low, high = points.min(axis=0), points.max(axis=0)
    origin = (high + low) * 0.5
    origin[1] = low[1]
    scale = high - low
    positions = (positions - origin) / scale
    normals = normals * scale
    if bark is not None:
        # Agrupa la corteza por celdas y conserva los planos completos de las hojas.
        cells = np.floor(positions * np.array([34, 49, 34])).astype(int)
        _, cell_ids, inverse = np.unique(cells, axis=0, return_index=True, return_inverse=True)
        bark_positions = cell_ids[inverse]

    vertices, indices, cache = [], [], {}
    for face_id, (_, material, face) in enumerate(faces):
        layer = layers[material]
        corners = []
        if any(corner[2] < 0 for corner in face):
            a, b, c = (positions[corner[0]] for corner in face[:3])
            face_normal = np.cross(b-a, c-a)
        for p, t, n in face:
            if material == bark:
                p = int(bark_positions[p])
                # Una normal y una coordenada continuas evitan grietas al soldar ramas.
                key = (p, layer)
            else:
                key = (p, t, n if n >= 0 else -face_id-1, layer)
            if key not in cache:
                point = positions[p]
                if material == bark:
                    tex = (np.arctan2(point[2], point[0]) / (2*np.pi) + 0.5, point[1] * 3)
                    normal = (point[0], 0.12, point[2])
                else:
                    tex = uv[t] if t >= 0 else (0, 0)
                    normal = normals[n] if n >= 0 else face_normal
                cache[key] = len(vertices)
                vertices.append((*point, *normal, *tex, layer))
            corners.append(cache[key])
        for i in range(1, len(corners)-1):
            triangle = corners[0], corners[i], corners[i+1]
            if len(set(triangle)) == 3:
                indices.extend(triangle)
    return np.asarray(vertices, dtype="<f4"), np.asarray(indices, dtype="<u4")


def simplify(model, resolution):
    """Suelda posiciones cercanas, conservando normales y UV de cada material."""
    vertices, indices = model
    positions = vertices[:, :3]
    cells = np.floor((positions + (0.5, 0, 0.5)) * resolution + 0.5).astype(int)
    _, groups = np.unique(cells, axis=0, return_inverse=True)
    group_counts = np.bincount(groups)
    centers = np.column_stack([
        np.bincount(groups, weights=axis) / group_counts for axis in positions.T
    ])
    triangles = indices.reshape(-1, 3)
    corners = groups[triangles]
    keep = (corners[:, 0] != corners[:, 1]) & (corners[:, 1] != corners[:, 2]) & (corners[:, 2] != corners[:, 0])
    triangles, corners = triangles[keep], corners[keep]
    # Las caras coincidentes de detalles pequeños ya se representan con una sola cara.
    keys = np.column_stack((np.sort(corners, axis=1), vertices[triangles[:, 0], 8]))
    _, unique = np.unique(keys, axis=0, return_index=True)
    used, remapped = np.unique(triangles[np.sort(unique)].ravel(), return_inverse=True)
    simplified = vertices[used].copy()
    simplified[:, :3] = centers[groups[used]]
    return simplified, remapped.astype("<u4")


def simplify_layer(model, material, resolution):
    vertices, indices = model
    faces = indices.reshape(-1, 3)
    selected = vertices[faces[:, 0], 8] == material

    def compact(subset):
        used, remapped = np.unique(subset.ravel(), return_inverse=True)
        return vertices[used], remapped.astype("<u4")

    rest, rest_indices = compact(faces[~selected])
    detail, detail_indices = simplify(compact(faces[selected]), resolution)
    return np.concatenate((rest, detail)), np.concatenate((rest_indices, detail_indices + len(rest)))


def boat_models(data, material):
    positions, _, _, faces = data
    parents = list(range(len(positions)))

    def root(i):
        while parents[i] != i:
            parents[i] = parents[parents[i]]
            i = parents[i]
        return i

    for _, _, face in faces:
        for corner in face[1:]:
            parents[root(corner[0])] = root(face[0][0])
    oars = {root(i) for i, p in enumerate(positions) if abs(p[0]) > 2.25}
    docked = mesh(data, lambda _, material, face: root(face[0][0]) not in oars, material)
    underway = mesh(data, lambda *_: True, material)
    return docked, underway


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("descargas", type=Path)
    parser.add_argument("extraidos", type=Path, help="Carpeta con los RAR extraídos, incluido obj/textures.rar")
    parser.add_argument("salida", type=Path)
    args = parser.parse_args()
    images = []

    def layer(path, opacity=None, color=None, amount=0.0):
        image = Image.open(args.extraidos / path).convert("RGBA").resize((SIZE, SIZE), Image.Resampling.LANCZOS)
        if color is not None:
            tinted = ImageOps.colorize(image.convert("L"), color[0], color[1]).convert("RGBA")
            image = Image.blend(image,tinted,amount)
        if opacity is not None:
            alpha = Image.open(args.extraidos / opacity).convert("L").resize((SIZE, SIZE), Image.Resampling.LANCZOS)
            image.putalpha(alpha)
        images.append(image)
        return len(images)-1

    plaster = ("#877268", "#fff1df")
    terracotta = ("#943e24", "#f2ad68")
    poor_layers = {
        "House": layer("Textures/House_BC.tga",color=plaster,amount=0.85),
        "WoodDetail": layer("Textures/WoodDetail_BC.tga"),
        "3_SET": layer("Textures/3_SET_BC.tga"),
        "ROOF.001": layer("Textures/WoodRoofTiles (2).jpg",color=terracotta,amount=0.87),
    }
    hoi_layers = {}
    for key in ("Base", "DoorwWindow", "RailingClumn", "RoofBase", "RoofTiles", "Walls"):
        filename = f"Texture_M2/TB_{key}_Texture.002_BaseColor.jpg"
        if key in ("RoofBase","RoofTiles"):
            index = layer(filename,color=terracotta,amount=0.88)
        elif key == "Walls":
            index = layer(filename,color=plaster,amount=0.90)
        else:
            index = layer(filename)
        hoi_layers[key + "_Texture.002"] = index
    old_layers = {
        "Material": layer("obj/textures/Medieval_Brick_Texture_by_goodtextures.jpg",color=plaster,amount=0.84),
        "Material.001": layer("obj/textures/WoodPlanksBare0051_3_M.jpg"),
        "Material.002": layer("obj/textures/windows_0279_01_preview.jpg"),
        "Teja": layer("obj/textures/WoodPlanksBare0051_3_M.jpg",color=terracotta,amount=0.94),
    }
    bark, leaves = "aiStandardSurface1SG", "aiStandardSurface3SG"
    bark_layer = layer("Maple/Maps/Bark004_2K_Color.jpg")
    leaf_path = "Maple/Maps/Maple_Leaf_Colour_Red.png"
    opacity_path = "Maple/Maps/Maple_Leaf_Opacity.png"
    leaf_layers = (
        layer(leaf_path,opacity_path,("#832319","#ffb552"),0.71),
        layer(leaf_path,opacity_path,("#8c3917","#ffce64"),0.86),
    )
    poor = obj(args.descargas / "Poor+man's+house.obj")
    hoi = obj(args.descargas / "HoiAnHouse_M2.obj")
    poor_rotation = np.array([[0, 1, 0], [0, 0, 1], [1, 0, 0]])
    old_p, old_n, old_uv, old_faces = obj(args.extraidos / "obj/house.obj")
    roof_groups = {"Cube.002_Cube.001", "Cube.001"}
    old_faces = [(group,"Teja" if group in roof_groups else material,face) for group,material,face in old_faces]
    models = [
        mesh((old_p,old_n,old_uv,old_faces), lambda _, material, face: material in old_layers, old_layers),
        mesh(poor, lambda group, *_: not group.startswith(("Fabrick", "meshok")), poor_layers, poor_rotation),
        mesh(hoi, lambda *_: True, hoi_layers),
    ]
    maple = obj(args.descargas / "Maple_Trees.obj")
    for index, name in enumerate(("Maple_01", "Maple_02")):
        first = min(corner[0] for group,material,face in maple[3]
                    if group.partition(" ")[0] == name and material == leaves for corner in face)

        def select(group, material, face):
            if group.partition(" ")[0] != name:
                return False
            if material == bark:
                return True
            # Cada hoja tiene dos caras consecutivas que comparten cuatro vértices.
            return ((min(v[0] for v in face) - first) // 4) % 3 != 0

        models.append(mesh(maple, select, {bark:bark_layer,leaves:leaf_layers[index]}, bark=bark))

    def poor_exterior(group, material, face):
        if group.startswith(("Walls_", "doors_")):
            return True
        if not group.startswith(("roof_", "WoodDetail_", "3_set_")):
            return False
        a, b, c = (poor[0][corner[0]] for corner in face[:3])
        normal = np.cross(b-a,c-a)
        area = np.linalg.norm(normal)
        if group.startswith("roof_"):
            return area > 0.0 and normal[2] > 0.15*area
        return area > 1100.0

    # Conserva los planos exteriores y las tejas originales en lugar de soldarlos entre sí.
    poor_compact = mesh(poor, poor_exterior, poor_layers, poor_rotation)
    hoi_compact = mesh(hoi, lambda _, material, face: material != "RoofTiles_Texture.002", hoi_layers)
    models.extend((poor_compact,simplify_layer(hoi_compact,hoi_layers["DoorwWindow_Texture.002"],19)))
    boat_layer = layer("Textures/Mesh_Base_Color.PNG")
    models.extend(boat_models(obj(args.descargas / "BoatOBJ.obj"), {"Material": boat_layer}))

    args.salida.mkdir(parents=True, exist_ok=True)
    atlas = Image.new("RGBA", (SIZE, SIZE*len(images)))
    for i, image in enumerate(images):
        atlas.paste(image, (0, i*SIZE))
    atlas.save(args.salida / "materiales.png")
    with (args.salida / "modelos.bin").open("wb") as destination:
        destination.write(struct.pack("<4sIIII", b"JTZ3", len(models), SIZE, len(images), old_layers["Material.002"]))
        for vertices, indices in models:
            destination.write(struct.pack("<II", len(vertices), len(indices)))
            destination.write(vertices.tobytes())
            destination.write(indices.tobytes())
    print("Triángulos por modelo:", ", ".join(str(len(indices)//3) for _, indices in models))


if __name__ == "__main__":
    main()
