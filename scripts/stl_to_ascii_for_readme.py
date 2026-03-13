"""Convert binary STL to ASCII for GitHub README embed. Decimates if too large."""
import os
import sys

def find_stl():
    for root, _, files in os.walk("."):
        for f in files:
            if f.lower().endswith(".stl") and "CajaNueva" in f:
                return os.path.join(root, f)
    for root, _, files in os.walk("."):
        for f in files:
            if f.lower().endswith(".stl"):
                return os.path.join(root, f)
    return None

def main():
    try:
        import trimesh
        from trimesh.exchange.stl import export_stl_ascii
    except ImportError:
        print("pip install trimesh")
        sys.exit(1)

    stl_path = find_stl()
    if not stl_path or not os.path.exists(stl_path):
        print("No STL found")
        sys.exit(1)

    mesh = trimesh.load(stl_path)
    if isinstance(mesh, trimesh.Scene):
        mesh = mesh.dump(concatenate=True)
    mesh.merge_vertices(merge_norm=True, merge_tex=True)

    # Reducir caras si es muy densa (la malla de caja no admite <~1800 caras por topología).
    TARGET_FACES = 2000
    n_faces = len(mesh.faces)
    if n_faces > TARGET_FACES:
        try:
            import pyfqmr
            vertices = mesh.vertices.astype("float64")
            faces = mesh.faces.astype("int32")
            simplifier = pyfqmr.Simplify()
            simplifier.setMesh(vertices, faces)
            simplifier.simplify_mesh(target_count=TARGET_FACES, aggressiveness=7, verbose=0)
            verts_out, faces_out, _ = simplifier.getMesh()
            if len(faces_out) > 0:
                mesh = trimesh.Trimesh(vertices=verts_out, faces=faces_out)
        except Exception as e:
            print(f"Simplificación falló ({e}), usando malla completa")

    ascii_stl = export_stl_ascii(mesh)
    size_kb = len(ascii_stl) / 1024
    print(f"ASCII STL: {len(ascii_stl):,} chars ({size_kb:.0f} KB), {len(mesh.faces)} faces")

    os.makedirs("docs", exist_ok=True)
    out = "docs/caja_ascii.stl"
    with open(out, "w") as f:
        f.write(ascii_stl)
    print(f"Saved {out}")

    # Also output for README (first 50 lines as preview of format)
    lines = ascii_stl.split("\n")
    print(f"Lines: {len(lines)}")

if __name__ == "__main__":
    main()
