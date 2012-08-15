#ifndef DWIConvertUtils_h
#define DWIConvertUtils_h
#include "itkVectorImage.h"
#include "itkImageFileWriter.h"
#include "itkImageFileReader.h"
#include <string>

typedef short PixelValueType;
typedef itk::Image< PixelValueType, 4 > VolumeType;
typedef itk::VectorImage<PixelValueType, 3> VectorVolumeType;

template <typename TArg>
int
CheckArg(const char *argName, const TArg &argVal, const TArg &emptyVal)
{
  if(argVal == emptyVal)
    {
    std::cerr << "Missing argument " << argName << std::endl;
    return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}

/** write a scalar short image
 */
template <typename TImage>
int
WriteVolume( typename TImage::Pointer &img, const std::string &fname )
{
  typename itk::ImageFileWriter< TImage >::Pointer imgWriter =
    itk::ImageFileWriter< TImage >::New();

  imgWriter->SetInput( img );
  imgWriter->SetFileName( fname.c_str() );
  try
    {
    imgWriter->Update();
    }
  catch (itk::ExceptionObject &excp)
    {
    std::cerr << "Exception thrown while writing "
              << fname << std::endl;
    std::cerr << excp << std::endl;
    return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}

template <typename TImage>
int
ReadVolume( typename TImage::Pointer &img, const std::string &fname )
{
  typename itk::ImageFileReader< TImage >::Pointer imgReader =
    itk::ImageFileReader< TImage >::New();

  imgReader->SetFileName( fname.c_str() );
  try
    {
    imgReader->Update();
    }
  catch (itk::ExceptionObject &excp)
    {
    std::cerr << "Exception thrown while reading "
              << fname << std::endl;
    std::cerr << excp << std::endl;
    return EXIT_FAILURE;
    }
  img = imgReader->GetOutput();
  return EXIT_SUCCESS;
}

#endif // DWIConvertUtils_h
