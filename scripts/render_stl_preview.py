"""Render STL to PNG for README preview. Uses matplotlib (no OpenGL needed)."""
import os
import sys

def find_stl():
    for root, _, files in os.walk("."):
        for f in files:
            if f.lower().endswith(".stl"):
                return os.path.join(root, f)
    return None

def main():
    try:
        import trimesh
        import matplotlib.pyplot as plt
        from mpl_toolkits.mplot3d import Axes3D
        import numpy as np
    except ImportError:
        print("pip install trimesh matplotlib numpy")
        sys.exit(1)

    stl_path = find_stl()
    if not stl_path or not os.path.exists(stl_path):
        print("No STL found")
        sys.exit(1)

    mesh = trimesh.load(stl_path)
    if isinstance(mesh, trimesh.Scene):
        mesh = mesh.dump(concatenate=True)

    verts = mesh.vertices
    faces = mesh.faces

    fig = plt.figure(figsize=(8, 6))
    ax = fig.add_subplot(111, projection="3d")
    ax.plot_trisurf(
        verts[:, 0], verts[:, 1], verts[:, 2],
        triangles=faces, color="#5588FF", alpha=0.9, shade=True
    )
    ax.set_axis_off()
    ax.view_init(elev=20, azim=45)
    plt.tight_layout(pad=0)
    os.makedirs("docs", exist_ok=True)
    out = "docs/caja_preview.png"
    plt.savefig(out, bbox_inches="tight", pad_inches=0.1, dpi=120)
    plt.close()
    print(f"Saved {out}")

if __name__ == "__main__":
    main()
