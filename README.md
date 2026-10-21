# ITK-SNAP 3D Editing Tool

[![Build and package](https://github.com/HUmutI/itksnap-3d-edit/actions/workflows/build.yml/badge.svg)](https://github.com/HUmutI/itksnap-3d-edit/actions/workflows/build.yml)

A version of [ITK-SNAP](https://github.com/pyushkevich/itksnap) in which you can **correct a
segmentation directly on its 3D surface**, instead of searching for the right voxels slice by slice.

![3D editing tool demo](docs/3d-editing-demo.gif)

*The demo shows an airway segmentation from a pediatric chest CT, the use case the tool was built
for. The tool works on any label image.*

## Contents

- [Why this tool exists](#why-this-tool-exists)
- [Quick start](#quick-start)
- [The four sub-tools](#the-four-sub-tools)
- [A typical correction session](#a-typical-correction-session)
- [Controls](#controls)
- [Settings](#settings)
- [Good to know](#good-to-know)
- [Troubleshooting](#troubleshooting)
- [Installation](#installation)
- [Building from source](#building-from-source)
- [How the brush works](#how-the-brush-works)
- [Limitations](#limitations)
- [Citing, licence and attribution](#citing-licence-and-attribution)

## Why this tool exists

Automatic segmentations of tree-like or thin structures (airways, vessels, nerves) usually go wrong
in ways that are easy to see in 3D and hard to find in 2D:

- a small fragment floating next to the structure,
- a branch that is broken into two pieces,
- a hole or tunnel through the wall,
- a missing piece of a thin branch.

In standard ITK-SNAP you see these defects in the 3D view, but you have to fix them in the 2D slice
views, which means finding the responsible voxels one slice at a time. This version adds a
**3D Editing Tool** to the 3D view, so you click on the defect where you see it and the voxels
underneath are changed for you.

Everything else in ITK-SNAP works as before. Files are standard ITK-SNAP files, so a segmentation
corrected here opens in any ITK-SNAP or other NIfTI viewer.

## Quick start

1. **Download** the package for your computer from the
   [Releases page](https://github.com/HUmutI/itksnap-3d-edit/releases) and install it
   (details in [Installation](#installation)).
2. **Open the image**: *File > Open Main Image*.
3. **Open the segmentation**: *Segmentation > Open Segmentation*.
4. **Choose the label to edit**: *Segmentation Labels > Active label* in the left panel.
5. **Build the 3D surface**: press **update** under the 3D view.
6. **Switch to the 3D tool**: *Tools > Active 3D Tool > 3D Editing Tool*, or click its button in
   the 3D toolbox of the left panel.
7. **Pick a sub-tool** (Brush, Delete island, Bridge or Fill hole) in the
   *3D Editing Tool Inspector* and click on the surface.
8. **Save**: *Segmentation > Save Segmentation*.

## The four sub-tools

| Sub-tool | Use it when | What you do | What happens |
|---|---|---|---|
| **Brush** | a part is too thin, missing, or too thick | left-drag on the surface to paint, right-drag to erase | voxels inside the brush ball are added to (or removed from) the active label |
| **Delete island** | a fragment is floating next to the structure | click the fragment | the whole connected piece you clicked is removed |
| **Bridge** | a branch is broken into two pieces | click one end, then the other end | a tube as thick as the brush is painted between the two points |
| **Fill hole** | there is a hole or tunnel in the surface | click next to the hole | the hole is closed with a local morphological closing |

## A typical correction session

Using an airway segmentation as an example:

1. Rotate the 3D view and look for defects.
2. **Fragments first.** Select *Delete island* and click each floating piece. Large pieces (over
   20 000 voxels) ask for confirmation, so the main tree cannot be deleted by a misclick.
3. **Broken branches.** Select *Bridge*, set the brush size to about the branch diameter, click the
   end of the parent branch and then the start of the separated piece.
4. **Holes.** Select *Fill hole* and click next to each hole. If a hole stays open, increase the
   *Closing radius* and click again.
5. **Shape.** Select *Brush* and paint along branches that are too thin or missing, or right-drag to
   erase leaks.
6. Use **Ctrl+Z** (Cmd+Z on macOS) to undo any step, then save.

## Controls

| Input | Action |
|---|---|
| Left-drag | Brush: paint. Other sub-tools: click to apply |
| Right-drag | Brush only: erase (only voxels of the active label) |
| Middle-drag | Rotate the camera |
| Shift + left-drag | Pan the camera |
| Ctrl + left-drag | Spin the camera |
| Mouse wheel | Zoom |
| `[` and `]` | Smaller / larger brush |
| `,` and `.` | Move the brush towards / away from the camera |
| `Esc` | Cancel the first point of a bridge |
| `Ctrl+Z` (`Cmd+Z`) | Undo, one step per stroke or click |

The keys work when the mouse pointer is over the 3D view. A wireframe sphere follows the cursor and
shows exactly what the brush will paint.

## Settings

All settings are in the *3D Editing Tool Inspector* panel, so every action is also possible
without the keyboard or the mouse wheel.

| Setting | Default | Meaning |
|---|---|---|
| Brush size | 6 voxels | diameter of the brush in voxels; 1 paints a single voxel. Also the thickness of a bridge |
| Brush shape | round | round (ball) or square (cube) |
| Depth | 0 | moves the brush centre along the viewing direction, from -20 to 20 voxels. Positive values go below the visible surface |
| Isotropic | off | when on, the brush has equal physical size in all directions on images with unequal voxel sizes |
| Closing radius | 2 voxels | size of the closing used by Fill hole (1 to 8). Larger values close larger holes |
| Match any label | off | Delete island: when on, the piece is traced through all labels, not only the label you clicked |

Settings return to their defaults each time ITK-SNAP starts, like the 2D paintbrush.

## Good to know

- **An active label is needed.** The tools paint, erase and add the active label. If *Clear Label*
  is selected there is nothing to paint, so the tool shows a message instead.
- **What you draw stays connected.** Brush strokes and bridges are always connected voxel to voxel,
  even at size 1, so a thin connection you draw really connects the two parts.
- **Delete island only removes the piece you clicked**, of the label under the cursor. Touching
  structures with a different label are not affected unless *Match any label* is on.
- **Fill hole is local and safe.** It only fills unlabelled voxels close to where you clicked (within
  twice the closing radius). It never changes other labels.
- **Undo is per stroke or click**, and the 3D surface updates after every change, including undo
  and redo.
- **Isotropic mode is off by default.** On a typical chest CT with voxels of about 0.9 x 0.9 x 0.5 mm,
  isotropic mode measures the brush in units of the smallest spacing, which makes a small brush
  very thin along one axis.

## Troubleshooting

**The tool does nothing when I click.**
Check that an active label is selected (not *Clear Label*) and that the 3D surface exists (press
**update**).

**A part looks disconnected, but Delete island removes the whole structure.**
ITK-SNAP smooths the surface before drawing it (Gaussian smoothing, sigma 0.8 by default).
Connections only one voxel thick can disappear from the drawing although the voxels are still
connected, and Delete island works on the voxels. To see the true connections, turn smoothing off
under *Preferences > 3D Rendering* and press **update**. From the command line, the `c3d` program
built with this repository lists the connected pieces and their sizes:

```sh
c3d your_mask.nii.gz -comp -dup -lstat
```

**The brush paints on the wrong side of a thin wall.**
Change the *Depth* (`,` and `.`) so the brush centre moves towards or away from the camera.

**macOS says the app is damaged or cannot be opened.**
The app is not notarized. See the [macOS installation](#macos) step below.

## Installation

Prebuilt packages are on the [Releases page](https://github.com/HUmutI/itksnap-3d-edit/releases).
Every build on the [Actions tab](https://github.com/HUmutI/itksnap-3d-edit/actions) also uploads the
same packages, so the latest version is always available there.

| Computer | File |
|---|---|
| Mac with Apple Silicon (M1 or newer) | `itksnap-*-Darwin-arm64.dmg` |
| Mac with Intel processor | `itksnap-*-Darwin-x86_64.dmg` |
| Ubuntu or other Linux, x86_64 | `itksnap-*-Linux-x86_64.tar.gz` |
| Windows, x64 (not tested by us) | `itksnap-*-win64-AMD64.exe` |

### macOS

Open the `.dmg` and drag `ITK-SNAP.app` into *Applications*. The app is not notarized by Apple, so
the first time, right-click the app and choose **Open**, or run:

```sh
xattr -dr com.apple.quarantine /Applications/ITK-SNAP.app
```

### Ubuntu / Linux

The package contains everything it needs (Qt included) and runs from any folder:

```sh
tar xzf itksnap-*-Linux-x86_64.tar.gz
cd itksnap-*-Linux-x86_64
bin/itksnap
```

It was built on Ubuntu 22.04 and runs on 22.04 or newer. On a minimal system, install the libraries
Qt needs first:

```sh
sudo apt install libgl1 libglu1-mesa libxkbcommon-x11-0 libxcb-cursor0 libxcb-icccm4 \
  libxcb-keysyms1 libxcb-shape0 libxcb-xinerama0 libfontconfig1
```

### Windows

The installer is built automatically in the same way as official ITK-SNAP. We use macOS and Ubuntu
ourselves and do not test Windows.

## Building from source

Only needed if you want to change the code. The build is the standard ITK-SNAP build; the 3D tool
adds no new dependencies. All commands run from one workspace folder, and nothing is built inside
the source tree.

### Requirements

| | Version | Notes |
|---|---|---|
| CMake | 3.16 or newer | |
| C++ compiler | C++17 | |
| ITK | 5.4.0 (5.4 or newer) | built with `Module_MorphologicalContourInterpolation=ON` |
| VTK | 9.5.2 (9.3.1 or newer) | built with Qt support and `RenderingExternal` |
| Qt | 6.7 or newer | 6.8.1 in CI; needs `qtbase`, `qtdeclarative` and `qttools` |
| libssh, libcurl | any recent version | |

Do not use the ITK or VTK from Homebrew or apt. Their ITK lacks the
`MorphologicalContourInterpolation` module and their VTK often lacks Qt support
(`VTK_GROUP_ENABLE_Qt=YES`).

### 1. Workspace and clone

```sh
mkdir ~/itksnap-ws && cd ~/itksnap-ws
git clone https://github.com/HUmutI/itksnap-3d-edit.git
git -C itksnap-3d-edit submodule update --init
```

Stay in `~/itksnap-ws` for all following commands. The submodules (`c3d`, `greedy`, `digestible`)
are required.

### 2. System packages

<details open>
<summary><b>macOS</b> (tested on Apple Silicon, macOS 26)</summary>

```sh
brew install cmake ninja qtbase qtdeclarative qttools libssh
```
</details>

<details>
<summary><b>Ubuntu / Linux</b></summary>

```sh
sudo apt install build-essential cmake ninja-build git python3-pip \
  libssh-dev libcurl4-openssl-dev libgl1-mesa-dev libglu1-mesa-dev \
  libxkbcommon-x11-0 libxcb-cursor0 libxcb-icccm4 libxcb-keysyms1 \
  libxcb-shape0 libxcb-xinerama0 libfontconfig1-dev
```

The Qt from apt is too old on Ubuntu 22.04 (6.2) and 24.04 (6.4). Install Qt 6.8.1 with
[aqtinstall](https://github.com/miurahr/aqtinstall), as the CI does:

```sh
python3 -m pip install aqtinstall
python3 -m aqt install-qt linux desktop 6.8.1 linux_gcc_64 -O ~/Qt
```

Then add `-DCMAKE_PREFIX_PATH=$HOME/Qt/6.8.1/gcc_64` to both the VTK configure (step 3) and the
ITK-SNAP configure (step 4). On Ubuntu 25.04 or newer, `apt install qt6-base-dev
qt6-declarative-dev qt6-tools-dev qt6-tools-dev-tools` is recent enough instead.
</details>

### 3. Build VTK and ITK

The same on both platforms. This takes much longer than ITK-SNAP itself, but only has to be done once.

```sh
# VTK 9.5.2 (Ubuntu: add -DCMAKE_PREFIX_PATH=$HOME/Qt/6.8.1/gcc_64)
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

`ITK_DIR` below points to the ITK **build** folder, not an install folder: ITK's install step misses
one header (`vnl_vector_ref.hxx`), and ITK-SNAP's own CI does the same.

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

Run `itksnap-build/ITK-SNAP`. To make the `.app` and `.dmg`:

```sh
cmake --build itksnap-build --target package
```

The package step signs the app ad hoc, so no developer certificate is needed. The finished app is
also placed at `itksnap-build/_CPack_Packages/Darwin-arm64/Bundle/itksnap-*/ITK-SNAP.app`; the
`.dmg` step uses Finder and can fail without a desktop session, but the app is complete either way.

If you copy an unpackaged build elsewhere, sign it again:

```sh
codesign --force --deep --sign - /path/to/ITK-SNAP.app
```

Do not build in an iCloud-synced folder such as `~/Desktop` or `~/Documents`: iCloud adds file
attributes that make `codesign` fail with *"resource fork, Finder information, or similar detritus
not allowed"*.
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

Run `itksnap-build/ITK-SNAP`. `cmake --build itksnap-build --target package` makes the same
portable tarball that CI publishes.
</details>

### 5. Test

```sh
cd itksnap-build
ctest -LE Remote                 # on Linux without a display: xvfb-run -a ctest -LE Remote
```

Tests labelled `Remote` need network access and are skipped. `ctest -R SegmentationEdit3DTest` runs
only the tests of the 3D editing operations (brush stamp, stroke connectivity, island deletion and
hole filling), without a GUI.

## How the brush works

The original ITK-SNAP 3D tools find the clicked point by stepping along the viewing ray through the
label image. That can only return a voxel that is already labelled, which is why the existing spray
tool can thicken a structure by at most one voxel and can never close a gap.

This tool instead finds the clicked point on the **drawn surface** (with a `vtkCellPicker`). That
point lies on the boundary between labelled and unlabelled voxels, so a ball centred there also
covers unlabelled voxels, and painting moves the surface outwards. While you drag, the space between
successive mouse positions is filled as a capsule, which is what lets a stroke cross a gap.

If the surface is missing or out of date (for example right after painting in 2D, before pressing
**update**), the tool falls back to the original method.

## Limitations

- After each edit the whole 3D surface is rebuilt, which takes a few seconds on a large structure.
  Rebuilding only the edited region is planned but not implemented yet.
- Settings are not saved between sessions.
- Windows builds are not tested.

## Citing, licence and attribution

If you use ITK-SNAP in published work, please cite:

> Paul A. Yushkevich, Joseph Piven, Heather Cody Hazlett, Rachel Gimpel Smith, Sean Ho, James C. Gee,
> and Guido Gerig. *User-guided 3D active contour segmentation of anatomical structures:
> Significantly improved efficiency and reliability.* Neuroimage. 2006 Jul 1; 31(3):1116-28.

If you use the 3D editing tool, please also cite: *(citation to be added)*

ITK-SNAP is licensed under the **GNU General Public License v3.0** (see [COPYING](COPYING)), and
this version is distributed under the same licence. All credit for ITK-SNAP itself belongs to
Paul A. Yushkevich, Guido Gerig and the contributors listed on the
[ITK-SNAP credits page](http://itksnap.org/credits.php). The original README is kept as
[README.upstream.md](README.upstream.md).

This version is not affiliated with or endorsed by the ITK-SNAP project, and its changes have not
been submitted upstream.
