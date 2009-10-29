/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Language:  C++
Date:      $Date$
Version:   $Revision: 15646 $

Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#ifndef QMITKVOLUMEVISUALIZATIONVIEW_H_
#define QMITKVOLUMEVISUALIZATIONVIEW_H_

#include <QmitkFunctionality.h>

#include <cherryISelectionListener.h>
#include <mitkDataTreeNodeSelection.h>

#include <mitkWeakPointer.h>

#include <mitkImage.h>

#include "mitkDataStorage.h"

#include <QmitkDataStorageListModel.h>
#include <QmitkDataStorageComboBox.h>
#include <QmitkTransferFunctionWidget.h>

#include "ui_QmitkVolumeVisualizationViewControls.h"

/**
 * \ingroup org_mitk_gui_qt_volumevisualization_internal
 */
class QmitkVolumeVisualizationView : public QObject, public QmitkFunctionality
{
  Q_OBJECT

public:

  void SetFocus();

  QmitkVolumeVisualizationView();

  virtual ~QmitkVolumeVisualizationView();

  virtual void CreateQtPartControl(QWidget *parent);
  
  ///
  /// Invoked when the DataManager selection changed
  ///
  virtual void SelectionChanged(cherry::IWorkbenchPart::Pointer part, cherry::ISelection::ConstPointer selection);


protected slots:

  void OnEnableRendering( bool state );
  void OnEnableLOD( bool state );

protected:

  ///
  /// A selection listener for datatreenode events
  ///
  cherry::ISelectionListener::Pointer m_SelectionListener;

  Ui::QmitkVolumeVisualizationViewControls* m_Controls;

private:

  mitk::WeakPointer<mitk::DataTreeNode> m_SelectedNode;

};

#endif /*QMITKVOLUMEVISUALIZATIONVIEW_H_*/
