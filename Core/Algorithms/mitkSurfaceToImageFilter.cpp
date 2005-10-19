/*=========================================================================

  Program:   Medical Imaging & Interaction Toolkit
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/ for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include "mitkSurfaceToImageFilter.h"
#include "mitkTimeHelper.h"

#include <vtkPolyData.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkImageStencil.h>
#include <vtkImageData.h>
#include <vtkPolyData.h>
#include <vtkLinearTransform.h>
#include <vtkTriangleFilter.h>
#include <vtkLinearExtrusionFilter.h>
#include <vtkDataSetTriangleFilter.h>
#include <vtkImageThreshold.h>
#include <vtkImageMathematics.h>
#include <vtkImageCast.h>
#include <vtkImageChangeInformation.h>
#include <vtkPolyDataNormals.h>

#include <vtkTransformPolyDataFilter.h>
#include <vtkTransform.h>
#include <vtkImageChangeInformation.h>

mitk::SurfaceToImageFilter::SurfaceToImageFilter()
{
  m_MakeOutputBinaryOn = false;
  m_BackgroundValue = -10000;
}

mitk::SurfaceToImageFilter::~SurfaceToImageFilter()
{
}

void mitk::SurfaceToImageFilter::GenerateInputRequestedRegion()
{
  mitk::Image* output = this->GetOutput();
  if((output->IsInitialized()==false) )
    return;

  GenerateTimeInInputRegion(output, const_cast< mitk::Image * > ( this->GetImage() ));
}

void mitk::SurfaceToImageFilter::GenerateOutputInformation()
{
  mitk::Image *inputImage = (mitk::Image*)this->GetImage();
  mitk::Image::Pointer output = this->GetOutput();
 
  itkDebugMacro(<<"GenerateOutputInformation()");

  if((inputImage == NULL) || 
     (inputImage->IsInitialized() == false) || 
     (inputImage->GetTimeSlicedGeometry() == NULL)) return;

  output->Initialize(PixelType(typeid(int)), *inputImage->GetTimeSlicedGeometry());

  output->SetPropertyList(inputImage->GetPropertyList()->Clone());    
}

void mitk::SurfaceToImageFilter::GenerateData()
{
  mitk::Image::ConstPointer inputImage = this->GetImage();
  mitk::Image::Pointer output = this->GetOutput();

  if(inputImage.IsNull())
    return;  

  if(output->IsInitialized()==false )
    return;

  mitk::Image::RegionType outputRegion = output->GetRequestedRegion();

  int tstart=outputRegion.GetIndex(3);
  int tmax=tstart+outputRegion.GetSize(3);

  if ( tmax > 0)
  {
    int t;
    for(t=tstart;t<tmax;++t)
    {
      Stencil3DImage( t );
    }
  }
  else 
  {
      Stencil3DImage( 0 );
  }
}

void mitk::SurfaceToImageFilter::Stencil3DImage(int time)
{
  const mitk::TimeSlicedGeometry *surfaceTimeGeometry = GetInput()->GetTimeSlicedGeometry();
  const mitk::TimeSlicedGeometry *imageTimeGeometry = GetImage()->GetTimeSlicedGeometry();
  
  int surfaceTimeStep = (int) surfaceTimeGeometry->TimeStepToMS( (int) (imageTimeGeometry->TimeStepToMS(time) ) );
  
  vtkPolyData * polydata = ( (mitk::Surface*)GetInput() )->GetVtkPolyData( surfaceTimeStep );

  vtkTransformPolyDataFilter * move=vtkTransformPolyDataFilter::New();
  move->SetInput(polydata);
  move->ReleaseDataFlagOn();

  vtkTransform *transform=vtkTransform::New();
  Geometry3D* geometry = surfaceTimeGeometry->GetGeometry3D( surfaceTimeStep );
  geometry->TransferItkToVtkTransform();
  transform->PostMultiply();
  transform->Concatenate(geometry->GetVtkTransform()->GetMatrix());
  // take image geometry into account. vtk-Image information will be changed to unit spacing and zero origin below.
  Geometry3D* imageGeometry = imageTimeGeometry->GetGeometry3D(time);
  imageGeometry->TransferItkToVtkTransform();
  transform->Concatenate(imageGeometry->GetVtkTransform()->GetLinearInverse());
  move->SetTransform(transform);

  vtkPolyDataNormals * normalsFilter = vtkPolyDataNormals::New();
  normalsFilter->SetFeatureAngle(50);
  normalsFilter->SetConsistency(1);
  normalsFilter->SetSplitting(1);
  normalsFilter->SetFlipNormals(0);
  normalsFilter->ReleaseDataFlagOn();

  normalsFilter->SetInput( move->GetOutput() );
  move->Delete();

  vtkPolyDataToImageStencil * surfaceConverter = vtkPolyDataToImageStencil::New();
  surfaceConverter->SetTolerance( 0.0 );
  surfaceConverter->ReleaseDataFlagOn();

  surfaceConverter->SetInput( normalsFilter->GetOutput() );
  normalsFilter->Delete();

  vtkImageChangeInformation *indexCoordinatesImageFilter = vtkImageChangeInformation::New();
  indexCoordinatesImageFilter->SetInput(( (mitk::Image*)GetImage() )->GetVtkImageData( time ));
  indexCoordinatesImageFilter->SetOutputSpacing(1.0,1.0,1.0);
  indexCoordinatesImageFilter->SetOutputOrigin(0.0,0.0,0.0);

  vtkImageCast * castFilter = vtkImageCast::New();
  castFilter->ReleaseDataFlagOn();
  castFilter->SetOutputScalarTypeToInt();
  castFilter->SetInput( indexCoordinatesImageFilter->GetOutput() );
  indexCoordinatesImageFilter->Delete();

  vtkImageStencil * stencil = vtkImageStencil::New();
  stencil->SetBackgroundValue( m_BackgroundValue );
  stencil->ReverseStencilOff();
  stencil->ReleaseDataFlagOn();

  stencil->SetStencil( surfaceConverter->GetOutput() );
  surfaceConverter->Delete();

  stencil->SetInput( castFilter->GetOutput() );
  castFilter->Delete();

  if (m_MakeOutputBinaryOn)
  {
    vtkImageThreshold * threshold = vtkImageThreshold::New();
    threshold->SetInput( stencil->GetOutput() );
    stencil->Delete();
    threshold->ThresholdByUpper(1);
    threshold->ReplaceInOn();
    threshold->ReplaceOutOn();
    threshold->SetInValue(1);
    threshold->SetOutValue(0);
//    threshold->SetOutputScalarTypeToUnsignedChar();
    threshold->Update();

    mitk::Image::Pointer output = this->GetOutput();
    output->SetVolume( threshold->GetOutput()->GetScalarPointer(), time );

    threshold->Delete();
  }
  else
  {
    stencil->Update();
    mitk::Image::Pointer output = this->GetOutput();
    output->SetVolume( stencil->GetOutput()->GetScalarPointer(), time );
    std::cout << "stencil ref count: " << stencil->GetReferenceCount() << std::endl;

    stencil->Delete();
  }
}


const mitk::Surface *mitk::SurfaceToImageFilter::GetInput(void)
{
  if (this->GetNumberOfInputs() < 1)
  {
    return 0;
  }

  return static_cast<const mitk::Surface * >
    ( this->ProcessObject::GetInput(0) );
}

void mitk::SurfaceToImageFilter::SetInput(const mitk::Surface *input)
{
  // Process object is not const-correct so the const_cast is required here
  this->ProcessObject::SetNthInput(0, 
    const_cast< mitk::Surface * >( input ) );
}

void mitk::SurfaceToImageFilter::SetImage(const mitk::Image *source)
{
  this->ProcessObject::SetNthInput( 1, const_cast< mitk::Image * >( source ) );	
}

const mitk::Image *mitk::SurfaceToImageFilter::GetImage(void)
{
  return static_cast< const mitk::Image * >(this->ProcessObject::GetInput(1));		
}
