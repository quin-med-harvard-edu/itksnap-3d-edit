#ifndef VIEWPANEL3D_H
#define VIEWPANEL3D_H

#include <SNAPComponent.h>
#include "Generic3DModel.h"
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QDebug>
#include <QTimer>
#include <QFutureWatcher>
#include <itkCommand.h>

namespace Ui {
  class ViewPanel3D;
}

namespace itk {
  template <class T> class MemberCommand;
}

class Generic3DModel;
class GenericView3D;
class QMenu;
class QToolButton;


class ViewPanel3D : public SNAPComponent
{
  Q_OBJECT

public:
  explicit ViewPanel3D(QWidget *parent = 0);
  ~ViewPanel3D();

  // Register with the global model
  void Initialize(GlobalUIModel *model);

  GenericView3D *Get3DView();

signals:

  void renderProgress(int progress);
  void updateContextMenu();

private slots:
  virtual void onModelUpdate(const EventBucket &bucket);

  void onView3dResize();

  void onContextMenuUpdateRequested();

  // Ask the user before deleting a large connected region in the 3D view
  void onConfirmIslandDelete();

  // Explain why a 3D edit was refused with the clear label active
  void onBrush3DInvalidLabel();

  void on_btnUpdateMesh_clicked();

  void on_btnScreenshot_clicked();

  void on_btnAccept_clicked();

  void on_btnCancel_clicked();

  void on_btnExpand_clicked();

  void onTimer();


  void on_actionReset_Viewpoint_triggered();

  void on_actionSave_Viewpoint_triggered();

  void on_actionRestore_Viewpoint_triggered();

  void on_actionImport_Viewpoint_triggered();

  void on_actionExport_Viewpoint_triggered();

  void on_actionContinuous_Update_triggered();

  void on_btnFlip_clicked();

  void on_actionClear_Rendering_triggered();

private:
  Ui::ViewPanel3D *ui;

  GlobalUIModel *m_GlobalUI;

  Generic3DModel *m_Model;

  QMenu *m_DropMenu;
  QMenu *m_MeshLayerMenu;
  QMenu *m_FocalPointTargetMenu;
  QMenu *m_ContextButtonMenu;

  QTimer *m_RenderTimer;

  // Whether the "pick a drawing label" explanation has already been shown
  bool m_Brush3DLabelWarningShown = false;

  // A future used to track background rendering
  QFuture<void> m_RenderFuture;

  // A mutex on the progress value, which is accesed by multiple threads
  mutable QMutex m_RenderProgressMutex;

  // Progress of the rendering operation
  double m_RenderProgressValue;

  // Elapsed time since begin of render operation
  int m_RenderElapsedTicks;

  // Context menu tool button
  QToolButton *m_ContextToolButton;

  typedef itk::MemberCommand<ViewPanel3D> CommandType;
  SmartPtr<CommandType> m_RenderProgressCommand;

  // True while RefreshMeshForEditing() is rebuilding the mesh on the GUI
  // thread; guards against re-entry from events pumped during the rebuild
  bool m_EditingMeshRefreshInProgress = false;

  // Progress command for the synchronous rebuild: drives the inline progress
  // bar and pumps paint events, without the modal progress dialog
  SmartPtr<CommandType> m_EditingProgressCommand;

  // Rebuild the mesh after a segmentation change while the 3D editing tool is
  // active. Synchronous, so the picking surface is current before the next
  // mouse event; non-modal, so nothing pops up on every stroke.
  void RefreshMeshForEditing();

  void EditingProgressCallback(itk::Object *source, const itk::EventObject &event);

  void UpdateExpandViewButton();

  void UpdateMeshesInBackground();

  void UpdateActionButtons();

  void UpdateMeshLayerMenu();

  // Load camera viewpoint
  void LoadCameraViewpoint(QString file);

  // Save camera viewpoint
  void SaveCameraViewpoint(QString file);

  // Apply color bar visibility based on the active mesh layer type
  void ApplyDefaultColorBarVisibility();

  void ProgressCallback(itk::Object *source, const itk::EventObject &event);

  void UpdateContextButtonLocation();
};

#endif // VIEWPANEL3D_H
