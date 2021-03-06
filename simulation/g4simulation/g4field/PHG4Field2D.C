//
//    *************************************
//    *                                   *
//    *            PHG4Field2D.C          *
//    *                                   *
//    *************************************
//
// ********************************************************************************
// GEANT Field Map, for use in converting ROOT maps of PHENIX magnetic field
// writting by Michael Stone, July 2011

#include "PHG4Field2D.h"

#include <Geant4/G4SystemOfUnits.hh>

#include <set>

using namespace std;

PHG4Field2D::PHG4Field2D( const string &filename, const int verb ) :
  verb_(verb)
{
  r_index0_cache = 0;
  r_index1_cache = 0;
  z_index0_cache = 0;
  z_index1_cache = 0;

  if (verb_ >= 0)
    cout << " ------------- PHG4Field2D::PHG4Field2D() ------------------" << endl;

  // open file
  TFile *rootinput = TFile::Open(filename.c_str());
  if (!rootinput)
    {
      G4cout << " could not open " << filename << " exiting now" << endl;
      exit(1);
    }
  if (verb_ >= 0) G4cout << "  Field grid file: " << filename << endl;
  rootinput->cd();

  Float_t ROOT_Z,  ROOT_R;
  Float_t ROOT_BZ, ROOT_BR;
  //  get root NTuple objects
  TNtuple *field_map = (TNtuple*)gDirectory->Get("fieldmap");
  if (field_map)
    {
      magfield_unit = tesla;
    }
  else
    {
      field_map = (TNtuple*)gDirectory->Get("map");
      if (! field_map)
        {
          cout << "PHG4Field2D: could not locate ntuple of name map or fieldmap, exiting now" << endl;
          exit(1);
        }
      magfield_unit = gauss;
    }
  field_map->SetBranchAddress("z",    &ROOT_Z);
  field_map->SetBranchAddress("r",    &ROOT_R);
  field_map->SetBranchAddress("bz",   &ROOT_BZ);
  field_map->SetBranchAddress("br",   &ROOT_BR);

  // get the number of entries in the tree
  int nz, nr;
  nz = field_map->GetEntries("z>-1e6");
  nr = field_map->GetEntries("r>-1e6");
  static const int NENTRIES = field_map->GetEntries();

  // run checks on entries
  if (verb_ >= 0) G4cout << "  The field grid contained " << NENTRIES << " entries" << endl;
  if ( verb_ > 0 )
    {
      G4cout << "\n  NENTRIES should be the same as the following values:"
	     << "\n  [ Number of values r,z: "
	     << nr << " " << nz << " ]! " << endl;
    }

  if (nz != nr)
    {
      G4cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	     << "\n The file you entered is not a \"table\" of values"
	     << "\n Something very likely went oh so wrong"
	     << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
    }

  // Keep track of the unique z, r, phi values in the grid using sets
  std::set<float> z_set, r_set;

  // Sort the entries to get rid of any stupid ordering problems...

  // We copy the TNtuple into a std::map (which is always sorted)
  // using a 3-tuple of (z, r, phi) so it is sorted in z, then r, then
  // phi.
  if ( verb_ > 0 )
    {
      G4cout << "  --> Sorting Entries..." << endl;
    }
  std::map<trio, trio> sorted_map;
  for ( int i = 0; i < field_map->GetEntries(); i++)
    {
      field_map->GetEntry(i);
      trio coord_key( ROOT_Z, ROOT_R );
      trio field_val( ROOT_BZ, ROOT_BR );
      sorted_map[coord_key] = field_val;

      z_set.insert(ROOT_Z*cm);
      r_set.insert(ROOT_R*cm);
    }

  // couts for assurance
  if (verb_ > 4)
    {
      map<trio, trio>::iterator it = sorted_map.begin();
      print_map(it);
      float last_z = it->first.get<0>();
      for ( it = sorted_map.begin(); it != sorted_map.end(); ++it )
        {
          if ( it->first.get<0>() != last_z )
            {
              last_z = it->first.get<0>();
              print_map( it );
            }
        }
    }

  if (verb_ > 0)
    {
      G4cout << "  --> Putting entries into containers... " <<  endl;
    }

  // grab the minimum and maximum z values
  minz_ = *(z_set.begin());
  std::set<float>::iterator ziter = z_set.end();
  --ziter;
  maxz_ = *ziter;

  // initialize maps
  nz = z_set.size();
  nr = r_set.size();
  r_map_.resize( nr, 0 );
  z_map_.resize( nz, 0 );

  std::copy(z_set.begin(), z_set.end(), z_map_.begin());
  std::copy(r_set.begin(), r_set.end(), r_map_.begin());

  // initialize the field map vectors to the correct sizes
  BFieldR_.resize( nz, vector<float>(nr, 0) );
  BFieldZ_.resize( nz, vector<float>(nr, 0) );

  // all of this assumes that  z_prev < z , i.e. the table is ordered (as of right now)
  unsigned int ir = 0, iz = 0;  // useful indexes to keep track of
  map<trio, trio>::iterator iter = sorted_map.begin();
  for ( ; iter != sorted_map.end(); ++iter )
    {

      // equivalent to ->GetEntry(iter)
      float z =    iter-> first.get<0>() * cm;
      float r =    iter-> first.get<1>() * cm;
      float Bz =   iter->second.get<0>() * magfield_unit;
      float Br =   iter->second.get<1>() * magfield_unit;

      if ( z > maxz_ ) maxz_ = z;
      if ( z < minz_ ) minz_ = z;

      // check for change in z value, when z changes we have a ton of updates to do
      if ( z != z_map_[iz] )
        {
          ++iz;
          ir = 0;

        }
      else if ( r != r_map_[ir] ) // check for change in r value
        {
          ++ir;
        }

      // shouldn't happen
      if ( iz > 0 && z < z_map_[iz-1] )
        {
          G4cout << "!!!!!!!!! Your map isn't ordered.... z: " << z << " zprev: " << z_map_[iz-1] << endl;
        }

      BFieldR_[iz][ir] = Br;
      BFieldZ_[iz][ir] = Bz;

      // you can change this to check table values for correctness
      // print_map prints the values in the root table, and the
      // couts print the values entered into the vectors
      if ( fabs(z) < 10 && ir < 10 /*&& iphi==2*/ && verb_ > 3)
        {
          print_map(iter);

          G4cout << " B("
		 << r_map_[ir] << ", "
		 << z_map_[iz] << "):  ("
		 << BFieldR_[iz][ir] << ", "
		 << BFieldZ_[iz][ir] << ")" << endl;
        }

    } // end loop over root field map file

  if (verb_ >= 0) G4cout << "  Mag field z boundaries (min,max): (" << minz_ / cm << ", " << maxz_ / cm << ") cm" << endl;
  if (verb_ >= 0) G4cout << "  Mag field r max boundary: " << r_map_.back()/ cm << " cm" << endl;
    
  if (verb_ >= 0)
    cout << " -----------------------------------------------------------" << endl;
}

void PHG4Field2D::GetFieldValue(const double point[4], double *Bfield ) const
{
  if (verb_ > 2)
    G4cout << "\nPHG4Field2D::GetFieldValue" << endl;
  double x = point[0];
  double y = point[1];
  double z = point[2];
  double r = sqrt(x * x + y * y);
  double phi;
  phi = atan2(y, x);
  if ( phi < 0 ) phi += 2 * M_PI;  // normalize phi to be over the range [0,2*pi]

  // Check that the point is within the defined z region (check r in a second)
  if ( (z >= minz_) && (z <= maxz_) )
    {

      double BFieldCyl[3];
      double cylpoint[4] = { z, r, phi, 0 };

      // take <z,r,phi> location and return a vector of <Bz, Br, Bphi>
      GetFieldCyl( cylpoint, BFieldCyl );

      // X direction of B-field ( Bx = Br*cos(phi) - Bphi*sin(phi)
      Bfield[0] = cos(phi) * BFieldCyl[1] - sin(phi) * BFieldCyl[2]; // unit vector transformations

      // Y direction of B-field ( By = Br*sin(phi) + Bphi*cos(phi)
      Bfield[1] = sin(phi) * BFieldCyl[1] + cos(phi) * BFieldCyl[2];

      // Z direction of B-field
      Bfield[2] = BFieldCyl[0];

    }
  else     // x,y,z is outside of z range of the field map
    {

      Bfield[0] = 0.0;
      Bfield[1] = 0.0;
      Bfield[2] = 0.0;
      if ( verb_ > 2 )
        G4cout << "!!!!!!!!!! Field point not in defined region (outside of z bounds)" << endl;
    }

  if (verb_ > 2)
    {
      G4cout << "END PHG4Field2D::GetFieldValue\n"
	     << "  --->  {Bx, By, Bz} : "
	     << "< " << Bfield[0] << ", " << Bfield[1] << ", " << Bfield[2] << " >" << endl;
    }

  return;
}

void PHG4Field2D::GetFieldCyl( const double CylPoint[4], double *BfieldCyl ) const
{
  float z = CylPoint[0];
  float r = CylPoint[1];

  BfieldCyl[0] = 0.0;
  BfieldCyl[1] = 0.0;
  BfieldCyl[2] = 0.0;

  if ( verb_ > 2 )
    G4cout << "GetFieldCyl@ <z,r>: {" << z << "," << r << "}" << endl;

  if (z < z_map_[0] || z > z_map_[z_map_.size()-1])
    {
      if ( verb_ > 2 )
        G4cout << "!!!! Point not in defined region (radius too large in specific z-plane)" << endl;
      return;
    }

  // since GEANT4 looks up the field ~95% of the time in the same voxel
  // between subsequent calls, we can save on the expense of the upper_bound
  // lookup (~10-15% of central event run time) with some caching between calls
  
  unsigned int r_index0 = r_index0_cache;
  unsigned int r_index1 = r_index1_cache;
  
  if (!((r > r_map_[r_index0])&&(r < r_map_[r_index1]))) {
    
    // if miss cached r values, search through the lookup table
    vector<float>::const_iterator riter = upper_bound(r_map_.begin(), r_map_.end(), r);
    r_index0 = distance(r_map_.begin(), riter) - 1;
    if (r_index0 >= r_map_.size()) {
      if ( verb_ > 2 )
	G4cout << "!!!! Point not in defined region (radius too large in specific z-plane)" << endl;
      return;
    }
    
    r_index1 = r_index0 + 1;
    if (r_index1 >= r_map_.size()) {
      if ( verb_ > 2 )
	G4cout << "!!!! Point not in defined region (radius too large in specific z-plane)" << endl;
      return;
    }

    // update cache
    r_index0_cache = r_index0;
    r_index1_cache = r_index1;
  }

  unsigned int z_index0 = z_index0_cache;
  unsigned int z_index1 = z_index1_cache;

  if (!((z > z_map_[z_index0])&&(z < z_map_[z_index1]))) {

    // if miss cached z values, search through the lookup table
    vector<float>::const_iterator ziter = upper_bound(z_map_.begin(), z_map_.end(), z);
    z_index0 = distance(z_map_.begin(), ziter) - 1;
    z_index1 = z_index0 + 1;
    if (z_index1 >= z_map_.size()) {
      if ( verb_ > 2 )
	G4cout << "!!!! Point not in defined region (z too large in specific r-plane)" << endl;
      return;
    }

    // update cache
    z_index0_cache = z_index0;
    z_index1_cache = z_index1;
  }
    
  double Br000 = BFieldR_[z_index0][r_index0];
  double Br010 = BFieldR_[z_index0][r_index1];
  double Br100 = BFieldR_[z_index1][r_index0];
  double Br110 = BFieldR_[z_index1][r_index1];

  double Bz000 = BFieldZ_[z_index0][r_index0];
  double Bz100 = BFieldZ_[z_index1][r_index0];
  double Bz010 = BFieldZ_[z_index0][r_index1];
  double Bz110 = BFieldZ_[z_index1][r_index1];

  double zweight = z - z_map_[z_index0];
  double zspacing = z_map_[z_index1] - z_map_[z_index0];
  zweight /= zspacing;

  double rweight = r - r_map_[r_index0];
  double rspacing = r_map_[r_index1] - r_map_[r_index0];
  rweight /= rspacing;

  // Z direction of B-field
  BfieldCyl[0] =
    (1 - zweight) * ((1 - rweight) * Bz000 +
                     rweight * Bz010) +
    zweight * ((1 - rweight) * Bz100 +
               rweight * Bz110);

  // R direction of B-field
  BfieldCyl[1] =
    (1 - zweight) * ((1 - rweight) * Br000 +
                     rweight * Br010) +
    zweight * ((1 - rweight) * Br100 +
               rweight * Br110);

  // PHI Direction of B-field
  BfieldCyl[2] = 0;

  if ( verb_ > 2 )
    {
      G4cout << "End GFCyl Call: <bz,br,bphi> : {"
	     << BfieldCyl[0] / gauss << "," << BfieldCyl[1] / gauss << "," << BfieldCyl[2] / gauss << "}"
	     << endl;
    }

  return;
}

// debug function to print key/value pairs in map
void PHG4Field2D::print_map( map<trio, trio>::iterator& it ) const
{

  cout << "    Key: <"
       << it->first.get<0>() / cm << ","
       << it->first.get<1>() / cm << ">"

       << " Value: <"
       << it->second.get<0>() / magfield_unit << ","
       << it->second.get<1>() / magfield_unit << ">\n";

}

//==============================================================
// Wrapper Functions, for debugging
//==============================================================

PHG4Field2DWrapper::PHG4Field2DWrapper(const string &filename, const int verb) :
  field(filename, verb)
{}

double PHG4Field2DWrapper::GetBx(double x, double y, double z)
{
  double pos[4] = {x*cm, y*cm, z*cm, 0};
  double Bfield[6] = {0};
  field.GetFieldValue(pos, Bfield);
  return Bfield[0] / tesla;
}

double PHG4Field2DWrapper::GetBy(double x, double y, double z)
{
  double pos[4] = {x*cm, y*cm, z*cm, 0};
  double Bfield[6] = {0};
  field.GetFieldValue(pos, Bfield);
  return Bfield[1] / tesla;
}

double PHG4Field2DWrapper::GetBz(double x, double y, double z)
{
  double pos[4] = {x*cm, y*cm, z*cm, 0};
  double Bfield[6] = {0};
  field.GetFieldValue(pos, Bfield);
  return Bfield[2] / tesla;
}

double PHG4Field2DWrapper::GetBCylZ(double z, double r, double phi)
{
  double pos[4] = {z*cm, r*cm, phi*rad, 0};
  double Bfield[6] = {0};
  field.GetFieldCyl(pos, Bfield);
  return Bfield[0] / tesla;
}

double PHG4Field2DWrapper::GetBCylR(double z, double r, double phi)
{
  double pos[4] = {z*cm, r*cm, phi*rad, 0};
  double Bfield[6] = {0};
  field.GetFieldCyl(pos, Bfield);
  return Bfield[1] / tesla;
}

double PHG4Field2DWrapper::GetBCylPHI(double z, double r, double phi)
{
  double pos[4] = {z*cm, r*cm, phi*rad, 0};
  double Bfield[6] = {0};
  field.GetFieldCyl(pos, Bfield);
  return Bfield[2] / tesla;
}

void
PHG4Field2DWrapper::Verbosity(const int i)
{
  field.Verbosity(i);
  return;
}
