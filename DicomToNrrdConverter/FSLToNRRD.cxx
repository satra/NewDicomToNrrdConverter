#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkMetaDataObject.h"
#include "FSLToNrrdCLP.h"

typedef short PixelValueType;
typedef itk::Image< PixelValueType, 4 > VolumeType;

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
int
WriteVolume( VolumeType::Pointer &img, const std::string &fname )
{
  itk::ImageFileWriter< VolumeType >::Pointer imgWriter =
    itk::ImageFileWriter< VolumeType >::New();

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
ReadBVals(std::vector<double> &bVals, int & bValCount, const std::string &bValFilename, double &maxBValue)
{
  std::ifstream bValFile(bValFilename.c_str(),std::ifstream::in);
  if(!bValFile.good())
    {
    std::cerr << "Failed to open " << bValFilename
              << std::endl;
    return EXIT_FAILURE;
    }
  bVals.clear();
  bValCount = 0;
  while(!bValFile.eof())
    {
    double x;
    bValFile >> x;
    if(bValFile.fail())
      {
      break;
      }
    if(x > maxBValue)
      {
      maxBValue = x;
      }
    bValCount++;
    bVals.push_back(x);
    }
  return EXIT_SUCCESS;
}

int
ReadBVecs(std::vector< std::vector<double> > &bVecs, int & bVecCount, const std::string &bVecFilename)
{
  std::ifstream bVecFile(bVecFilename.c_str(),std::ifstream::in);
  if(!bVecFile.good())
    {
    std::cerr << "Failed to open " << bVecFilename
              << std::endl;
    return EXIT_FAILURE;
    }
  bVecs.clear();
  bVecCount = 0;
  while(!bVecFile.eof())
    {
    std::vector<double> x;
    for(unsigned i = 0; i < 3; ++i)
      {
      double val;
      bVecFile >> val;
      if(bVecFile.fail())
        {
        break;
        }
      x.push_back(val);
      }
    if(bVecFile.fail())
      {
      break;
      }
    bVecCount++;
    bVecs.push_back(x);
    }
  return EXIT_SUCCESS;
}

int
main(int argc, char *argv[])
{
  PARSE_ARGS;
  if(CheckArg<std::string>("Input Volume",inputVolume,"") == EXIT_FAILURE ||
     CheckArg<std::string>("Output Volume",outputVolume,"") == EXIT_FAILURE ||
     CheckArg<std::string>("B Values", BValues, "") == EXIT_FAILURE ||
     CheckArg<std::string>("B Vectors", BVectors, ""))
    {
    return EXIT_FAILURE;
    }

  VolumeType::Pointer inputVol;
  if(ReadVolume(inputVol,inputVolume) != EXIT_SUCCESS)
    {
    return EXIT_FAILURE;
    }
  std::vector<double> BVals;
  std::vector< std::vector<double> > BVecs;
  int bValCount, bVecCount;
  double maxBValue(0.0);
  if(ReadBVals(BVals,bValCount,BValues,maxBValue) != EXIT_SUCCESS)
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
  unsigned volumeCount = inputVol->GetLargestPossibleRegion().GetSize()[3];
  if(volumeCount != bValCount)
    {
    std::cerr << "Mismatch between BVector count ("
              << bVecCount << ") and image volume count ("
              << volumeCount << ")" << std::endl;
    return EXIT_SUCCESS;
    }
  itk::MetaDataDictionary &dict = inputVol->GetMetaDataDictionary();
  std::string spaceVal("left-posterior-superior");
  itk::EncapsulateMetaData<std::string>(dict,"space",spaceVal);

  std::string modalityVal("DWMRI");
  itk::EncapsulateMetaData<std::string>(dict,"modality",modalityVal);

  std::string space_units("\"mm\" \"mm\" \"mm\"");
  itk::EncapsulateMetaData<std::string>(dict,"space_units",space_units);

  VolumeType::SpacingType spacing = inputVol->GetSpacing();
  std::stringstream thicknessSS;
  thicknessSS << spacing[0] << " " << spacing[1] << " "
              << spacing[2];
  itk::EncapsulateMetaData<std::string>(dict,"thicknesses",thicknessSS.str());
  for(unsigned int i = 0; i < bVecCount; ++i)
    {
    std::stringstream vec;
    vec << BVecs[i][0] << "  "
        << BVecs[i][1] << "  "
        << BVecs[i][2];
    std::stringstream label;
    label << "DWMRI_gradient_" << std::setw(4) << std::setfill('0') << i;
    itk::EncapsulateMetaData<std::string>(dict,label.str(),vec.str());
    }
  std::stringstream maxBValueSS;
  maxBValueSS << maxBValue;
  itk::EncapsulateMetaData<std::string>(dict,"DWMRI_b-value", maxBValueSS.str());
  return WriteVolume(inputVol,outputVolume);
}

