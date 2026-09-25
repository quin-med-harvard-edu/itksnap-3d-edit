/*=========================================================================

  Program:   ITK-SNAP
  Module:    SegmentationEdit3DTest.cxx
  Copyright (c) 2026 Paul A. Yushkevich

  This file is part of ITK-SNAP

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

=========================================================================*/

// Exercises the voxel-level operations behind the 3D editing tool without any
// GUI: brush stamps and sweeps (BrushStamp3D / BrushCapsule3D via
// SegmentationEdit3D::ApplyStamp), island deletion, hole filling and the undo
// bookkeeping. Usage: SegmentationEdit3DTest <main-image>

#include "IRISApplication.h"
#include "ImageIODelegates.h"
#include "IRISException.h"
#include "GlobalState.h"
#include "LabelImageWrapper.h"
#include "SegmentationEdit3D.h"
#include "SegmentationUpdateIterator.h"
#include "BrushStamp3D.h"
#include "UIReporterDelegates.h"
#include "RLEImageRegionConstIterator.h"
#include "itksys/SystemTools.hxx"

#include <itkImageRegionConstIterator.h>

#include <cmath>
#include <iostream>
#include <string>

namespace
{

class TestSystemInfoDelegate : public SystemInfoDelegate
{
public:
  TestSystemInfoDelegate(const char *argv0) : m_Exe(argv0) {}

  std::string GetApplicationDirectory() override
    { return itksys::SystemTools::GetFilenamePath(m_Exe); }
  std::string GetApplicationFile() override { return m_Exe; }
  std::string GetApplicationPermanentDataLocation() override { return ".itksnap.test"; }
  std::string GetUserDocumentsLocation() override { return ".itksnap.test"; }
  std::string GetTempDirectory() override
    {
    const char *t = getenv("TMPDIR");
    return t ? std::string(t) : std::string("/tmp");
    }
  std::string EncodeServerURL(const std::string &url) override { return url; }

  typedef SystemInfoDelegate::GrayscaleImage GrayscaleImage;
  typedef SystemInfoDelegate::RGBAImageType  RGBAImageType;
  void LoadResourceAsImage2D(std::string, GrayscaleImage *) override {}
  void LoadResourceAsRegistry(std::string, Registry &) override {}
  void WriteRGBAImage2D(std::string, RGBAImageType *) override {}

private:
  std::string m_Exe;
};

class TestColorMapSource : public AbstractColorMapPresetNameSource
{
public:
  std::string GetPresetName(ColorMap::SystemPreset p, bool) override
    { return "Preset " + std::to_string(static_cast<int>(p)); }
};

typedef LabelImageWrapper::ImageType   LabelImageType;
typedef itk::ImageRegion<3>            RegionType;
typedef itk::Index<3>                  IndexType;

int g_failures = 0;

#define CHECK(cond, msg)                                                      \
  do {                                                                        \
    if(!(cond))                                                               \
      {                                                                       \
      std::cerr << "FAIL [" << __LINE__ << "] " << msg << std::endl;          \
      g_failures++;                                                           \
      }                                                                       \
    else                                                                      \
      std::cout << "  ok   " << msg << std::endl;                             \
  } while(0)

unsigned long CountLabel(LabelImageWrapper *seg, LabelType label)
{
  const LabelImageType *img = seg->GetImage();
  itk::ImageRegionConstIterator<LabelImageType> it(img, img->GetBufferedRegion());
  unsigned long n = 0;
  for(; !it.IsAtEnd(); ++it)
    if(it.Get() == label)
      n++;
  return n;
}

LabelType PixelAt(LabelImageWrapper *seg, const IndexType &idx)
{
  return seg->GetImage()->GetPixel(idx);
}

// Overwrite a region with a label, ignoring draw-over rules
void FillRegion(LabelImageWrapper *seg, const RegionType &region, LabelType label)
{
  SegmentationUpdateIterator it(seg, region, label, DrawOverFilter(PAINT_OVER_ALL, 0));
  for(; !it.IsAtEnd(); ++it)
    it.PaintLabel(label);
  it.Finalize("test fill");
}

void ClearAll(LabelImageWrapper *seg)
{
  FillRegion(seg, seg->GetImage()->GetBufferedRegion(), 0);
}

RegionType Box(const IndexType &lo, const IndexType &hi)
{
  RegionType r;
  for(unsigned int i = 0; i < 3; i++)
    {
    r.SetIndex(i, lo[i]);
    r.SetSize(i, (unsigned long)(hi[i] - lo[i] + 1));
    }
  return r;
}

IndexType Off(const IndexType &c, long dx, long dy, long dz)
{
  IndexType r = c;
  r[0] += dx; r[1] += dy; r[2] += dz;
  return r;
}

Vector3d OffD(const IndexType &c, double dx, double dy, double dz)
{
  return Vector3d(c[0] + dx, c[1] + dy, c[2] + dz);
}

const Vector3d kUnitSpacing(1.0, 1.0, 1.0);
const DrawOverFilter kOverAll(PAINT_OVER_ALL, 0);

unsigned long Ball(LabelImageWrapper *seg, const Vector3d &center, double radius,
                   LabelType label, bool erase = false, const char *undo = "test ball")
{
  BrushStamp3D stamp(center, radius, PAINTBRUSH_ROUND, false, kUnitSpacing);
  return SegmentationEdit3D::ApplyStamp(seg, stamp, label, kOverAll, erase, undo, nullptr);
}

// ---------------------------------------------------------------------------

void TestStamp(LabelImageWrapper *seg, const IndexType &c)
{
  std::cout << "Brush stamp" << std::endl;

  // The voxel under the cursor is always painted, even at the smallest size
  // and with a centre far from any voxel centre
  ClearAll(seg);
  unsigned long n = Ball(seg, OffD(c, 0.3, 0.4, 0.45), 0.5, 1);
  CHECK(n == 1, "size-1 brush off-centre paints exactly one voxel (painted " << n << ")");
  CHECK(PixelAt(seg, c) == 1, "size-1 brush paints the rounded cursor voxel");

  // Radius 1 (size 2) centred on a voxel is just that voxel; centred on the
  // midpoint of four voxels it is the 2x2 block, as in the 2D paintbrush. A
  // 3D corner is sqrt(3)/2 from the voxel centres, beyond the 0.75 effective
  // radius, so only the cursor voxel is painted there.
  ClearAll(seg);
  n = Ball(seg, OffD(c, 0, 0, 0), 1.0, 1);
  CHECK(n == 1, "radius-1 ball on a voxel centre paints one voxel (painted " << n << ")");
  ClearAll(seg);
  n = Ball(seg, OffD(c, 0.5, 0.5, 0.0), 1.0, 1);
  CHECK(n == 4, "radius-1 ball between four voxels paints the 2x2 block (painted " << n << ")");
  ClearAll(seg);
  n = Ball(seg, OffD(c, 0.5, 0.5, 0.5), 1.0, 1);
  CHECK(n == 1, "radius-1 ball on a voxel corner still paints the cursor voxel (painted " << n << ")");

  // Radius 1.5 (size 3) mirrors the 2D paintbrush: the 7-voxel cross
  ClearAll(seg);
  n = Ball(seg, OffD(c, 0, 0, 0), 1.5, 1);
  CHECK(n == 7, "radius-1.5 ball paints the 7-voxel cross (painted " << n << ")");
  CHECK(PixelAt(seg, Off(c, 1, 1, 0)) == 0, "radius-1.5 ball excludes edge neighbours");

  // Off-image stamps are a no-op with an empty dirty region
  RegionType dirty;
  BrushStamp3D far_away(Vector3d(-100.0, -100.0, -100.0), 3.0, PAINTBRUSH_ROUND, false, kUnitSpacing);
  n = SegmentationEdit3D::ApplyStamp(seg, far_away, 1, kOverAll, false, "off", &dirty);
  CHECK(n == 0 && dirty.GetNumberOfPixels() == 0, "stamp outside the image changes nothing");
}

void TestEraseAndDrawOver(LabelImageWrapper *seg, const IndexType &c)
{
  std::cout << "Erase and draw-over" << std::endl;

  ClearAll(seg);
  unsigned long n_ball = Ball(seg, OffD(c, 0, 0, 0), 3.0, 1);
  CHECK(n_ball > 7, "radius-3 ball paints a volume (" << n_ball << " voxels)");

  // Erasing removes only the active label
  unsigned long n = Ball(seg, OffD(c, 0, 0, 0), 3.0, 2, true);
  CHECK(n == 0 && CountLabel(seg, 1) == n_ball, "erase with another label active changes nothing");
  n = Ball(seg, OffD(c, 0, 0, 0), 3.0, 1, true);
  CHECK(n == n_ball && CountLabel(seg, 1) == 0, "erase with the ball's label removes it entirely");

  // Draw-over: paint over one label only
  Ball(seg, OffD(c, 0, 0, 0), 3.0, 1);
  BrushStamp3D stamp(OffD(c, 0, 0, 0), 3.0, PAINTBRUSH_ROUND, false, kUnitSpacing);
  n = SegmentationEdit3D::ApplyStamp(seg, stamp, 2, DrawOverFilter(PAINT_OVER_ONE, 3), false, "do", nullptr);
  CHECK(n == 0, "paint over label 3 leaves a label-1 ball alone");
  n = SegmentationEdit3D::ApplyStamp(seg, stamp, 2, DrawOverFilter(PAINT_OVER_ONE, 1), false, "do", nullptr);
  CHECK(n == n_ball && CountLabel(seg, 2) == n_ball, "paint over label 1 relabels the whole ball");
}

void TestUndo(LabelImageWrapper *seg, const IndexType &c)
{
  std::cout << "Undo" << std::endl;

  ClearAll(seg);
  seg->ClearUndoPoints();
  CHECK(!seg->IsUndoPossible(), "no undo after clearing the history");

  // A one-shot stamp stores its own undo point
  unsigned long n_ball = Ball(seg, OffD(c, 0, 0, 0), 2.0, 1);
  CHECK(seg->IsUndoPossible(), "one-shot stamp stores an undo point");
  seg->Undo();
  CHECK(CountLabel(seg, 1) == 0, "undo removes the stamp");
  seg->Redo();
  CHECK(CountLabel(seg, 1) == n_ball, "redo restores the stamp");

  // A stroke: several staged stamps merged into a single undo point
  ClearAll(seg);
  seg->ClearUndoPoints();
  for(int i = 0; i < 4; i++)
    Ball(seg, OffD(c, 2.0 * i, 0, 0), 1.5, 1, false, nullptr);
  CHECK(!seg->IsUndoPossible(), "staged stamps do not create undo points by themselves");
  unsigned long n_stroke = CountLabel(seg, 1);
  seg->StoreUndoPoint("stroke");
  CHECK(seg->IsUndoPossible(), "committing the stroke creates one undo point");
  seg->Undo();
  CHECK(CountLabel(seg, 1) == 0, "one undo reverts the whole stroke (" << n_stroke << " voxels)");
  CHECK(!seg->IsUndoPossible(), "the stroke was a single undo point");
}

// Deletes the 6-connected component at `seed` and reports how many voxels
// of `label` survive elsewhere
unsigned long DeleteAndCount(LabelImageWrapper *seg, const IndexType &seed, LabelType label,
                             bool any_label, bool *deleted_out, unsigned long *n_out)
{
  RegionType dirty;
  unsigned long n = 0;
  bool ok = SegmentationEdit3D::DeleteConnectedComponent(
        seg, seed, any_label, 1ul << 30, &dirty, &n);
  if(deleted_out) *deleted_out = ok;
  if(n_out) *n_out = n;
  return CountLabel(seg, label);
}

void TestSweepConnectivity(LabelImageWrapper *seg, const IndexType &c)
{
  std::cout << "Sweep / bridge connectivity" << std::endl;

  const double radii[] = { 0.5, 1.0, 1.5 };
  for(double r : radii)
    {
    ClearAll(seg);
    Vector3d a = OffD(c, -8.2, -6.7, -5.1);
    Vector3d b = OffD(c,  8.4,  7.3,  6.6);
    BrushCapsule3D capsule(a, b, r, PAINTBRUSH_ROUND, false, kUnitSpacing);

    unsigned long n = SegmentationEdit3D::ApplyStamp(seg, capsule, 1, kOverAll, false, "bridge", nullptr);
    CHECK(n > 0, "radius " << r << " capsule paints " << n << " voxels");

    // Every voxel of the face-crossing path must be painted ...
    bool path_ok = true;
    for(const IndexType &idx : capsule.GetVoxelPath())
      if(PixelAt(seg, idx) != 1)
        path_ok = false;
    CHECK(path_ok, "radius " << r << " capsule paints its " << capsule.GetVoxelPath().size() << "-voxel path");

    // ... and the endpoints must be one 6-connected component: deleting the
    // component at A must leave nothing behind
    IndexType seed_a;
    for(unsigned int i = 0; i < 3; i++)
      seed_a[i] = (long) std::floor(a[i] + 0.5);
    bool deleted = false;
    unsigned long left = DeleteAndCount(seg, seed_a, 1, false, &deleted, nullptr);
    CHECK(deleted && left == 0, "radius " << r << " capsule is a single 6-connected component (" << left << " left)");
    }
}

void TestDeleteIsland(LabelImageWrapper *seg, const IndexType &c)
{
  std::cout << "Delete island" << std::endl;

  ClearAll(seg);
  unsigned long n_left  = Ball(seg, OffD(c, -10, 0, 0), 3.0, 1);
  unsigned long n_right = Ball(seg, OffD(c,  10, 0, 0), 3.0, 1);
  CHECK(CountLabel(seg, 1) == n_left + n_right, "two separate balls painted");

  bool deleted = false;
  unsigned long n = 0;
  unsigned long left = DeleteAndCount(seg, Off(c, -10, 0, 0), 1, false, &deleted, &n);
  CHECK(deleted && n == n_left, "left ball reported as " << n << " voxels");
  CHECK(left == n_right, "deleting the left ball leaves the right one intact");

  // Clicking background is a no-op
  RegionType dirty;
  bool ok = SegmentationEdit3D::DeleteConnectedComponent(seg, Off(c, -10, 0, 0), false, 1ul << 30, &dirty, &n);
  CHECK(!ok && CountLabel(seg, 1) == n_right, "deleting at a background voxel does nothing");

  // Over budget: nothing deleted, size reported
  n = 0;
  ok = SegmentationEdit3D::DeleteConnectedComponent(seg, Off(c, 10, 0, 0), false, 5, &dirty, &n);
  CHECK(!ok && n >= 6 && CountLabel(seg, 1) == n_right,
        "over-budget component is left alone and its size (" << n << ") reported");

  // Touching balls of different labels: delete one label only, or any label
  ClearAll(seg);
  Ball(seg, OffD(c, 0, 0, 0), 3.0, 1);
  Ball(seg, OffD(c, 5, 0, 0), 3.0, 2);
  unsigned long n1 = CountLabel(seg, 1), n2 = CountLabel(seg, 2);
  CHECK(n1 > 0 && n2 > 0, "two touching balls with labels 1 and 2");
  left = DeleteAndCount(seg, Off(c, -2, 0, 0), 1, false, &deleted, nullptr);
  CHECK(deleted && left == 0 && CountLabel(seg, 2) == n2, "single-label delete spares the neighbour");

  Ball(seg, OffD(c, 0, 0, 0), 3.0, 1);
  Ball(seg, OffD(c, 5, 0, 0), 3.0, 2);
  left = DeleteAndCount(seg, Off(c, -2, 0, 0), 1, true, &deleted, nullptr);
  CHECK(deleted && left == 0 && CountLabel(seg, 2) == 0, "any-label delete removes both");

  // The ROI grows until the whole component is found (a component larger
  // than the 32-voxel initial cube)
  ClearAll(seg);
  FillRegion(seg, Box(Off(c, -25, -2, -2), Off(c, 25, 2, 2)), 1);
  unsigned long n_bar = CountLabel(seg, 1);
  left = DeleteAndCount(seg, c, 1, false, &deleted, &n);
  CHECK(deleted && n == n_bar && left == 0, "component spanning the initial ROI is fully found and deleted");
}

void TestCloseHole(LabelImageWrapper *seg, const IndexType &c)
{
  std::cout << "Fill hole" << std::endl;

  // A 3-voxel thick plate with a 2x2 through-hole at the centre, another
  // 2x2 hole further away, and one foreign-label voxel inside the near hole
  ClearAll(seg);
  FillRegion(seg, Box(Off(c, -8, -8, -1), Off(c, 8, 8, 1)), 1);
  unsigned long n_plate = CountLabel(seg, 1);

  FillRegion(seg, Box(Off(c, -1, -1, -1), Off(c, 0, 0, 1)), 0);   // near hole, 12 voxels
  FillRegion(seg, Box(Off(c,  6,  6, -1), Off(c, 7, 7, 1)), 0);   // far hole, 12 voxels
  FillRegion(seg, Box(Off(c, -1, -1, 0), Off(c, -1, -1, 0)), 2);  // foreign voxel in near hole
  CHECK(CountLabel(seg, 1) == n_plate - 24, "plate with two holes");

  RegionType dirty;
  bool changed = SegmentationEdit3D::CloseHole(seg, Off(c, 0, 0, 0), 1, 2.0, false, &dirty);
  CHECK(changed, "closing at the near hole changes the image");
  CHECK(PixelAt(seg, Off(c, 0, 0, 0)) == 1 && PixelAt(seg, Off(c, -1, 0, 1)) == 1,
        "near hole is filled");
  CHECK(PixelAt(seg, Off(c, -1, -1, 0)) == 2, "foreign-label voxel inside the hole is preserved");
  CHECK(PixelAt(seg, Off(c, 6, 6, 0)) == 0 && PixelAt(seg, Off(c, 7, 7, -1)) == 0,
        "far hole is not touched");
  CHECK(CountLabel(seg, 1) == n_plate - 12 - 1, "exactly the near hole (minus the foreign voxel) was filled");

  // Nothing to close: no change
  changed = SegmentationEdit3D::CloseHole(seg, Off(c, -6, -6, 0), 1, 2.0, false, &dirty);
  CHECK(!changed, "closing on solid material changes nothing");

  // Clear label is refused
  changed = SegmentationEdit3D::CloseHole(seg, Off(c, 6, 6, 0), 0, 2.0, false, &dirty);
  CHECK(!changed && PixelAt(seg, Off(c, 6, 6, 0)) == 0, "closing with the clear label is refused");
}

} // namespace


int main(int argc, char *argv[])
{
  if(argc < 2)
    {
    std::cerr << "Usage: " << argv[0] << " <main-image>" << std::endl;
    return 1;
    }

  TestSystemInfoDelegate sidel(argv[0]);
  SystemInterface::SetSystemInfoDelegate(&sidel);

  TestColorMapSource color_map_source;
  ColorMap::SetColorMapPresetNameSource(&color_map_source);

  try
    {
    IRISApplication::Pointer app = IRISApplication::New();
    IRISWarningList warnings;
    app->OpenImage(argv[1], MAIN_ROLE, warnings);

    LabelImageWrapper *seg = app->GetSelectedSegmentationLayer();
    if(!seg)
      {
      app->AddBlankSegmentation();
      seg = app->GetSelectedSegmentationLayer();
      }
    if(!seg)
      {
      std::cerr << "No segmentation layer after loading " << argv[1] << std::endl;
      return 1;
      }

    RegionType full = seg->GetImage()->GetBufferedRegion();
    IndexType c;
    for(unsigned int i = 0; i < 3; i++)
      {
      if(full.GetSize(i) < 60)
        {
        std::cerr << "Test image must be at least 60 voxels along every axis" << std::endl;
        return 1;
        }
      c[i] = full.GetIndex(i) + (long) full.GetSize(i) / 2;
      }

    TestStamp(seg, c);
    TestEraseAndDrawOver(seg, c);
    TestUndo(seg, c);
    TestSweepConnectivity(seg, c);
    TestDeleteIsland(seg, c);
    TestCloseHole(seg, c);
    }
  catch(IRISException &exc)
    {
    std::cerr << "IRISException: " << exc.what() << std::endl;
    return 1;
    }
  catch(std::exception &exc)
    {
    std::cerr << "Exception: " << exc.what() << std::endl;
    return 1;
    }

  if(g_failures)
    {
    std::cerr << g_failures << " check(s) failed" << std::endl;
    return 1;
    }

  std::cout << "All checks passed" << std::endl;
  return 0;
}
