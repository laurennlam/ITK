/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    itkFloodFilledSpatialFunctionConditionalConstIterator.txx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef _itkFloodFilledSpatialFunctionConditionalConstIterator_txx
#define _itkFloodFilledSpatialFunctionConditionalConstIterator_txx

#include "itkFloodFilledSpatialFunctionConditionalConstIterator.h"

namespace itk
{

template<class TImage, class TFunction>
FloodFilledSpatialFunctionConditionalConstIterator<TImage, TFunction>
::FloodFilledSpatialFunctionConditionalConstIterator(const ImageType *imagePtr,
                                     FunctionType *fnPtr,
                                     IndexType startIndex): Superclass(imagePtr, fnPtr, startIndex)
{
  // The default inclusion strategy is "center"
  this->SetCenterInclusionStrategy();
}

template<class TImage, class TFunction>
FloodFilledSpatialFunctionConditionalConstIterator<TImage, TFunction>
::FloodFilledSpatialFunctionConditionalConstIterator(const ImageType *imagePtr,
                                     FunctionType *fnPtr): Superclass(imagePtr, fnPtr)
{
  // The default inclusion strategy is "center"
  this->SetCenterInclusionStrategy();
}

template<class TImage, class TFunction>
bool
FloodFilledSpatialFunctionConditionalConstIterator<TImage, TFunction>
::IsPixelIncluded(const IndexType & index) const
{
  // This temp var is used in all cases
  FunctionInputType position;

  switch( m_InclusionStrategy ) {
  
  // Origin
  case 0:
    {
    // Get the physical location of this index
    m_Image->TransformIndexToPhysicalPoint(index, position);

    // Evaluate the function at this point
    return m_Function->Evaluate(position);
    }
  break;
  
  // Center
  case 1:
    {
      // The center of the pixel is the index provided in the function
      // call converted to a continuous index with an offset of 0.5
      // along each dimension
      ContinuousIndex<double, TImage::ImageDimension> contIndex;

      for(unsigned int i = 0; i < TImage::ImageDimension; i ++ )
        {
        contIndex[i] = (double)index[i] + 0.5;
        }

      // Get the physical location of this index
      m_Image->TransformContinuousIndexToPhysicalPoint(contIndex, position);

      // Evaluate the function at this point
      return m_Function->Evaluate(position);
    }
  break;

  // Complete
  case 2:
    {
      // This is unfortunately a little complicated...
      // We want to examine whether or not all of the corners of this pixel
      // are within the spatial function. For a pixel at (0,0) with a spacing
      // of (1,1), this involves checking the following pixels:
      // (0,0) (0,1) (1,0) (1,1)
      // In other words, all possible permutations of adding either 0 or 1 to
      // the index of the pixel of interest. For an index of dimension n,
      // there are 2^n indices that need to be tested.
      // The simplest way to implement this is by counting in binary fashion
      // and adding the value of the appropriate binary digit to the corresponding
      // index location
      // Since I've chosen to implement this algorithm with an unsigned int counter,
      // it will only behave correctly for images with dimensions <= 16.
      // However, given that the number of function inclusion tests is 2^n as well,
      // it seems unlikely that anyone would want to use this for images larger than
      // 3 or 4 dimensions, most likely only 3. Cases 0 or 1 provide a constant time
      // means of determining index inclusion.

      // To reiterate... DO NOT use this on images higher than 16D
      unsigned int counter = 0;
      unsigned int counterCopy = 0;
      unsigned int dim = TImage::ImageDimension;
      unsigned int numReps = static_cast<unsigned int>( pow(
                                            static_cast<double>( 2.0 ),
                                            static_cast<double>( dim ) ) );

      IndexType tempIndex;

      // First we loop over the binary counter
      for(counter = 0; counter < numReps; counter++)
        {
        // Next we use the binary values in the counter to form
        // an index to look at
        for(unsigned int i = 0; i < dim; i++)
          {
            counterCopy = counter;
            tempIndex[i] = index[i] + static_cast<int>( (counterCopy >> i) & 0x0001 );
          }

        // Now that we've built an index, we can test it
        // Get the physical location of this index
        m_Image->TransformIndexToPhysicalPoint(tempIndex, position);

        // Evaluate the function at this index, if it's false
        // then the AND of all function dimensions is false,
        // and hence it's not included
        if( !(m_Function->Evaluate(position)) )
          return false;
        }
      
      // If we reach this point, we've tested all dimensions and none
      // were outside the function, therefore the pixel is inside
      return true;
    }
  break;

  // Intersect
  case 3:
    {
    // The notes for the previous case apply here as well
    // The only difference is that we return true if any of the
    // generated indices are true

    // To reiterate... DO NOT use this on images higher than 16D
    unsigned int counter = 0;
    unsigned int counterCopy = 0;
    unsigned int dim = TImage::ImageDimension;
    unsigned int numReps = static_cast<unsigned int>( pow(
                                          static_cast<double>( 2.0 ),
                                          static_cast<double>( dim ) ) );
    IndexType tempIndex;

    // First we loop over the binary counter
    for(counter = 0; counter < numReps; counter++)
      {
      // Next we use the binary values in the counter to form
      // an index to look at
      for(unsigned int i = 0; i < dim; i++)
        {
          counterCopy = counter;
          tempIndex[i] = index[i] + static_cast<int>( (counterCopy >> i) & 0x0001);
        }

      // Now that we've built an index, we can test it
      // Get the physical location of this index
      m_Image->TransformIndexToPhysicalPoint(tempIndex, position);

      // Evaluate the function at this index, if it's true
      // then the OR of all function dimensions is true,
      // and hence it's included
      if( m_Function->Evaluate(position) )
        return true;
      }
      
      // If we reach this point, we've tested all dimensions and none
      // were inside the function, therefore the pixel is outside
      return false;
    }
  } // end switch inclusion strategy

  // Somehow me managed to exit the switch statement without returning
  // To be safe, we'll say that the pixel is not inside
  return false;

}

} // end namespace itk

#endif
