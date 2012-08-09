
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

#include "DWISimpleCompareCLP.h"

namespace
{
#define DIMENSION 4

template <class PixelType>
std::vector< std::vector<double> >
RecoverGVector(typename itk::Image<PixelType,DIMENSION>::Pointer &img)
{
  std::vector< std::vector<double> > rval;

  itk::MetaDataDictionary &dict = img->GetMetaDataDictionary();

  for(unsigned curGradientVec = 0; ;++curGradientVec)
    {
    std::stringstream labelSS;
    labelSS << "DWMRI_gradient_" << std::setw(4) << std::setfill('0') << curGradientVec;
    std::string valString;
    // look for gradients in metadata until none by current name exists
    if(!itk::ExposeMetaData<std::string>(dict,labelSS.str(),valString))
      {
      break;
      }
    std::stringstream valSS(valString);
    std::vector<double> vec;
    for(;;)
      {
      double curVal;
      valSS >> curVal;
      if(!valSS.fail())
        {
        vec.push_back(curVal);
        }
      else
        {
        break;
        }
      }
    rval.push_back(vec);
    }
  return rval;
}

template <class PixelType>
int
RecoverBValue(typename itk::Image<PixelType,DIMENSION>::Pointer &img, double &val)
{
  std::string valString;

  itk::MetaDataDictionary &dict = img->GetMetaDataDictionary();

  if(!itk::ExposeMetaData<std::string>(dict,"DWMRI_b-value",valString))
    {
    return EXIT_FAILURE;
    }

  std::stringstream valSS(valString);

  valSS >> val;

  if(valSS.fail())
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

template <typename TValue>
bool
CloseEnough(TValue a, TValue b)
{
  double averageMag = (vcl_fabs(static_cast<double>(a)) +
                       vcl_fabs(static_cast<double>(b))) / 2.0;
  double diff = vcl_fabs(static_cast<double>(a) - static_cast<double>(b));
  // case one -- both near zero
  if(averageMag < 0.000001)
    return true;
  // case 2 -- diff > average / 100000;
  if(diff > (averageMag / 100000.0))
    {
    return false;
    }
  return true;
}

template <typename TVal>
bool
CloseEnough(const std::vector<TVal> &a, const std::vector<TVal> &b)
{
  if(a.size() != b.size())
    {
    std::cerr << "Vector size mismatch: "
              << a.size() << " "
              << b.size() << std::endl;
    return false;
    }
  for(unsigned i = 0; i < a.size(); ++i)
    {
    if(!CloseEnough(a[i],b[i]))
      {
      std::cerr << "Value mismatch" << std::endl;
      return false;
      }
    }
  return true;
}

template <class PixelType>
int DoIt( const std::string &inputVolume1, const std::string &inputVolume2, PixelType, bool CheckDWIData )
{

  typedef itk::Image<PixelType,DIMENSION> ImageType;
  typedef itk::ImageFileReader<ImageType> FileReaderType;

  typename FileReaderType::Pointer firstReader = FileReaderType::New();
  typename FileReaderType::Pointer secondReader = FileReaderType::New();

  firstReader->SetFileName( inputVolume1.c_str() );
  secondReader->SetFileName( inputVolume2.c_str() );

  firstReader->Update(); secondReader->Update();
  typename ImageType::Pointer firstImage = firstReader->GetOutput();
  typename ImageType::Pointer secondImage = secondReader->GetOutput();
  //
  // check origin -- problem with file conversion causing some drift
  typename ImageType::PointType firstOrigin(firstImage->GetOrigin());
  typename ImageType::PointType secondOrigin(secondImage->GetOrigin());
  double distance =
    vcl_sqrt(firstOrigin.SquaredEuclideanDistanceTo(secondOrigin));
  if(distance > 1.0E-3)
    {
    std::cerr << "Origins differ " << firstOrigin
              << " " << secondOrigin << std::endl;
    return EXIT_FAILURE;
    }
  else if(distance > 1.0E-6)
    {
    // if there is a small difference make them the same
    firstImage->SetOrigin(secondOrigin);
    }
  // same deal with spacing, can be slightly off due to numerical error
  typename ImageType::SpacingType firstSpacing(firstImage->GetSpacing());
  typename ImageType::SpacingType secondSpacing(secondImage->GetSpacing());
  for(unsigned int i = 0; i < ImageType::GetImageDimension(); ++i)
    {
    double diff = vcl_fabs(firstSpacing[i] - secondSpacing[i]);
    if(diff > 1.0e-6 && diff < 1.0e-4)
      {
      firstSpacing[i] = secondSpacing[i];
      }
    }
  firstImage->SetSpacing(firstSpacing);

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
  if(!CheckDWIData)
    {
    return EXIT_SUCCESS;
    }

  double bVal1, bVal2;
  if(RecoverBValue<PixelType>(firstImage,bVal1))
    {
    std::cerr << "Missing BValue in "
              << inputVolume1 << std::endl;
    return EXIT_FAILURE;
    }

  if(!RecoverBValue<PixelType>(secondImage,bVal2))
    {
    std::cerr << "Missing BValue in "
              << inputVolume2 << std::endl;
    return EXIT_FAILURE;
    }

  if(!CloseEnough(bVal1,bVal2))
    {
    std::cerr << "BValue mismatch: " << bVal1
              << " " << bVal2 << std::endl;
    return EXIT_FAILURE;
    }

  std::vector< std::vector<double> > firstGVector(RecoverGVector<PixelType>(firstImage)),
    secondGVector(RecoverGVector<PixelType>(secondImage));
  if(firstGVector.size() != secondGVector.size())
    {
    std::cerr << "First image Gradient Vectors size ("
              << firstGVector.size()
              << ") doesn't match second image Gradient vectors size ("
              << secondGVector.size() << ")" << std::endl;
    return EXIT_FAILURE;
    }
  if(!CloseEnough(firstGVector,secondGVector))
    {
    std::cerr << "Gradient vectors don't match" << std::endl;
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
        return DoIt( inputVolume1, inputVolume2, static_cast<unsigned char>(0),CheckDWIData );
        break;
      case itk::ImageIOBase::CHAR:
        return DoIt( inputVolume1, inputVolume2, static_cast<char>(0),CheckDWIData );
        break;
      case itk::ImageIOBase::USHORT:
        return DoIt( inputVolume1, inputVolume2, static_cast<unsigned short>(0),CheckDWIData );
        break;
      case itk::ImageIOBase::SHORT:
        return DoIt( inputVolume1, inputVolume2, static_cast<short>(0),CheckDWIData );
        break;
      case itk::ImageIOBase::UINT:
        return DoIt( inputVolume1, inputVolume2, static_cast<unsigned int>(0),CheckDWIData );
        break;
      case itk::ImageIOBase::INT:
        return DoIt( inputVolume1, inputVolume2, static_cast<int>(0),CheckDWIData );
        break;
      case itk::ImageIOBase::ULONG:
        return DoIt( inputVolume1, inputVolume2, static_cast<unsigned long>(0),CheckDWIData );
        break;
      case itk::ImageIOBase::LONG:
        return DoIt( inputVolume1, inputVolume2, static_cast<long>(0),CheckDWIData );
        break;
      case itk::ImageIOBase::FLOAT:
        return DoIt( inputVolume1, inputVolume2, static_cast<float>(0),CheckDWIData );
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
