# ITK-SNAP 3D Editing Tool

[![Build and package](https://github.com/HUmutI/itksnap-3d-edit/actions/workflows/build.yml/badge.svg)](https://github.com/HUmutI/itksnap-3d-edit/actions/workflows/build.yml)

A fork of [ITK-SNAP](https://github.com/pyushkevich/itksnap) that adds **segmentation editing
directly in the 3D view**.

ITK-SNAP lets you paint a segmentation slice by slice in the 2D views and then render it as a 3D
surface. Until now, fixing a mistake you can only *see* in 3D — a floating fragment, a leak, a hole,
a branch that broke off — meant hunting back through 2D slices to find the voxels responsible.

This fork lets you fix those defects where you see them.

![3D editing tool demo](docs/3d-editing-demo.gif)

> **Note:** the demo above shows the tool on a pediatric chest CT airway segmentation, which is the
> workflow it was built for. Nothing about it is airway-specific.

---

## What it adds

A new **3D Editing Tool** in the 3D view toolbar, with four sub-tools:

| Sub-tool | What it does |
|---|---|
| **Brush** | Left-drag paints on the rendered surface, right-drag erases. Size down to a single voxel. |
| **Delete island** | Click a disconnected fragment and its entire connected component is removed. |
| **Bridge** | Click two points to join separated structures with a tube. |
| **Fill hole** | Click a gap to close it with a local morphological closing. |

### Why the brush can *add* material

The picking is the interesting part. ITK-SNAP's existing 3D tools locate a click by ray-marching the
label image, which can only ever return a voxel that is *already labeled* — which is why the existing
spray tool can only thicken a structure by one voxel and can never bridge a gap.

This tool picks against the **rendered mesh** with a `vtkCellPicker` instead. The hit point lies on
the marching-cubes isosurface, i.e. *between* labeled and unlabeled voxels, so a ball centred there
covers unlabeled voxels and the surface advances outward. Dragging sweeps a capsule between
successive samples, which is what makes it possible to draw across a gap.

The old ray-march is kept as a fallback for when the mesh is stale or absent — for example right
after painting in 2D, before you press **update**.

### Controls

| Input | Action |
|---|---|
| Left-drag | Paint (Brush sub-tool); click for the other sub-tools |
| Right-drag | Erase — Brush sub-tool only; erases only voxels of the active label |
| Middle-drag | Rotate camera |
| Shift / Ctrl + left-drag | Pan / spin camera |
| Wheel | Zoom |
| `[` `]` | Brush size |
| `,` `.` | Depth along the view ray |
| `Esc` | Cancel a pending bridge endpoint |
| `Ctrl+Z` / `Cmd+Z` | Undo — one step per stroke |

**Brush size** is a diameter in voxels: size 1 paints exactly one voxel. The keys work once the
pointer is over the 3D view. Undo is disabled while a stroke is in progress.

A wireframe sphere follows the cursor showing the footprint that will actually be painted.

Nothing is bound to <kbd>Alt</kbd>, and no binding *requires* the keyboard or the mouse wheel: on
ITK-SNAP's offscreen (OSMesa) widget backend neither keys nor the wheel reach the 3D view, so every
setting is also available in the inspector panel.

### Notes on behaviour

- **Set an active drawing label first.** The tools work with the active label: the brush paints and
  erases it, and bridge and fill-hole add it. With "Clear Label" selected there is nothing to paint,
  erase or add, so the tool refuses and explains instead.
- **Strokes and bridges are always connected.** The brush stroke and the bridge tube are guaranteed
  6-connected even at size 1, so a thin connection you draw is a connection the segmentation
  actually has.
- **Delete island** removes the 6-connected component of the label you clicked — the mesh under the
  cursor decides which label, so touching structures of different labels are not confused. Anything
  over 20 000 voxels asks for confirmation first ("at least N voxels"), so a misclick on the main
  structure cannot silently wipe it out.
- **Fill hole** only fills background (unlabeled) voxels, ignores the draw-over setting, and only
  within twice the closing radius of the click. It never touches other labels and never fills a hole
  you did not point at.
- **Undo is per stroke**, and while the tool is active the 3D mesh refreshes on every change,
  including undo and redo.
- **Isotropic mode is off by default**, matching the 2D paintbrush. With it on, the radius is
  measured in units of the *smallest* voxel spacing, which on a typical chest CT (~0.9 × 0.9 × 0.5 mm)
  collapses a small brush into a needle along z.
- **Tool settings reset on each launch** (size, depth, sub-tool), like the 2D paintbrush.

### Current limitation

After each edit the mesh is rebuilt in full, which takes a few seconds on a large structure. A
localized re-mesh (marching cubes over only the edited region, spliced into the cached surface) is
designed but not yet implemented.

---

## Download

Prebuilt packages are on the
[**Releases** page](https://github.com/HUmutI/itksnap-3d-edit/releases):

| Platform | File |
|---|---|
| macOS, Apple Silicon | `itksnap-*-Darwin-arm64.dmg` |
| macOS, Intel | `itksnap-*-Darwin-x86_64.dmg` |
| Ubuntu / Linux, x86_64 | `itksnap-*-Linux-x86_64.tar.gz` |
| Windows, x64 (best effort) | `itksnap-*-win64-AMD64.exe` installer |

Every CI run on the [Actions tab](https://github.com/HUmutI/itksnap-3d-edit/actions) also uploads
the same packages as build artifacts, so the latest commit is always downloadable.

The Windows installer is produced by CI the same way upstream ITK-SNAP builds it, but the fork's
authors do not use or test Windows; macOS and Ubuntu are the supported platforms.

### macOS

Open the `.dmg` and drag `ITK-SNAP.app` to `/Applications`. The app is ad-hoc signed, not
notarized, so the first launch needs one extra step: **right-click → Open**, or

```sh
xattr -dr com.apple.quarantine /Applications/ITK-SNAP.app
```

### Ubuntu / Linux

The tarball is self-contained (Qt is bundled) and runs from any directory:

```sh
tar xzf itksnap-*-Linux-x86_64.tar.gz
cd itksnap-*-Linux-x86_64
bin/itksnap
```

`bin/itksnap` is a small launcher that runs the real binary in `lib/snap-<version>/`. It was built
on Ubuntu 22.04 and runs on 22.04 or newer. On a bare system install the runtime libraries Qt needs:

```sh
sudo apt install libgl1 libglu1-mesa libxkbcommon-x11-0 libxcb-cursor0 libxcb-icccm4 \
  libxcb-keysyms1 libxcb-shape0 libxcb-xinerama0 libfontconfig1
```

---

## Building from source

The build is the standard ITK-SNAP build; the tool adds no new dependencies. Everything below runs
from **one workspace directory** — the repository, ITK, VTK and the build directories all sit next to
each other and nothing is ever built inside the source tree.

### Requirements

| | Version | Notes |
|---|---|---|
| CMake | ≥ 3.16 | |
| C++ | C++17 | |
| **ITK** | 5.4.0 (≥ 5.4 required) | Must be built with `Module_MorphologicalContourInterpolation=ON` |
| **VTK** | 9.5.2 (≥ 9.3.1 required) | Must be built with Qt support and `RenderingExternal` |
| **Qt** | ≥ 6.7 | 6.8.1 in CI; tested with Homebrew 6.11. Needs `qtbase`, `qtdeclarative` (for Qt6Qml) and `qttools` (for LinguistTools) |
| libssh, libcurl | any recent | |

**Do not use a distribution's packaged ITK.** ITK-SNAP requires the
`MorphologicalContourInterpolation` remote module, which is off by default and is not enabled in
Homebrew's or apt's ITK builds. VTK likewise needs `VTK_GROUP_ENABLE_Qt=YES`, which packaged builds
often omit.

### 1. Workspace and clone

```sh
mkdir ~/itksnap-ws && cd ~/itksnap-ws

git clone https://github.com/HUmutI/itksnap-3d-edit.git
git -C itksnap-3d-edit submodule update --init
```

Stay in `~/itksnap-ws` for every command that follows. The submodules (`c3d`, `greedy`,
`digestible`) are required; the build fails without them.

### 2. Platform packages

<details open>
<summary><b>macOS</b> (verified on Apple Silicon, macOS 26)</summary>

```sh
brew install cmake ninja qtbase qtdeclarative qttools libssh
```

Homebrew's Qt is found automatically through `-DQt6_DIR` in step 4.
</details>

<details>
<summary><b>Ubuntu / Linux</b></summary>

```sh
sudo apt install build-essential cmake ninja-build git python3-pip \
  libssh-dev libcurl4-openssl-dev libgl1-mesa-dev libglu1-mesa-dev \
  libxkbcommon-x11-0 libxcb-cursor0 libxcb-icccm4 libxcb-keysyms1 \
  libxcb-shape0 libxcb-xinerama0 libfontconfig1-dev
```

**apt's Qt is too old** on every current LTS (6.2 on 22.04, 6.4 on 24.04; the build needs ≥ 6.7).
Install Qt 6.8.1 with [`aqtinstall`](https://github.com/miurahr/aqtinstall) instead, the same way
CI does:

```sh
python3 -m pip install aqtinstall
python3 -m aqt install-qt linux desktop 6.8.1 linux_gcc_64 -O ~/Qt
```

Then add `-DCMAKE_PREFIX_PATH=$HOME/Qt/6.8.1/gcc_64` to **both** the VTK configure (step 3) and the
ITK-SNAP configure (step 4). Only on Ubuntu 25.04 or newer can you use `apt install qt6-base-dev
qt6-declarative-dev qt6-tools-dev qt6-tools-dev-tools` and skip this.
</details>

### 3. Build VTK and ITK

Same on both platforms. Build these once; they take considerably longer than ITK-SNAP itself.

```sh
# VTK 9.5.2  (Ubuntu: add -DCMAKE_PREFIX_PATH=$HOME/Qt/6.8.1/gcc_64)
git clone --depth 1 --branch v9.5.2 --recurse-submodules \
    https://gitlab.kitware.com/vtk/vtk.git vtk-src
cmake -S vtk-src -B vtk-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF -DBUILD_SHARED_LIBS=OFF \
  -DVTK_GROUP_ENABLE_Qt=YES \
  -DVTK_MODULE_ENABLE_VTK_RenderingExternal=WANT \
  -DVTK_MODULE_ENABLE_VTK_GUISupportQtQuick=NO \
  -DVTK_MODULE_ENABLE_VTK_GUISupportQtSQL=NO \
  -DCMAKE_INSTALL_PREFIX=$PWD/vtk-install
cmake --build vtk-build --target install --parallel

# ITK 5.4.0
git clone --depth 1 --branch v5.4.0 \
    https://github.com/InsightSoftwareConsortium/ITK.git itk-src
cmake -S itk-src -B itk-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF -DBUILD_SHARED_LIBS=OFF \
  -DModule_MorphologicalContourInterpolation=ON
cmake --build itk-build --parallel
```

`ITK_DIR` points at the ITK **build** directory, not an install prefix — ITK's install step misses a
header (`vnl_vector_ref.hxx`), which is why ITK-SNAP's own CI does the same.

### 4. Build ITK-SNAP

<details open>
<summary><b>macOS</b></summary>

```sh
cmake -S itksnap-3d-edit -B itksnap-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DITK_DIR=$PWD/itk-build \
  -DVTK_DIR=$PWD/vtk-install/lib/cmake/vtk-9.5 \
  -DQt6_DIR=$(brew --prefix)/lib/cmake/Qt6
cmake --build itksnap-build --parallel
```

The binary is `itksnap-build/ITK-SNAP`. To produce the `.app` bundle and `.dmg`:

```sh
cmake --build itksnap-build --target package
```

The package target signs the bundle ad-hoc (no developer certificate needed), so the resulting
`ITK-SNAP.app` launches as-is — see the [macOS download notes](#macos) for the first-launch
quarantine step. CPack stages the bundle at
`itksnap-build/_CPack_Packages/Darwin-arm64/Bundle/itksnap-*/ITK-SNAP.app` before building the
`.dmg`; the `.dmg` step drives Finder via AppleScript and can fail on a headless session, but the
staged `.app` is complete regardless.

Only if you copy an *unpackaged* build somewhere else do you need to re-sign it yourself:

```sh
codesign --force --deep --sign - /path/to/ITK-SNAP.app
```

Do not stage the bundle inside an iCloud-synced folder such as `~/Desktop` or `~/Documents`. iCloud
sets `com.apple.FinderInfo` on the bundle directories, and `codesign` then refuses with
*"resource fork, Finder information, or similar detritus not allowed"*.
</details>

<details>
<summary><b>Ubuntu / Linux</b></summary>

```sh
cmake -S itksnap-3d-edit -B itksnap-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DITK_DIR=$PWD/itk-build \
  -DVTK_DIR=$PWD/vtk-install/lib/cmake/vtk-9.5 \
  -DCMAKE_PREFIX_PATH=$HOME/Qt/6.8.1/gcc_64
cmake --build itksnap-build --parallel
```

Run it with `itksnap-build/ITK-SNAP`. A relocatable tarball (the same one CI publishes) comes from
`cmake --build itksnap-build --target package`.
</details>

### 5. Verify

```sh
cd itksnap-build
ctest -LE Remote                 # Linux: xvfb-run -a ctest -LE Remote
```

The `Remote` tests are excluded because they need network access; the GUI tests need a display.
`ctest -R SegmentationEdit3DTest` runs the unit test for the 3D editing operations themselves —
stamp, stroke connectivity, island deletion and hole filling — without a GUI.

---

## Using the tool

1. Open your image (**File → Open Main Image**) and segmentation
   (**Segmentation → Open Segmentation**).
2. Pick your label under **Segmentation Labels → Active label**.
3. Press **update** in the 3D panel to build the mesh.
4. Choose the tool: **Tools → Active 3D Tool → 3D Editing Tool**, or the button in the left panel's
   3D toolbox.
5. Select a sub-tool (**Brush / Delete island / Bridge / Fill hole**) in the *3D Editing Tool
   Inspector* panel and edit.

### If parts look disconnected but the tools disagree

ITK-SNAP applies Gaussian smoothing (σ = 0.8 by default) before marching cubes. Connections that are
one voxel thick fall below the isosurface threshold and vanish from the rendering, so a structure can
*look* broken while its voxels are still connected — in which case **Delete island** will correctly
report the whole structure as one component.

To see the true voxel topology, turn Gaussian smoothing off under
**Preferences → 3D Rendering**, then press **update**.

To check from the command line, using the `c3d` binary this repository builds:

```sh
c3d your_mask.nii.gz -comp -dup -lstat
```

The component sizes tell you immediately whether a fragment is genuinely separate.

---

## Citing

If you use ITK-SNAP in published work, please cite:

> Paul A. Yushkevich, Joseph Piven, Heather Cody Hazlett, Rachel Gimpel Smith, Sean Ho, James C. Gee,
> and Guido Gerig. *User-guided 3D active contour segmentation of anatomical structures:
> Significantly improved efficiency and reliability.* Neuroimage. 2006 Jul 1; 31(3):1116-28.

If you use the 3D editing tool, please also cite: *(citation to be added)*

## Licence and attribution

ITK-SNAP is licensed under the **GNU General Public License v3.0** — see [COPYING](COPYING). This
fork is distributed under the same licence.

All credit for ITK-SNAP itself belongs to Paul A. Yushkevich, Guido Gerig and the many contributors
listed on the [ITK-SNAP credits page](http://itksnap.org/credits.php). The upstream project's README
is preserved here as [README.upstream.md](README.upstream.md).

This fork is not affiliated with or endorsed by the ITK-SNAP project, and the changes here have not
been submitted upstream.
