/*=========================================================================
  Program:   Insight Segmentation & Registration Toolkit
  Module:    itkIPLCommonImageIO.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

  =========================================================================*/
#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif

#include <itksys/SystemTools.hxx>
#include "itkIOCommon.h"
#include "itkIPLCommonImageIO.h"
#include "itkExceptionObject.h"
#include "itkByteSwapper.h"
#include "itkGEImageHeader.h"
//#include "idbm_hdr_def.h"
#include "itkDirectory.h"
#include "itkMetaDataObject.h"
#include <iostream>
#include <fstream>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <vector>
#ifdef __BORLANDC__
#include <dxtmpl.h>
#endif

//From uiig library "The University of Iowa Imaging Group-UIIG"

namespace itk 
{
// Default constructor
IPLCommonImageIO::IPLCommonImageIO()
{
  m_system_byteOrder = ByteSwapper<int>::SystemIsBigEndian() ? ImageIOBase::BigEndian :
    ImageIOBase::LittleEndian;
  m_ImageHeader = 0;
  m_fnlist = new IPLFileNameList;
}

IPLCommonImageIO::~IPLCommonImageIO()
{
  if(m_ImageHeader != 0)
    delete m_ImageHeader;
  delete m_fnlist;
}

void IPLCommonImageIO::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}

bool IPLCommonImageIO::CanWriteFile(const char * )
{
  return false;
}

const std::type_info& IPLCommonImageIO::GetPixelType() const
{
  return typeid(S16);
}

unsigned int IPLCommonImageIO::GetComponentSize() const
{
  return sizeof(S16);
}
void IPLCommonImageIO::Read(void* buffer)
{
  S16 *img_buffer = (S16 *)buffer;
  IPLFileNameList::IteratorType it = m_fnlist->begin();
  IPLFileNameList::IteratorType itend = m_fnlist->end();

  for(;it != itend; it++)
    {
    std::ifstream f((*it)->GetimageFileName().c_str(),
                    std::ios::binary | std::ios::in);

    //std::cerr << (*it)->imageFileName << std::endl; std::cerr.flush();
    if(!f.is_open())
      RAISE_EXCEPTION();
    f.seekg ((*it)->GetSliceOffset(), std::ios::beg);
    if(!this->ReadBufferAsBinary(f, img_buffer, m_fnlist->GetXDim() * m_fnlist->GetYDim() * sizeof(S16)))
      {
      f.close();
      RAISE_EXCEPTION();
      }
    f.close();
    // ByteSwapper::SwapRangeFromSystemToBigEndian is set up based on
    // the FILE endian-ness, not as the name would lead you to believe.
    // So, on LittleEndian systems, SwapFromSystemToBigEndian will swap.
    // On BigEndian systems, SwapFromSystemToBigEndian will do nothing.
    itk::ByteSwapper<S16>::SwapRangeFromSystemToBigEndian(img_buffer,m_fnlist->GetXDim()*m_fnlist->GetYDim());
    img_buffer += m_fnlist->GetXDim() * m_fnlist->GetYDim();
    }
#if 0 // Debugging
  std::ofstream f2("test.img",std::ios::binary | std::ios::out);
  f2.write(buffer,(m_fnlist->numImageInfoStructs *
                   m_fnlist->GetXDim() * m_fnlist->GetYDim() * sizeof(S16)));
  f2.close();
#endif
}
struct GEImageHeader *IPLCommonImageIO::ReadHeader(const char * )
{
  //
  // must be redefined in a child class
  //
  return 0;
}

bool IPLCommonImageIO::CanReadFile( const char* )
{
  //
  // must be redefined in child class or you'll never read anything ;-)
  //
  return false;
}

void IPLCommonImageIO::ReadImageInformation()
{
  std::string FileNameToRead = this->GetFileName();
  this->m_ImageHeader = this->ReadHeader(FileNameToRead.c_str());
  //std::cerr << "HEADER SIZE " << m_ImageHeader->offset << std::endl;
  //
  // if anything fails in the header read, just let
  // exceptions propogate up.
  AddElementToList(m_ImageHeader->filename,
                   m_ImageHeader->sliceLocation,
                   m_ImageHeader->offset,
                   m_ImageHeader->imageXsize,
                   m_ImageHeader->imageYsize,
                   m_ImageHeader->seriesNumber,
                   m_ImageHeader->echoNumber);
    
  // Add header info to metadictionary
    
  itk::MetaDataDictionary &thisDic=this->GetMetaDataDictionary();
  std::string classname(this->GetNameOfClass());
  itk::EncapsulateMetaData<std::string>(thisDic,ITK_InputFilterName,
                                        classname);
  itk::EncapsulateMetaData<std::string>(thisDic,
                                        ITK_OnDiskStorageTypeName,
                                        std::string("SHORT"));
  itk::EncapsulateMetaData<short int>(thisDic,ITK_OnDiskBitPerPixel,(short int)16);
    
    
  itk::IOCommon::ValidOrientationFlags orient;
  switch(m_ImageHeader->imagePlane)
    {
    case GE_AXIAL:
      orient = IOCommon::ITK_ORIENTATION_IRP_TRANSVERSE;
      break;
    case GE_SAGITTAL:
      orient = IOCommon::ITK_ORIENTATION_IRP_SAGITTAL  ;
      break;
    case GE_CORONAL:
      // fall thru
    default:
      orient = IOCommon::ITK_ORIENTATION_IRP_CORONAL   ;
    }
  itk::EncapsulateMetaData<itk::IOCommon::ValidOrientationFlags>(thisDic,ITK_Orientation,orient);
  itk::EncapsulateMetaData<std::string>(thisDic,ITK_PatientID,std::string(m_ImageHeader->patientId));
  itk::EncapsulateMetaData<std::string>(thisDic,ITK_ExperimentDate,std::string(m_ImageHeader->date));


  //
  // GE images are stored in separate files per slice.
  //char imagePath[IOCommon::ITK_MAXPATHLEN+1];
  //TODO -- use std::string instead of C strings
  char imageMask[IOCommon::ITK_MAXPATHLEN+1];
  char imagePath[IOCommon::ITK_MAXPATHLEN+1];
  std::string _imagePath =
    itksys::SystemTools::CollapseFullPath(FileNameToRead.c_str());

  if(_imagePath == "")
    RAISE_EXCEPTION();
  strncpy(imagePath,_imagePath.c_str(),sizeof(imagePath));
  imagePath[IOCommon::ITK_MAXPATHLEN] = '\0';
  strncpy(imageMask,imagePath,sizeof(imageMask));
  imageMask[IOCommon::ITK_MAXPATHLEN] = '\0';

  char *lastslash = strrchr(imagePath,'/');
  if(lastslash == NULL)
    {
    strcpy(imagePath,".");
    }
  else
    {
    *lastslash = '\0';
    }
  itk::Directory::Pointer dir = itk::Directory::New();
  if(dir->Load(imagePath) == 0)
    RAISE_EXCEPTION();
  std::vector<std::string>::size_type i;
  std::vector<std::string>::size_type numfiles;
    
  struct GEImageHeader *curImageHeader;

  for(i = 0, numfiles = dir->GetNumberOfFiles(); i < numfiles; i++) 
    {
    const char *curFname =  dir->GetFile(i);
    if(curFname == 0)
      {
      break;
      }
    else if (FileNameToRead == curFname) //strcmp(curFname,FileNameToRead) == 0)
      {
      continue;
      }
    char fullPath[IOCommon::ITK_MAXPATHLEN+1];
    sprintf(fullPath,"%s/%s",imagePath,curFname);
    try 
      {
      curImageHeader = this->ReadHeader(fullPath);
      }
    catch (itk::ExceptionObject e)
      {
      // ReadGE4XHeader throws an exception on any error.
      // So if, for example we run into a subdirectory, it would
      // throw an exception, and we'd just want to skip it.
      continue;
      }
    if(curImageHeader->echoNumber == m_fnlist->GetKey2() &&
       curImageHeader->seriesNumber == m_fnlist->GetKey1())
      {
      AddElementToList(curImageHeader->filename,
                       curImageHeader->sliceLocation,
                       curImageHeader->offset,
                       curImageHeader->imageXsize,
                       curImageHeader->imageYsize,
                       curImageHeader->seriesNumber,
                       curImageHeader->echoNumber);
      }
    delete curImageHeader;
    }
  switch(m_ImageHeader->imagePlane)
    {
    case AXIAL:  //Axial needs to descend
      sortImageListDescend ();
      break;
    case CORONAL: //Fall through and ascend
    case SAGITTAL:
      sortImageListAscend ();
      break;
    default:
      break;
    }
  //
  //
  // set the image properties
  this->SetNumberOfDimensions(3);
  this->SetDimensions(0,m_ImageHeader->imageXsize);
  this->SetDimensions(1,m_ImageHeader->imageYsize);
  this->SetDimensions(2,m_fnlist->NumFiles());
  this->SetSpacing(0, m_ImageHeader->imageXres);
  this->SetSpacing(1, m_ImageHeader->imageYres);
  this->SetSpacing(2, m_ImageHeader->sliceThickness);
    
}



/**
   *
   */
void
IPLCommonImageIO
::WriteImageInformation(void)
{
  RAISE_EXCEPTION();
}


/**
   *
   */
void
IPLCommonImageIO
::Write( const void * )
{
  RAISE_EXCEPTION();
}

int
IPLCommonImageIO
::GetStringAt(std::ifstream &f,
              std::streamoff Offset,
              char *buf,
              size_t amount,bool throw_exception)
{
  f.seekg(Offset,std::ios::beg);
  if( f.fail() )
    { 
    if(throw_exception)
      {
      RAISE_EXCEPTION(); 
      }
    else
      {
      return -1;
      }
    }
  if( !this->ReadBufferAsBinary( f, (void *)buf, amount ) )
    { 
    if(throw_exception)
      {
      RAISE_EXCEPTION(); 
      }
    else
      {
      return -1;
      }
    }
  return 0;
}
int IPLCommonImageIO
::GetIntAt(std::ifstream &f,std::streamoff Offset,int *ip,
           bool )
{
  int tmp;
  this->GetStringAt(f,Offset,(char *)&tmp,sizeof(int));
  *ip = this->hdr2Int((char *)&tmp);
  return 0;
}
int IPLCommonImageIO
::GetShortAt(std::ifstream &f,std::streamoff Offset,short *ip,
             bool )
{
  short tmp;
  this->GetStringAt(f,Offset,(char *)&tmp,sizeof(short));
  *ip = this->hdr2Short((char *)&tmp);
  return 0;
}
int IPLCommonImageIO
::GetFloatAt(std::ifstream &f,std::streamoff Offset,float *ip,
             bool )
{
  float tmp;
  this->GetStringAt(f,Offset,(char *)&tmp,sizeof(float));
  *ip = this->hdr2Float((char *)&tmp);
  return 0;
}
int IPLCommonImageIO
::GetDoubleAt(std::ifstream &f,std::streamoff Offset,double *ip,
              bool )
{
  double tmp;
  this->GetStringAt(f,Offset,(char *)&tmp,sizeof(double));
  *ip = this->hdr2Double((char *)&tmp);
  return 0;
}
short IPLCommonImageIO
::hdr2Short (char *hdr)
{
  short shortValue;
  memcpy (&shortValue, hdr, sizeof(short));
  ByteSwapper<short int>::SwapFromSystemToBigEndian (&shortValue);
  return (shortValue);
}

int IPLCommonImageIO
::hdr2Int (char *hdr)
{
  int intValue;

  memcpy (&intValue, hdr, sizeof(int));
  ByteSwapper< int>::SwapFromSystemToBigEndian (&intValue);
  return (intValue);
}

float IPLCommonImageIO
::hdr2Float (char *hdr)
{
  float floatValue;

  memcpy (&floatValue, hdr, 4);
  ByteSwapper<float>::SwapFromSystemToBigEndian (&floatValue);

  return (floatValue);
}

double IPLCommonImageIO
::hdr2Double (char *hdr)
{
  double doubleValue;

  memcpy (&doubleValue, hdr, sizeof(double));
  ByteSwapper<double>::SwapFromSystemToBigEndian (&doubleValue);

  return (doubleValue);
}

int IPLCommonImageIO
::AddElementToList(char const * const filename, const float sliceLocation, const int offset, const int XDim, const int YDim, const int Key1, const int Key2 )
{
  if(m_fnlist->NumFiles() == 0)
    {
    m_fnlist->SetXDim(XDim);
    m_fnlist->SetYDim(YDim);
    m_fnlist->SetKey1(Key1);
    m_fnlist->SetKey2(Key2);
    }
  else if(XDim != m_fnlist->GetXDim() || YDim != m_fnlist->GetYDim()  )
    {
    return 0;
    }
  else if (m_fnlist->GetKey1() != Key1 ||  m_fnlist->GetKey2() != Key2)
    {
    return 1;  //It is OK for keys to not match,  Just don't add.
    }
  m_fnlist->AddElementToList(filename,sliceLocation,
                            0,XDim,YDim,0,Key1,Key2);
  return 1;
}

void IPLCommonImageIO
::sortImageListAscend ()
{
  m_fnlist->sortImageListAscend();
  return;
}
void IPLCommonImageIO
::sortImageListDescend ()
{
  m_fnlist->sortImageListDescend();
  return;
}
int IPLCommonImageIO
::statTimeToAscii (void *clock, char *timeString)
{
  char *asciiTime;
  unsigned int i;
#ifdef SGI
  timespec_t *lclock;
#else

#endif

#ifdef SGI
  lclock = (timespec_t *) clock;
  asciiTime = ctime (&(lclock->tv_sec));
#else
  time_t tclock = (time_t) *((int *) clock);
  asciiTime = ctime (&tclock);
#endif

  strncpy (timeString, asciiTime, 64);

  for (i = 0; i < 26; i++)
    {
    if (timeString[i] == '\n')
      timeString[i] = '\0';
    }

  return 1;

}


} // end namespace itk
