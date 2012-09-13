/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "itkDCMTKImageIO.h"

#include "itkByteSwapper.h"
#include "itksys/SystemTools.hxx"
#include "itkDCMTKFileReader.h"
#include <iostream>

#include "dcmtk/dcmimgle/dcmimage.h"
#include "dcmtk/dcmjpeg/djdecode.h"
#include "dcmtk/dcmjpls/djdecode.h"
#include "dcmtk/dcmdata/dcrledrg.h"

namespace itk
{
/** Constructor */
DCMTKImageIO::DCMTKImageIO()
{
  m_DImage = NULL;

  // standard ImageIOBase variables
  m_ByteOrder = BigEndian;
  this->SetNumberOfDimensions(3); // otherwise, things go crazy w/dir cosines
  m_PixelType = SCALAR;
  m_ComponentType = UCHAR;
  //m_FileType =

  // specific members
  m_UseJPEGCodec = false;
  m_UseJPLSCodec = false;
  m_UseRLECodec  = false;

  this->AddSupportedWriteExtension(".dcm");
  this->AddSupportedWriteExtension(".DCM");
  this->AddSupportedWriteExtension(".dicom");
  this->AddSupportedWriteExtension(".DICOM");

  // this->AddSupportedReadExtension(".dcm");
  // this->AddSupportedReadExtension(".DCM");
  // this->AddSupportedReadExtension(".dicom");
  // this->AddSupportedReadExtension(".DICOM");
}

/** Destructor */
DCMTKImageIO::~DCMTKImageIO()
{}

bool DCMTKImageIO::CanReadFile(const char *filename)
{
  // First check the filename extension
  std::string fname = filename;

  if ( fname == "" )
    {
    itkDebugMacro(<< "No filename specified.");
    }

  bool                   extensionFound = false;
  std::string::size_type dcmPos = fname.rfind(".dcm");
  if ( ( dcmPos != std::string::npos )
       && ( dcmPos == fname.length() - 4 ) )
    {
    extensionFound = true;
    }

  dcmPos = fname.rfind(".DCM");
  if ( ( dcmPos != std::string::npos )
       && ( dcmPos == fname.length() - 4 ) )
    {
    extensionFound = true;
    }

  dcmPos = fname.rfind(".dicom");
  if ( ( dcmPos != std::string::npos )
       && ( dcmPos == fname.length() - 6 ) )
    {
    extensionFound = true;
    }

  dcmPos = fname.rfind(".DICOM");
  if ( ( dcmPos != std::string::npos )
       && ( dcmPos == fname.length() - 6 ) )
    {
    extensionFound = true;
    }

  if ( !extensionFound )
    {
    itkDebugMacro(<< "The filename extension is not recognized");
    return false;
    }

  // check the content by reading the header only (to start)
  return true;
}

bool DCMTKImageIO::CanWriteFile(const char *name)
{
  std::string fname = name;

  if ( fname == "" )
    {
    itkDebugMacro(<< "No filename specified.");
    }

  bool                   extensionFound = false;
  std::string::size_type dcmPos = fname.rfind(".dcm");
  if ( ( dcmPos != std::string::npos )
       && ( dcmPos == fname.length() - 4 ) )
    {
    extensionFound = true;
    }

  dcmPos = fname.rfind(".DCM");
  if ( ( dcmPos != std::string::npos )
       && ( dcmPos == fname.length() - 4 ) )
    {
    extensionFound = true;
    }

  dcmPos = fname.rfind(".dicom");
  if ( ( dcmPos != std::string::npos )
       && ( dcmPos == fname.length() - 6 ) )
    {
    extensionFound = true;
    }

  dcmPos = fname.rfind(".DICOM");
  if ( ( dcmPos != std::string::npos )
       && ( dcmPos == fname.length() - 6 ) )
    {
    extensionFound = true;
    }

  if ( !extensionFound )
    {
    itkDebugMacro(<< "The filename extension is not recognized");
    return false;
    }

  if ( extensionFound )
    {
    return true;
    }
  return false;
}

// Internal function to rescale pixel according to Rescale Slope/Intercept
template< class TBuffer >
void RescaleFunction(TBuffer *buffer,
                     double slope,
                     double intercept,
                     size_t size)
{
  for ( unsigned int i = 0; i < size; i++ )
    {
    double tmp = static_cast< double >( buffer[i] ) * slope;
    tmp += intercept;
    buffer[i] = static_cast< TBuffer >( tmp );
    }
}

//------------------------------------------------------------------------------
void DCMTKImageIO::Read(void *buffer)
{
  // start simple
  if( m_DImage == NULL )
    {
    m_DImage = new DicomImage( m_FileName.c_str() );
    }
  else
    {
    if( !m_DicomImageSetByUser )
      {
      delete m_DImage;
      m_DImage = new DicomImage( m_FileName.c_str() );
      }
    }
  if(m_DImage == 0)
    {
    return; // throw exception perhaps?
    }
  if (m_DImage->getStatus() == EIS_Normal)
    {
    m_Dimensions[0] = (unsigned int)(m_DImage->getWidth());
    m_Dimensions[1] = (unsigned int)(m_DImage->getHeight());
    // m_Spacing[0] =
    // m_Spacing[1] =
    // m_Origin[0] =
    // m_Origin[1] =

    // pick a size for output image (should get it from DCMTK in the ReadImageInformation()))
    // NOTE ALEX: EP_Representation is made for that
    // but i don t know yet where to fetch it from
    unsigned bitdepth;
    unsigned scalarSize;
    switch(this->m_ComponentType)
      {
      case UCHAR:
        scalarSize = sizeof(unsigned char);
        bitdepth = scalarSize * 8;
        break;
      case CHAR:
        scalarSize = sizeof(char);
        bitdepth = scalarSize * 8;
        break;
      case USHORT:
        scalarSize = sizeof(unsigned short);
        bitdepth = scalarSize * 8;
        break;
      case SHORT:
        scalarSize = sizeof(short);
        bitdepth = scalarSize * 8;
        break;
      case UINT:
        scalarSize = sizeof(unsigned int);
        bitdepth = scalarSize * 8;
        break;
      case INT:
        scalarSize = sizeof(int);
        bitdepth = scalarSize * 8;
        break;
      case ULONG:
        scalarSize = sizeof(unsigned long);
        bitdepth = scalarSize * 8;
        break;
      case LONG:
        scalarSize = sizeof(long);
        bitdepth = scalarSize * 8;
        break;
      case UNKNOWNCOMPONENTTYPE:
      case FLOAT:
      case DOUBLE:
        itkExceptionMacro(<< "Bad component type" <<
                          ImageIOBase::GetComponentTypeAsString(this->m_ComponentType));
        break;
      }
    // get the image in the DCMTK buffer
    unsigned long len = m_DImage->getOutputDataSize(bitdepth);
    m_DImage->getOutputData(buffer, len, bitdepth,0);
    if(this->m_RescaleSlope != 1.0 || m_RescaleIntercept != 0)
      {
      size_t numElts = len / scalarSize;
      switch ( this->m_ComponentType )
        {
        case CHAR:
          RescaleFunction(static_cast< char * >( buffer ),
                          this->m_RescaleSlope,
                          this->m_RescaleIntercept, numElts);
          break;
        case UCHAR:
          RescaleFunction(static_cast< unsigned char * >( buffer ),
                          this->m_RescaleSlope,
                          this->m_RescaleIntercept, numElts);
          break;
        case SHORT:
          RescaleFunction(static_cast< short * >( buffer ),
                          this->m_RescaleSlope,
                          this->m_RescaleIntercept, numElts);
          break;
        case USHORT:
          RescaleFunction(static_cast< unsigned short * >( buffer ),
                          this->m_RescaleSlope,
                          this->m_RescaleIntercept, numElts);
          break;
        case INT:
          RescaleFunction(static_cast< int * >( buffer ),
                          this->m_RescaleSlope,
                          this->m_RescaleIntercept, numElts);
          break;
        case UINT:
          RescaleFunction(static_cast< unsigned int * >( buffer ),
                          this->m_RescaleSlope,
                          this->m_RescaleIntercept, numElts);
          break;
        case LONG:
          RescaleFunction(static_cast< long * >( buffer ),
                          this->m_RescaleSlope,
                          this->m_RescaleIntercept, numElts);
          break;
        case ULONG:
          RescaleFunction(static_cast< unsigned long * >( buffer ),
                          this->m_RescaleSlope,
                          this->m_RescaleIntercept, numElts);
          break;
        case FLOAT:
          RescaleFunction(static_cast< float * >( buffer ),
                          this->m_RescaleSlope,
                          this->m_RescaleIntercept, numElts);
          break;
        case DOUBLE:
          RescaleFunction(static_cast< double * >( buffer ),
                          this->m_RescaleSlope,
                          this->m_RescaleIntercept, numElts);
          break;
        default:
          if ( this->GetPixelType() == SCALAR )
            {
            itkExceptionMacro(<< "Datatype: "
                              << this->GetComponentTypeAsString(this->m_ComponentType)
                              << " not supported");
            }
        }

      }
    }
  else
    {
    std::cerr << "Error: cannot load DICOM image (";
    std::cerr << DicomImage::getString(m_DImage->getStatus());
    std::cerr << ")" << std::endl;
    }

}

/**
 *  Read Information about the DICOM file
 */
void DCMTKImageIO::ReadImageInformation()
{

  DJDecoderRegistration::registerCodecs();
  DcmRLEDecoderRegistration::registerCodecs();

  DCMTKFileReader reader;
  reader.SetFileName(this->m_FileName);
  try
    {
    reader.LoadFile();
    }
  catch(...)
    {
    std::cerr << "DCMTKImageIO::ReadImageInformation: "
              << "DicomImage could not read the file." << std::endl;
    }
  unsigned short rows,columns;
  reader.GetDimensions(rows,columns);
  this->m_Dimensions[0] = rows;
  this->m_Dimensions[1] = columns;
  this->m_Dimensions[2] = reader.GetFrameCount();

  vnl_vector<double> dir1(3),dir2(3),dir3(3);
  reader.GetDirCosines(dir1,dir2,dir3);
  this->SetDirection(0,dir1);
  this->SetDirection(1,dir2);
  if(this->m_NumberOfDimensions > 2 && this->m_Dimensions[2] != 1)
    {
    this->SetDirection(2,dir3);
    }
  // get slope and intercept
  reader.GetSlopeIntercept(this->m_RescaleSlope,this->m_RescaleIntercept);
  this->m_ComponentType = reader.GetImageDataType();
  this->m_PixelType = reader.GetImagePixelType();
  double spacing[3];
  double origin[3];
  reader.GetSpacing(spacing);
  reader.GetOrigin(origin);
  this->m_Origin.resize(3);
  for(unsigned i = 0; i < 3; i++)
    {
    this->m_Origin[i] = origin[i];
    }

  unsigned spacingLimit = 2;
  if(this->m_NumberOfDimensions > 2 && this->m_Dimensions[2] != 1)
    {
    spacingLimit = 3;
    }
  this->m_Spacing.resize(spacingLimit);
  for(unsigned i = 0; i < spacingLimit; i++)
    {
    this->m_Spacing[i] = spacing[i];
    }

  if(this->m_DImage != 0)
    {
    delete m_DImage;
    }
  this->m_DImage = new DicomImage(this->m_FileName.c_str());
  EP_Representation pixelRep = this->m_DImage->getInterData()->getRepresentation();
  switch(pixelRep)
    {
    case EPR_Uint8:
      this->m_ComponentType = UCHAR; break;
    case EPR_Sint8:
      this->m_ComponentType = CHAR; break;
    case EPR_Uint16:
      this->m_ComponentType = USHORT; break;
    case EPR_Sint16:
      this->m_ComponentType = SHORT; break;
    case EPR_Uint32:
      this->m_ComponentType = UINT; break;
    case EPR_Sint32:
      this->m_ComponentType = INT; break;
    default: // HACK should throw exception
      this->m_ComponentType = USHORT; break;
    }
  int numPlanes = this->m_DImage->getInterData()->getPlanes();
  switch(numPlanes)
    {
    case 1:
      this->m_PixelType = SCALAR; break;
    case 2:
      // hack, supposedly Luminence/Alpha
      this->m_PixelType = VECTOR; break;
      break;
    case 3:
      this->m_PixelType = RGB; break;
    case 4:
      this->m_PixelType = RGBA; break;
    }
}

void
DCMTKImageIO
::WriteImageInformation(void)
{}

/** */
void
DCMTKImageIO
::Write(const void *buffer)
{
  (void)(buffer);
}

/** Print Self Method */
void DCMTKImageIO::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}
} // end namespace itk
