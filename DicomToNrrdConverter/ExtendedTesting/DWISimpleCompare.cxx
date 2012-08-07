
#include <iostream>
#include <algorithm>
#include <string>
#include <itkMetaDataObject.h>
#include <itkImage.h>
#include <itkVector.h>
#include <itkVectorImage.h>

#include <itkImageFileWriter.h>
#include <itkImageFileReader.h>
#include <itkNrrdImageIO.h>

#include <itkImageRegionIterator.h>
#include <itkImageRegionConstIterator.h>
#include <itkSubtractImageFilter.h>
#include <itkStatisticsImageFilter.h>
#include <vcl_algorithm.h>

#include "DWICompareCLP.h"

namespace
{
#define DIMENSION 4

template <class PixelType>
int DoIt( int argc, char * argv[], PixelType )
{

  PARSE_ARGS;
  typedef itk::Image<PixelType,DIMENSION> ImageType;
  typedef itk::ImageFileReader<ImageType> FileReaderType;

  typename FileReaderType::Pointer firstReader = FileReaderType::New();
  typename FileReaderType::Pointer secondReader = FileReaderType::New();

  firstReader->SetFileName( inputVolume1.c_str() );
  secondReader->SetFileName( inputVolume2.c_str() );

  typedef itk::SubtractImageFilter<ImageType> SubtractFilterType;
  typename SubtractFilterType::Pointer subtractFilter =
    SubtractFilterType::New();
  subtractFilter->SetInput1(firstReader->GetOutput());
  subtractFilter->SetInput2(secondReader->GetOutput());

  typedef itk::StatisticsImageFilter<ImageType> StatisticsFilterType;
  typename StatisticsFilterType::Pointer statisticsFilter =
    StatisticsFilterType::New();
  statisticsFilter->SetInput(subtractFilter->GetOutput());
  try
    {
    statisticsFilter->Update();
    }
  catch( itk::ExceptionObject& e )
    {
    std::cerr << "Exception detected while comparing "
              << inputVolume1 << "  "
              << inputVolume2 << e.GetDescription();
    return EXIT_FAILURE;
    }
  if(vcl_fabs(static_cast<float>(statisticsFilter->GetMaximum())) > 0.0001 ||
     vcl_fabs(static_cast<float>(statisticsFilter->GetMinimum())) > 0.0001)
    {
    return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}

void GetImageType(std::string fileName,
                  itk::ImageIOBase::IOPixelType & pixelType,
                  itk::ImageIOBase::IOComponentType & componentType)
{
  typedef itk::Image<short, 3> ImageType;
  itk::ImageFileReader<ImageType>::Pointer imageReader =
    itk::ImageFileReader<ImageType>::New();
  imageReader->SetFileName(fileName.c_str() );
  imageReader->UpdateOutputInformation();

  pixelType = imageReader->GetImageIO()->GetPixelType();
  componentType = imageReader->GetImageIO()->GetComponentType();
}

}

int main( int argc, char * argv[] )
{

  PARSE_ARGS;

  itk::ImageIOBase::IOPixelType     pixelType;
  itk::ImageIOBase::IOComponentType componentType;

  try
    {
    // itk::GetImageType (inputVolume1, pixelType, componentType);
    GetImageType(inputVolume1, pixelType, componentType);

    // This filter handles all types

    switch( componentType )
      {
      case itk::ImageIOBase::UCHAR:
        return DoIt( argc, argv, static_cast<unsigned char>(0) );
        break;
      case itk::ImageIOBase::CHAR:
        return DoIt( argc, argv, static_cast<char>(0) );
        break;
      case itk::ImageIOBase::USHORT:
        return DoIt( argc, argv, static_cast<unsigned short>(0) );
        break;
      case itk::ImageIOBase::SHORT:
        return DoIt( argc, argv, static_cast<short>(0) );
        break;
      case itk::ImageIOBase::UINT:
        return DoIt( argc, argv, static_cast<unsigned int>(0) );
        break;
      case itk::ImageIOBase::INT:
        return DoIt( argc, argv, static_cast<int>(0) );
        break;
      case itk::ImageIOBase::ULONG:
        return DoIt( argc, argv, static_cast<unsigned long>(0) );
        break;
      case itk::ImageIOBase::LONG:
        return DoIt( argc, argv, static_cast<long>(0) );
        break;
      case itk::ImageIOBase::FLOAT:
        return DoIt( argc, argv, static_cast<float>(0) );
        // std::cout << "FLOAT type not currently supported." << std::endl;
        break;
      case itk::ImageIOBase::DOUBLE:
        std::cout << "DOUBLE type not currently supported." << std::endl;
        break;
      case itk::ImageIOBase::UNKNOWNCOMPONENTTYPE:
      default:
        std::cout << "unknown component type" << std::endl;
        break;
      }
    }
  catch( itk::ExceptionObject & excep )
    {
    std::cerr << argv[0] << ": exception caught !" << std::endl;
    std::cerr << excep << std::endl;
    return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
