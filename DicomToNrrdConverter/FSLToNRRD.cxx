#include <iostream>
#include <string>
#include <fstream>
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "FSLToNrrdCLP.h"

typedef short PixelValueType;
typedef itk::Image< PixelValueType, 3 > VolumeType;
typedef itk::Image< PixelValueTYpe, 4 > DWIVolumeType;
template <typename TArg>
void
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
int
WriteVolume( DWIVolumeType::Pointer &img, const std::string &fname )
{
  itk::ImageFileWriter< DWIVolumeType >::Pointer imgWriter =
    itk::ImageFileWriter< DWIVolumeType >::New();

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

int
ReadVolume( VolumeType::Pointer &img, const std::string &fname )
{
  itk::ImageFileReader< VolumeType >::Pointer imgReader =
    itk::ImageFileReader< VolumeType >::New();

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

int
ReadBVals(std::vector<double> &bVals, int & bValCount, const std::string &bValFilename)
{
  std::ifstream bValFile(bValFileName,std::ifstream::in);
  if(!bValFile.good())
    {
    std::cerr << "Failed to open " << bValFileName
              << std::endl;
    bVals.clear();
    bValCount = 0;
    while(!bValFile.eof())
      {
      double x;
      bValFile >> x;
      bVals.push_back(x);
      }
    return EXIT_FAILURE;
    }
}

int
ReadBVals(std::vector< std::vector<double> > &bVecs, int & bVecCount, const std::string &bVecFilename)
{
  std::ifstream bVecFile(bVecFileName,std::ifstream::in);
  if(!bVecFile.good())
    {
    std::cerr << "Failed to open " << bVecFileName
              << std::endl;
    bVecs.clear();
    bVecCount = 0;
    while(!bVecFile.eof())
      {
      std::vector<double> x;
      for(i = 0; i < 3; ++i)
        {
        if(bVecFile.eof())
          {
          std::cerr << "Malformed B Vector file " << bVecFilename << std::endl;
          return EXIT_FAILURE;
          }
        bValFile >> x[i];
        }
      bVecs.push_back(x);
      }
    return EXIT_FAILURE;
    }
}

int
main(int argc, char *argv[])
{
  PARSE_ARGS;
  CheckArg("Input Volume",inputVolume,"");
  CheckArg("Output Volume",outputVolume,"");
  CheckArg("B Values", BValues, "");
  CheckArg("B Vectors", BVectors, "");

  VolumeType::Pointer inputVol;
  if(ReadVolume(inputVol,inputVolume) != EXIT_SUCCESS)
    {
    return EXIT_FAILURE;
    }
  std::vector<double> BVals;
  std::vector< std::vector<double> > BVecs;
  int bValCount, bVecCount;
  if(ReadBVals(BVals,bValCount,BValues) != EXIT_SUCCESS)
    {
    return EXIT_FAILURE;
    }
  if(ReadBVecs(BVecs,bVecCount,BVectors) != EXIT_SUCCESS)
    {
    return EXIT_FAILURE;
    }
  if(bValCount != bVecCount)
    {
    std::cerr << "Mismatch between count of B Vectors ("
              << bVecCount << ") and B Values ("
              << bValCount << ")" << std::endl;
    return EXIT_FAILURE;
    }
  VolumeType::SizeType size3D(inputVol->GetLargetsPossibleRegion().GetSize());
  VolumeType::DirectionType direction3D(inputVol->GetDirection());
  VolumeType::SpacingType spacing3D(inputVol->GetSpacing());
  VolumeType::PointType origin3D(inputVol->GetOrigin());

  DWIVolumeType::SizeType size4D;
  size4D[0] = size3D[0];
  size4D[1] = size3D[1];
  size4D[2] = size3D[2] / bVecCount;
  size4D[3] = bVecCount;

  DWIVolumeType::DirectionType direction4D;
  for(unsigned i = 0; i < 3; ++i)
    for(unsigned j = 0; j < 3; ++j)
      {
      direction4D[i][j] = direction3D[i][j];
      direction4D[3][j] = 0.0;
      direction4D[j][3] = 0.0;
      }
  direction4D[3][3] = 1.0;

  DWIVolumeType::SpacingType spacing4D;
  DWIVolumeType::PointType origin4D;
  for(unsigned i = 0; i < 3; ++i)
    {
    spacing4D[i] = spacing3D[i];
    origin4D[i] = origin3D[i];
    }
  spacing4D[3] = 1.0;
  origin4D[3] = 0.0;


  DWIVolumeType::Pointer nrrdFile = DWIVolumeType::New();
  nrrdFile->SetRegions(size4D);
  nrrdFile->SetDirection(direction4D);
  nrrdFile->SetSpacing(spacing4D);
  nrrdFile->SetOrigin(origin4D);

  nrrdFile->Allocate();
  memcpy(nrrdFile->GetBufferPointer(),inputVol->GetBufferPointer(),
         inputVol->GetNumberOfPixels() * sizeof(PixelValueType));

  return EXIT_SUCCESS;
}

