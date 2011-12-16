#include "gdcmPrivateTag.h"
#include "gdcmReader.h"
#include "gdcmDirectory.h"
#include "gdcmCSAHeader.h"
#include "gdcmPDBHeader.h"
#include "gdcmPrinter.h"
//#include "eig3.h"
#include "vnl/vnl_vector_fixed.h"
#include "vnl/vnl_matrix_fixed.h"
#include "vnl/algo/vnl_svd.h"

/**
GEMS
0x0043,0x0039,"GEMS_PARM_01"
0x0019,0x00bb,"GEMS_ACQU_01"
0x0019,0x00bc,"GEMS_ACQU_01"
0x0019,0x00bd,"GEMS_ACQU_01"


BVALUE can be found in the PDB header from GEMS
*/
bool GetFromGEMS(const gdcm::File & file)
{
  gdcm::PDBHeader pdb;
  const gdcm::DataSet& ds = file.GetDataSet();

  const gdcm::PrivateTag &t1 = pdb.GetPDBInfoTag();
  bool found = false;
  int ret = 0;
  if( ds.FindDataElement( t1 ) )
    {
    pdb.LoadFromDataElement( ds.GetDataElement( t1 ) );
    //pdb.Print( std::cout );
    found = true;
    }
  if( !found ) return false;

  // gdcmdump --pdb DWI_GEMS.dcm
  // ...
  // BVALUE "1000"
  int bvalue = 0;
  if( pdb.FindPDBElementByName( "BVALUE" ) )
    {
    const gdcm::PDBElement &pdbel = pdb.GetPDBElementByName( "BVALUE" );
    const char *value = pdbel.GetValue();
    bvalue = atoi( value );
    std::cout << "BVALUE: " << bvalue << std::endl;
    }

  return true;
}

/*
{0x0019,0x000c,"SIEMENS MR HEADER",VR::IS,VM::VM1,"B_value",false },
{0x0019,0x000e,"SIEMENS MR HEADER",VR::FD,VM::VM3,"DiffusionGradientDirection",false },
{0x0019,0x0027,"SIEMENS MR HEADER",VR::FD,VM::VM6,"B_matrix",false },
*/
bool GetFromSIEMENS(gdcm::File const &file)
{
  gdcm::CSAHeader csa;
  const gdcm::DataSet& ds = file.GetDataSet();

  const gdcm::PrivateTag &t1 = csa.GetCSAImageHeaderInfoTag();
  //const gdcm::PrivateTag &t2 = csa.GetCSASeriesHeaderInfoTag();
  //const gdcm::PrivateTag &t3 = csa.GetCSADataInfo();

  bool found = false;
  int ret = 0;
  if( ds.FindDataElement( t1 ) )
    {
    csa.LoadFromDataElement( ds.GetDataElement( t1 ) );
    //csa.Print( std::cout );
    found = true;
    if( csa.GetFormat() == gdcm::CSAHeader::ZEROED_OUT )
      {
      std::cout << "CSA Header has been zero-out (contains only 0)" << std::endl;
      ret = 1;
      }
    else if( csa.GetFormat() == gdcm::CSAHeader::DATASET_FORMAT )
      {
      gdcm::Printer p;
      gdcm::File f;
      f.SetDataSet( csa.GetDataSet() );
      p.SetFile( f );
//      p.Print( std::cout );
      }
    }
  if( !found ) return false;


  // 6 - 'B_value' VM 1, VR IS, SyngoDT 6, NoOfItems 6, Data '1000    '
  int bvalue = 0;
  if( csa.FindCSAElementByName( "B_value" ) )
    {
    const gdcm::CSAElement &csael = csa.GetCSAElementByName( "B_value" );
    //std::cout << csael << std::endl;
    const gdcm::ByteValue *bv = csael.GetByteValue();
    gdcm::Element<gdcm::VR::IS, gdcm::VM::VM1> el;
    el.Set( csael.GetValue() );
    bvalue = el.GetValue();
    //std::cout << "B_value:" << el.GetValue() << std::endl;
    }

  // 77 - 'B_matrix' VM 6, VR FD, SyngoDT 4, NoOfItems 6, Data '952.00000000'\'-172.00000000'\'95.00000000'\'32.00000000'\'-17.00000000'\'10.00000000'
  const double *bmatrix = 0;
  if( csa.FindCSAElementByName( "B_matrix" ) )
    {
    const gdcm::CSAElement &csael = csa.GetCSAElementByName( "B_matrix" );
    //std::cout << csael << std::endl;
    const gdcm::ByteValue *bv = csael.GetByteValue();
    // BUG: FD is not really FD as it is in ASCII, lets use VR::DS instead:
    gdcm::Element<gdcm::VR::DS, gdcm::VM::VM6> el;
    el.Set( csael.GetValue() );
    bmatrix = el.GetValues();
    //std::cout << "B_value:" << el.GetValue() << std::endl;
    for( int i = 0; i < 6; ++i )
      {
      std::cout << "B_matrix: " << i << " " << bmatrix[i] << std::endl;
      }
    }
  // 21 - 'DiffusionGradientDirection' VM 3, VR FD, SyngoDT 4, NoOfItems 0, Data
  std::vector<double> diffusiongradientdirection;
  diffusiongradientdirection.resize( 3 );
  if( csa.FindCSAElementByName( "DiffusionGradientDirection" ) )
    {
    const gdcm::CSAElement &csael = csa.GetCSAElementByName( "DiffusionGradientDirection" );
    gdcm::Element<gdcm::VR::DS, gdcm::VM::VM3> el;
    el.Set( csael.GetValue() );
    const double *v = el.GetValues();
    diffusiongradientdirection = std::vector<double>(v, v+3);
    }

  // 24 - 'DiffusionDirectionality' VM 1, VR CS, SyngoDT 16, NoOfItems 6, Data 'DIRECTIONAL'
  std::string diffusiondirectionality;
  if( csa.FindCSAElementByName( "DiffusionDirectionality" ) )
    {
    const gdcm::CSAElement &csael = csa.GetCSAElementByName( "DiffusionDirectionality" );
    gdcm::Element<gdcm::VR::CS, gdcm::VM::VM1> el;
    el.Set( csael.GetValue() );
    diffusiondirectionality = el.GetValue();
    }

  if( !bmatrix ) return false;

  double A[3][3];
  vnl_matrix_fixed<double, 3, 3> bMatrix;
  bMatrix[0][0] = bmatrix[0];
  bMatrix[0][1] = bmatrix[1];
  bMatrix[0][2] = bmatrix[2];
  bMatrix[1][1] = bmatrix[3];
  bMatrix[1][2] = bmatrix[4];
  bMatrix[2][2] = bmatrix[5];
  bMatrix[1][0] = bMatrix[0][1];
  bMatrix[2][0] = bMatrix[0][2];
  bMatrix[2][1] = bMatrix[1][2];

  A[0][0] = bmatrix[0];
  A[1][0] = bmatrix[1];
  A[2][0] = bmatrix[2];
  A[1][1] = bmatrix[3];
  A[2][1] = bmatrix[4];
  A[2][2] = bmatrix[5];
  A[0][1] = bmatrix[1];
  A[0][2] = bmatrix[2];
  A[1][2] = bmatrix[4];
//  for(int i=0;i<3;++i)
//    {
//    for(int j=0;j<3;++j)
//      std::cout << A[i][j] << ",\t";
//    std::cout << std::endl;
//    }

  double V[3][3];
  double dd[3];

//  eigen_decomposition(A, V, dd);
  vnl_svd<double> svd(bMatrix);

  // UNC comments: Extracting the principal eigenvector i.e. the gradient direction
  vnl_vector_fixed<double, 3> vect3d;
  vect3d.fill( 0 );
  vect3d[0] = svd.U(0,0);
  vect3d[1] = svd.U(1,0);
  vect3d[2] = svd.U(2,0);

  std::cout << "BMatrix: " << std::endl;
  std::cout << bMatrix[0][0] << std::endl;
  std::cout << bMatrix[0][1] << "\t" << bMatrix[1][1] << std::endl;
  std::cout << bMatrix[0][2] << "\t" << bMatrix[1][2] << "\t" << bMatrix[2][2] << std::endl;

  // UNC comments: The b-value si the trace of the bmatrix
  bvalue = bMatrix[0][0] + bMatrix[1][1] + bMatrix[2][2];
  std::cout << bvalue << std::endl;
  std::cout << "Gradient coordinates: " << vect3d[0] << " " << vect3d[1] << " " << vect3d[2] << std::endl;

  // Let's compare the computed value to what is actually stored in the siemens private header:
  std::cout << "DiffusionGradientDirection: " << diffusiongradientdirection[0] << " " << diffusiongradientdirection[1] << " " << diffusiongradientdirection[2] << std::endl;


//  for(int i=0;i<3;++i)
//    {
//    for(int j=0;j<3;++j)
//      std::cout << V[i][j] << ",\t";
//    std::cout << std::endl;
//    }

//  std::cout << dd[0] << std::endl;
//  std::cout << dd[1] << std::endl;
//  std::cout << dd[2] << std::endl;

//  assert( dd[2] > dd[1] && dd[2] > dd[0] );

//  double gradient[3];
//  gradient[0] = V[2][0];
//  gradient[1] = V[2][1];
//  gradient[2] = V[2][2];
//
//  std::cout << "gradient: " << gradient[0] << "," << gradient[1] << "," << gradient[2] << std::endl;

  return true;
}

/*
{0x2001,0x0003,"Philips Imaging DD 001",VR::FL,VM::VM1,"Diffusion B-Factor",false },
{0x2001,0x0004,"Philips Imaging DD 001",VR::CS,VM::VM1,"Diffusion Direction",false },
{0x2005,0x00b0,"Philips MR Imaging DD 001",VR::FL,VM::VM1,"Diffusion Direction RL",false },
{0x2005,0x00b1,"Philips MR Imaging DD 001",VR::FL,VM::VM1,"Diffusion Direction AP",false },
{0x2005,0x00b2,"Philips MR Imaging DD 001",VR::FL,VM::VM1,"Diffusion Direction FH",false },
*/
bool GetFromPMS(const gdcm::File & file)
{
  using namespace gdcm;
  const DataSet& ds = file.GetDataSet();

  const PrivateTag tdiffusionbfactor(0x2001,0x03,"Philips Imaging DD 001");
  if( !ds.FindDataElement( tdiffusionbfactor ) ) return 1;
  const DataElement& diffusionbfactor = ds.GetDataElement( tdiffusionbfactor );
  const PrivateTag tdiffusiondirection(0x2001,0x04,"Philips Imaging DD 001");
  if( !ds.FindDataElement( tdiffusiondirection ) ) return 1;
  const DataElement& diffusiondirection = ds.GetDataElement( tdiffusiondirection );
  const PrivateTag tdiffusiondirectionRL(0x2005,0xb0,"Philips MR Imaging DD 001");
  if( !ds.FindDataElement( tdiffusiondirectionRL ) ) return 1;
  const DataElement& diffusiondirectionRL = ds.GetDataElement( tdiffusiondirectionRL );
  const PrivateTag tdiffusiondirectionAP(0x2005,0xb1,"Philips MR Imaging DD 001");
  if( !ds.FindDataElement( tdiffusiondirectionAP ) ) return 1;
  const DataElement& diffusiondirectionAP = ds.GetDataElement( tdiffusiondirectionAP );
  const PrivateTag tdiffusiondirectionFH(0x2005,0xb2,"Philips MR Imaging DD 001");
  if( !ds.FindDataElement( tdiffusiondirectionFH ) ) return 1;
  const DataElement& diffusiondirectionFH = ds.GetDataElement( tdiffusiondirectionFH );

  //std::cout << diffusionbfactor << std::endl;
  Element<VR::FL,VM::VM1> ddbf;
  ddbf.SetFromDataElement( diffusionbfactor );
  std::cout << ddbf.GetValue() << std::endl;
  std::cout << diffusiondirection << std::endl;
  //std::cout << diffusiondirectionRL << std::endl;
  Element<VR::FL,VM::VM1> ddrl;
  ddrl.SetFromDataElement( diffusiondirectionRL );
  std::cout << ddrl.GetValue() << std::endl;
  //std::cout << diffusiondirectionAP << std::endl;
  Element<VR::FL,VM::VM1> ddap;
  ddap.SetFromDataElement( diffusiondirectionAP );
  std::cout << ddap.GetValue() << std::endl;
  //std::cout << diffusiondirectionFH << std::endl;
  Element<VR::FL,VM::VM1> ddfh;
  ddfh.SetFromDataElement( diffusiondirectionFH );
  std::cout << ddfh.GetValue() << std::endl;

  return true;
}

int main(int argc, char *argv[])
{
  using namespace gdcm;
  const char *directory = argv[1];

  Directory d;
  unsigned int nfiles = d.Load( directory );
  Directory::FilenamesType const &filenames = d.GetFilenames();
  Directory::FilenamesType::const_iterator it = filenames.begin();

  const char *filename = it->c_str();
  std::cout << "Process: " << filename << std::endl;

  gdcm::Reader reader;
  reader.SetFileName( filename );
  if( !reader.Read() )
    {
    return 1;
    }

  bool b1 = GetFromGEMS( reader.GetFile() );
  bool b2 = GetFromSIEMENS( reader.GetFile() );
  bool b3 = GetFromPMS( reader.GetFile() );

  return 0;
}
