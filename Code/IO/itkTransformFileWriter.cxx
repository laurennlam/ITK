/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    itkTransformFileWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __itkTransformFileWriter_cxx
#define __itkTransformFileWriter_cxx

#include "itkTransformFileWriter.h"
#include "metaScene.h"
#include "itkBSplineDeformableTransform.h"
#include "itkRigid2DTransform.h"
#include "itkAffineTransform.h"

namespace itk
{

TransformFileWriter
::TransformFileWriter()
{
  m_FileName = "";
}

TransformFileWriter
::~TransformFileWriter()
{
}

/** Set the input transform and reinitialize the list of transforms */
void TransformFileWriter::SetInput(const TransformType* transform)
{
  m_TransformList.clear();
  m_TransformList.push_back(transform);
}

/** Add a transform to be written */
void TransformFileWriter::AddTransform(const TransformType* transform)
{
  m_TransformList.push_back(transform);
}


#define ITK_CONVERT_ITK_BSPLINEDEFORMABLE_TOMETAIOTRANSFORM(scalartype,dimension,order)\
  if( ((*it)->GetInputSpaceDimension() == (int)(dimension)) \
    )\
  {\
  transform->TransformOrder(3);\
  double* gridRegionSize = new double[(*it)->GetInputSpaceDimension()];\
  double* gridOrigin = new double[(*it)->GetInputSpaceDimension()];\
  double* gridSpacing = new double[(*it)->GetInputSpaceDimension()];\
  for(unsigned int i = 0; i< (*it)->GetInputSpaceDimension(); i++)\
    {\
    gridRegionSize[i] = reinterpret_cast<const itk::BSplineDeformableTransform<scalartype,dimension,order>*>(*it)->GetGridRegion().GetSize()[i];\
    gridOrigin[i] = reinterpret_cast<const itk::BSplineDeformableTransform<scalartype,dimension,order>*>(*it)->GetGridOrigin()[i];\
    gridSpacing[i] = reinterpret_cast<const itk::BSplineDeformableTransform<scalartype,dimension,order>*>(*it)->GetGridSpacing()[i];\
    }\
  transform->GridRegionSize(gridRegionSize);\
  transform->GridOrigin(gridOrigin);\
  transform->GridSpacing(gridSpacing);\
  delete[] gridRegionSize;\
  delete[] gridOrigin;\
  delete[] gridSpacing;\
  }\


/** Update the writer */
void TransformFileWriter
::Update()
{  
  MetaScene scene(3);
  std::list<const TransformType *>::iterator it = m_TransformList.begin();

  while(it != m_TransformList.end())
    {
    MetaTransform* transform = new MetaTransform;
    transform->InitializeEssential((*it)->GetInputSpaceDimension());
    transform->ObjectSubTypeName((*it)->GetNameOfClass());
    transform->Parameters((*it)->GetNumberOfParameters(),(*it)->GetParameters().data_block());

    // Add the Transformations here
    if(!strcmp((*it)->GetNameOfClass(),"AffineTransform") ||
       !strcmp((*it)->GetNameOfClass(),"CenteredAffineTransform") ||
       !strcmp((*it)->GetNameOfClass(),"FixedCenterOfRotationAffineTransform") ||
       !strcmp((*it)->GetNameOfClass(),"ScaleLogarithmicTransform") ||
       !strcmp((*it)->GetNameOfClass(),"ScaleTransform") ||
       !strcmp((*it)->GetNameOfClass(),"CenteredAffineTransform") ||
       !strcmp((*it)->GetNameOfClass(),"QuaternionRigidTransform") ||
       !strcmp((*it)->GetNameOfClass(),"CenteredEuler3DTransform") ||
       !strcmp((*it)->GetNameOfClass(),"CenteredRigid2DTransform") ||
       !strcmp((*it)->GetNameOfClass(),"Rigid3DPerspectiveTransform") ||
       !strcmp((*it)->GetNameOfClass(),"Rigid3DTransform") ||
       !strcmp((*it)->GetNameOfClass(),"Euler3DTransform") ||
       !strcmp((*it)->GetNameOfClass(),"VersorRigid3DTransform") ||
       !strcmp((*it)->GetNameOfClass(),"ScaleSkewVersor3DTransform") 
      )
      {
      double* cor = new double[(*it)->GetInputSpaceDimension()];
      for(unsigned int i=0;i<(*it)->GetInputSpaceDimension();i++)
        {
        cor[i] = reinterpret_cast<const AffineTransform<>*>(*it)->GetCenter()[i];
        }
      transform->CenterOfRotation(cor);
      delete cor;
      }
    else if(
       !strcmp((*it)->GetNameOfClass(),"Similarity2DTransform") ||
       !strcmp((*it)->GetNameOfClass(),"CenteredSimilarity2DTransform") ||
       !strcmp((*it)->GetNameOfClass(),"Rigid2DTransform") ||
       !strcmp((*it)->GetNameOfClass(),"Euler2DTransform")
      )
      {
      double* cor = new double[(*it)->GetInputSpaceDimension()];
      for(unsigned int i=0;i<(*it)->GetInputSpaceDimension();i++)
        {
        cor[i] = reinterpret_cast<const itk::Rigid2DTransform<>*>(*it)->GetCenter()[i];
        }
      transform->CenterOfRotation(cor);
      delete cor;
      }
    else if(!strcmp((*it)->GetNameOfClass(),"BSplineDeformableTransform"))
      {
      ITK_CONVERT_ITK_BSPLINEDEFORMABLE_TOMETAIOTRANSFORM(double,2,3)
      ITK_CONVERT_ITK_BSPLINEDEFORMABLE_TOMETAIOTRANSFORM(double,3,3)
      }    
    
    scene.AddObject(transform);
    it++;
    }

  // Write out the transform
  scene.Write(m_FileName.c_str());
}


} // namespace itk

#endif
