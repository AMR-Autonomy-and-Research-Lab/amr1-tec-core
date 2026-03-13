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

    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, projection="3d")
    ax.plot_trisurf(
        verts[:, 0], verts[:, 1], verts[:, 2],
        triangles=faces, color="#4a7ac4", alpha=0.95, shade=True, edgecolor="none"
    )
    ax.set_axis_off()
    ax.set_facecolor("#f8f9fa")
    ax.view_init(elev=25, azim=55)
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
    os.makedirs("docs", exist_ok=True)
    out = "docs/caja_preview.png"
    plt.savefig(out, bbox_inches="tight", pad_inches=0.15, dpi=150, facecolor="#f8f9fa")
    plt.close()
    print(f"Saved {out}")

if __name__ == "__main__":
    main()
