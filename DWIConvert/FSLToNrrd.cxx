#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <sstream>
#include "DWIConvertUtils.h"

#include "itkMetaDataObject.h"

typedef short PixelValueType;
typedef itk::Image< PixelValueType, 4 > VolumeType;
typedef itk::VectorImage<PixelValueType, 3> VectorVolumeType;

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
FSLToNrrd(const std::string &inputVolume,
          const std::string &outputVolume,
          const std::string &inputBValues,
          const std::string &inputBVectors)
{
  if(CheckArg<std::string>("Input Volume",inputVolume,"") == EXIT_FAILURE ||
     CheckArg<std::string>("Output Volume",outputVolume,"") == EXIT_FAILURE ||
     CheckArg<std::string>("B Values", inputBValues, "") == EXIT_FAILURE ||
     CheckArg<std::string>("B Vectors", inputBVectors, ""))
    {
    return EXIT_FAILURE;
    }

  VolumeType::Pointer inputVol;
  if(ReadVolume<VolumeType>(inputVol,inputVolume) != EXIT_SUCCESS)
    {
    return EXIT_FAILURE;
    }
  std::vector<double> BVals;
  std::vector< std::vector<double> > BVecs;
  int bValCount, bVecCount;
  double maxBValue(0.0);
  if(ReadBVals(BVals,bValCount,inputBValues,maxBValue) != EXIT_SUCCESS)
    {
    return EXIT_FAILURE;
    }
  if(ReadBVecs(BVecs,bVecCount,inputBVectors) != EXIT_SUCCESS)
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

  VolumeType::SizeType inputSize =
    inputVol->GetLargestPossibleRegion().GetSize();

  unsigned volumeCount = inputSize[3];
  if(volumeCount != bValCount)
    {
    std::cerr << "Mismatch between BVector count ("
              << bVecCount << ") and image volume count ("
              << volumeCount << ")" << std::endl;
    return EXIT_SUCCESS;
    }

  //
  // convert from image series to vector voxels
  VolumeType::SpacingType inputSpacing = inputVol->GetSpacing();
  VolumeType::PointType inputOrigin = inputVol->GetOrigin();
  VolumeType::DirectionType inputDirection = inputVol->GetDirection();

#if 0
  VectorVolumeType::Pointer vectorVolume =
    VectorVolumeType::New();
  VectorVolumeType::SizeType vectorVolSize;
  VectorVolumeType::SpacingType vectorVolSpacing;
  VectorVolumeType::PointType vectorVolOrigin;
  VectorVolumeType::DirectionType vectorVolDirection;

  for(unsigned int i = 0; i < 3; ++i)
    {
    vectorVolSize[i] = inputSize[i];
    vectorVolSpacing[i] = inputSpacing[i];
    vectorVolOrigin[i] = inputOrigin[i];
    for(unsigned int j = 0; j < 3; ++j)
      {
      vectorVolDirection[i][j] = inputDirection[i][j];
      }
    }
  vectorVolume->SetRegions(vectorVolSize);
  vectorVolume->SetOrigin(vectorVolOrigin);
  vectorVolume->SetSpacing(vectorVolSpacing);
  vectorVolume->SetDirection(vectorVolDirection);
  vectorVolume->SetVectorLength(inputSize[3]);
  vectorVolume->Allocate();

  VectorVolumeType::IndexType vectorIndex;
  VolumeType::IndexType volumeIndex;
  for(unsigned int i = 0; i < inputSpacing[3]; ++i)
    {
    volumeIndex[3] = i;
    for(unsigned int j = 0; j < inputSpacing[2]; ++j)
      {
      volumeIndex[2] = j;
      vectorIndex[2] = j;
      for(unsigned int k = 0; k < inputSpacing[1]; ++k)
        {
        volumeIndex[1] = k;
        vectorIndex[1] = k;
        for(unsigned int m = 0; m < inputSpacing[0]; ++m)
          {
          volumeIndex[0] = m;
          vectorIndex[0] = m;
          vectorVolume->GetPixel(vectorIndex)[i] =
            inputVol->GetPixel(volumeIndex);
          }
        }
      }
    }
  itk::MetaDataDictionary &dict = vectorVolume->GetMetaDataDictionary();
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
  return WriteVolume(vectorVolume,outputVolume);
#else
  std::ofstream header;
  //std::string headerFileName = outputDir + "/" + outputFileName;

  header.open (outputVolume.c_str(), std::ios::out | std::ios::binary);
  header << "NRRD0005" << std::endl;
  header << "type: short" << std::endl;
  header << "dimension: 4" << std::endl;

  // need to check
  header << "space: left-posterior-superior" << std::endl;
  // in nrrd, size array is the number of pixels in 1st, 2nd, 3rd, ... dimensions
  header << "sizes: " << inputSize[0] << " "
         << inputSize[1] << " "
         << inputSize[2] << " "
         << inputSize[3] << std::endl;
  header << "thicknesses:  NaN  NaN " << inputSpacing[2] << " NaN" << std::endl;

  // need to check
  header << "space directions: "
         << "(" << (inputDirection[0][0]) << ","<< (inputDirection[1][0]) << ","<< (inputDirection[2][0]) << ") "
         << "(" << (inputDirection[0][1]) << ","<< (inputDirection[1][1]) << ","<< (inputDirection[2][1]) << ") "
         << "(" << (inputDirection[0][2]) << ","<< (inputDirection[1][2]) << ","<< (inputDirection[2][2])
         << ") none" << std::endl;
  header << "centerings: cell cell cell ???" << std::endl;
  header << "kinds: space space space list" << std::endl;

  header << "endian: little" << std::endl;
  header << "encoding: raw" << std::endl;
  header << "space units: \"mm\" \"mm\" \"mm\"" << std::endl;
  header << "space origin: "
         <<"(" << inputOrigin[0]
         << ","<< inputOrigin[1]
         << ","<< inputOrigin[2] << ") " << std::endl;
  header << "measurement frame: "
         << "(" << 1 << ","<< 0 << ","<< 0 << ") "
         << "(" << 0 << ","<< 1 << ","<< 0 << ") "
         << "(" << 0 << ","<< 0 << ","<< 1 << ")"
         << std::endl;

  header << "modality:=DWMRI" << std::endl;
  // this is the norminal BValue, i.e. the largest one.
  header << "DWMRI_b-value:=" << maxBValue << std::endl;
  for(unsigned int i = 0; i < bVecCount; ++i)
    {
    header << "DWMRI_gradient_" << std::setw(4) << std::setfill('0')
           << i << ":="
           << BVecs[i][0] << "   "
           << BVecs[i][1] << "   "
           << BVecs[i][2] 
           << std::endl;
    }

  // write data in the same file is .nrrd was chosen
  header << std::endl;;
  unsigned long nVoxels = inputVol->GetLargestPossibleRegion().GetNumberOfPixels();
  header.write( reinterpret_cast<char *>(inputVol->GetBufferPointer()),
                nVoxels*sizeof(short) );
  header.close();
  return EXIT_SUCCESS;
#endif
}

