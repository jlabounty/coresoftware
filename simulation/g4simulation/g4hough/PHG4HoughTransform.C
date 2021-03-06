#include "PHG4HoughTransform.h"

// g4hough includes
#include "SvtxVertexMap.h"
#include "SvtxVertex.h"
#include "SvtxTrackMap.h"
#include "SvtxTrack.h"
#include "SvtxClusterMap.h"
#include "SvtxCluster.h"

// PHENIX Geant4 includes
#include <g4detectors/PHG4CylinderGeomContainer.h>
#include <g4detectors/PHG4CylinderGeom.h>
#include <g4detectors/PHG4CylinderCellGeom.h>
#include <g4detectors/PHG4CylinderCellGeomContainer.h>
#include <g4main/PHG4InEvent.h>
#include <g4main/PHG4VtxPoint.h>
#include <g4detectors/PHG4CylinderCellContainer.h>
#include <g4main/PHG4HitContainer.h>
#include <g4main/PHG4Hit.h>

// PHENIX includes
#include <fun4all/Fun4AllReturnCodes.h>
#include <phool/PHNodeIterator.h>
#include <phool/PHTypedNodeIterator.h>
#include <phool/PHCompositeNode.h>
#include <phool/PHIODataNode.h>
#include <fun4all/getClass.h>

// Geant4 includes
#include <Geant4/G4MagneticField.hh>
#include <Geant4/G4TransportationManager.hh>
#include <Geant4/G4FieldManager.hh>

// Helix Hough includes
#include <SimpleHit3D.h>
#include <SimpleTrack3D.h>
#include <HelixResolution.h>
#include <HelixRange.h>
#include <HelixHough.h>
#include <VertexFinder.h>

// ROOT includes
#include <TVector3.h>
#include <TH1D.h>

// standard includes
#include <cmath>
#include <iostream>
#include <float.h>

using findNode::getClass;
using namespace std;
using namespace Eigen;

static inline double sign(double x)
{
  return ((double)(x > 0.)) - ((double)(x < 0.));
}

void PHG4HoughTransform::projectToRadius(const SvtxTrack& track, double radius, vector<double>& intersection)
{
  float phi = track.phi;
  float d = track.d;
  float k = track.kappa;
  float z0 = track.z0;
  float dzdl = track.dzdl;
  
  intersection.clear();intersection.assign(3,0.);
  double& x = intersection[0];
  double& y = intersection[1];
  double& z = intersection[2];
  
  float rad_det = radius;
  
  float cosphi = cos(phi);
  float sinphi = sin(phi);
  
  // get outer hit
  float hitx = d*cosphi;
  float hity = d*sinphi;
  int nhits = track.getNhits();
  for(int l=(nhits-1);l>=0;l-=1)
  {
    if(track.getClusterID(l) >= 0)
    {
      hitx = track.getHitPosition(l, 0);
      hity = track.getHitPosition(l, 1);
      break;
    }
  }
  
  
  
  k = fabs(k);
  
  float kd = (d*k + 1.);
  float kcx = kd*cosphi;
  float kcy = kd*sinphi;
  float kd_inv = 1./kd;
  float R2 = rad_det*rad_det;
  float a = 0.5*(k*R2 + ( d*d*k + 2.*d ))*kd_inv;
  float tmp1 = a*kd_inv;
  float P2x = kcx*tmp1;
  float P2y = kcy*tmp1;
  
  float h = sqrt(R2 - a*a);
  
  float ux = -kcy*kd_inv;
  float uy = kcx*kd_inv;
  
  float x1 = P2x + ux*h;
  float y1 = P2y + uy*h;
  float x2 = P2x - ux*h;
  float y2 = P2y - uy*h;
  float diff1 = (x1-hitx)*(x1-hitx) + (y1-hity)*(y1-hity);
  float diff2 = (x2-hitx)*(x2-hitx) + (y2-hity)*(y2-hity);
  float signk = 0.;
  if(diff1 < diff2){signk = 1.;}
  else{signk = -1.;}
  x = P2x + signk*ux*h;
  y = P2y + signk*uy*h;
  
  double sign_dzdl = sign(dzdl);
  double startx = d*cosphi;
  double starty = d*sinphi;
  double D = sqrt((startx-x)*(startx-x) + (starty-y)*(starty-y));
  double v = 0.5*k*D;
  z = 0.;
  if(v > 0.1)
  {
    if(v >= 0.999999){v=0.999999;}
    double s = 2.*asin(v)/k;
    double dz = sqrt(s*s*dzdl*dzdl/(1. - dzdl*dzdl));
    z = z0 + sign_dzdl*dz;
  }
  else
  {
    double s = 0.;
    double temp1 = k*D*0.5;temp1*=temp1;
    double temp2 = D*0.5;
    s += 2.*temp2;
    temp2*=temp1;
    s += temp2/3.;
    temp2*=temp1;
    s += (3./20.)*temp2;
    temp2*=temp1;
    s += (5./56.)*temp2;
    double dz = sqrt(s*s*dzdl*dzdl/(1. - dzdl*dzdl));
    z = z0 + sign_dzdl*dz;
  }
}

float PHG4HoughTransform::kappaToPt(float kappa) {  
  return _pt_rescale * _magField / 333.6 / kappa;
}

float PHG4HoughTransform::ptToKappa(float pt) {  
  return _pt_rescale * _magField / 333.6 / pt;
}

PHG4HoughTransform::PHG4HoughTransform(unsigned int seed_layers, unsigned int req_seed, const string &name) :
  SubsysReco(name),
  _timer(PHTimeServer::get()->insert_new("PHG4HoughTransform")),
  _timer_initial_hough(PHTimeServer::get()->insert_new("PHG4HoughTransform::track finding")), 
  _min_pT(0.2), 
  _min_pT_init(0.2), 
  _seed_layers(seed_layers), 
  _req_seed(req_seed), 
  _reject_ghosts(true), 
  _remove_hits(true), 
  _use_cell_size(false),
  _max_cluster_error(3.0),
  _bin_scale(0.8), 
  _z_bin_scale(0.8), 
  _cut_on_dca(false), 
  _dca_cut(0.1),
  _dcaz_cut(0.2)
{
  verbosity = 0;
  _magField = 1.5; // Tesla
  _use_vertex = true;
  _chi2_cut_init = 4.0;
  _chi2_cut_fast_par0 = 16.0;
  _chi2_cut_fast_par1 = 0.0;
  _chi2_cut_fast_max = FLT_MAX;
  _chi2_cut_full = 4.0;
  _ca_chi2_cut = 4.0;
  _cos_angle_cut = 0.985;

  _beta = 1;
  _lambda = 1;

  _pt_rescale = 1.0;
  
  _vote_error_scale.assign(_seed_layers, 1.0);
  _fit_error_scale.assign(_seed_layers, 1.0/sqrt(12.));

  _layer_ilayer_map.clear();
}

int PHG4HoughTransform::Init(PHCompositeNode *topNode)
{
  return Fun4AllReturnCodes::EVENT_OK;
}


int PHG4HoughTransform::InitRun(PHCompositeNode *topNode)
{
  int code = CreateNodes(topNode);

  if (verbosity >= 0) {
    cout << "====================== PHG4HoughTransform::InitRun() ======================" << endl;
    cout << " CVS Version: $Id: PHG4HoughTransform.C,v 1.101 2015/04/21 23:47:09 pinkenbu Exp $" << endl;
    cout << " Magnetic field set to: " << _magField << " Tesla" << endl;
    cout << " Number of tracking layers: " << _nlayers << endl;
    for (int i=0; i<_nlayers; ++i) {
      cout << "   Tracking layer #" << i << " "
	   << "radius = " << _radii[i] << " cm, "
	   << "material = " << _material[i]
	   << endl;
      cout << "   Tracking layer #" << i << " "
	   << "vote error scale = " << _vote_error_scale[i] << ", "
	   << "fit error scale = " << _fit_error_scale[i]
	   << endl;
    }
    cout << " Required hits: " << _req_seed << endl;
    cout << " Minimum pT: " << _min_pT << endl;
    cout << " Fast fit chisq cut min(par0+par1/pt,max): min( "
	 << _chi2_cut_fast_par0 << " + " << _chi2_cut_fast_par1 << " / pt, "
	 << _chi2_cut_fast_max << " )" << endl;
    cout << " Maximum chisq (kalman fit): " << _chi2_cut_full << endl;
    cout << " Cell automaton chisq: " << _ca_chi2_cut << endl;
    cout << " Cos Angle Cut: " << _cos_angle_cut << endl;
    cout << " Ghost rejection: " << boolalpha << _reject_ghosts << noboolalpha << endl;
    cout << " Hit removal: " << boolalpha << _remove_hits << noboolalpha << endl;
    cout << " Use cell size in place of cluster sizes: " << boolalpha << _use_cell_size << noboolalpha << endl;
    if (!_use_cell_size) cout << " Max cluster size error = " << _max_cluster_error << endl;
    cout << " Maximum DCA: " << boolalpha << _cut_on_dca << noboolalpha << endl;
    if (_cut_on_dca) {
      cout << "   Maximum DCA cut: " << _dca_cut << endl;
    }
    cout << "   Maximum DCAZ cut: " << _dcaz_cut << endl;
    cout << " Phi bin scale: " << _bin_scale << endl;
    cout << " Z bin scale: " << _z_bin_scale << endl;
    cout << " Produce an initial vertex for tracking: " << boolalpha << _use_vertex << noboolalpha << endl;
    if (_use_vertex) {
      cout << "   Initial vertex minimum pT: " << _min_pT_init << endl;
      cout << "   Initial vertex maximum chisq: " << _chi2_cut_init << endl;
    }
    cout << " Momentum rescale factor: " << _pt_rescale << endl; 
    cout << "===========================================================================" << endl;
  }

  return code;
}

int PHG4HoughTransform::process_event(PHCompositeNode *topNode)
{
  _timer.get()->restart();

  if(verbosity > 0) cout << "PHG4HoughTransform::process_event -- entered" << endl;

  // moving clearing to the beginning of event or we will have
  // return bugs from early exits!
  _clusters_init.clear();
  _clusters.clear();
  _tracks.clear();
  
  //---------------------------------
  // Get Objects off of the Node Tree
  //---------------------------------
  
  GetNodes(topNode);
  
  // Translate into Helix_Hough objects
  //-----------------------------------
  //wrap_clusters_timer.get()->restart();

  for (SvtxClusterMap::Iter iter = _g4clusters->begin();
       iter != _g4clusters->end();
       ++iter) {
    SvtxCluster* cluster = &iter->second;

    //cluster->identify();
    
    float phi = atan2(cluster->get_position(1),cluster->get_position(0));
    unsigned int ilayer = _layer_ilayer_map[cluster->get_layer()];
    
    float xy_error=0.;float z_error=0.;
    if (_use_cell_size) {
      xy_error = _smear_xy_layer[ilayer] * _vote_error_scale[ilayer];
      z_error  = _smear_z_layer[ilayer] * _vote_error_scale[ilayer];
      
    }
    else {
      if( cluster->get_phi_size() <= _max_cluster_error*_smear_xy_layer[ilayer] ){xy_error = cluster->get_phi_size() * _vote_error_scale[ilayer];}
      else{xy_error = _max_cluster_error*_smear_xy_layer[ilayer] * _vote_error_scale[ilayer];}
      if(cluster->get_z_size() <= _max_cluster_error*_smear_z_layer[ilayer]){z_error  = cluster->get_z_size() * _vote_error_scale[ilayer];}
      else{z_error  = _max_cluster_error*_smear_z_layer[ilayer] * _vote_error_scale[ilayer];}
    }

    vector<SimpleHit3D>* which_vec = &_clusters;
    if (ilayer<_seed_layers) {which_vec=&_clusters_init;}

    //SimpleHit3D(float xx, float dxx, float yy, float dyy, float zz, float dzz, unsigned int ind, int lyr=-1)
    SimpleHit3D hit3d(cluster->get_x(),fabs(xy_error*sin(phi)),
		      cluster->get_y(),fabs(xy_error*cos(phi)),
		      cluster->get_z(),z_error,
		      cluster->get_id(),ilayer);

    // copy covariance over
    for (int i=0; i<3; ++i) {
      for (int j=i; j<3; ++j) {
	hit3d.set_error(i,j,cluster->get_error(i,j));
      }
    }

    which_vec->push_back(hit3d);
  }

  if (verbosity > 20) {
    cout << "-------------------------------------------------------------------" << endl;
    cout << "PHG4HoughTransform::process_event has the following input clusters:" << endl;

    if (!_clusters_init.empty()) {
      for (unsigned int i = 0; i < _clusters_init.size(); ++i) {
	cout << "n init clusters = "<<_clusters_init.size() << endl;
	_clusters_init[i].print();
      }
    } else {
      for (unsigned int i = 0; i < _clusters.size(); ++i) {
	cout << "n clusters = "<<_clusters.size() << endl;
	_clusters[i].print();
      }
    }
    
    cout << "-------------------------------------------------------------------" << endl;
  }
  
  //------------------------------------
  // Perform the initial zvertex finding
  //------------------------------------

  if(verbosity > 0) cout << "PHG4HoughTransform::process_event -- initial vertex finding..." << endl;

  // Grab some initial tracks for initial z-vertex finding
  _tracks.clear();

  _vertex.clear();
  _vertex.push_back(0.0); // x guess
  _vertex.push_back(0.0); // y guess
  _vertex.push_back(0.0); // z guess

  if(_use_vertex) {
    
    unsigned int nz0 = _tracker_vertex.size();
    double z0min = -10.;
    double z0_step = 20./((double)nz0);
    
    TH1D npairs("npairs", "npairs", nz0, z0min, z0min + z0_step*((double)nz0));
    
    float extrap_scale = _radii[0]/(_radii[1] - _radii[0]);
    for(unsigned int i=0;i<_clusters_init.size();++i)
    {
      if(_clusters_init[i].layer != 0){continue;}
      float z1 = _clusters_init[i].z;
      for(unsigned int j=0;j<_clusters_init.size();++j)
      {
        if(_clusters_init[j].layer != 1){continue;}
        float z2 = _clusters_init[j].z;
        float zint = z1 + (z1-z2)*extrap_scale;
        npairs.Fill(zint);
      }
    }
    
    int which_vtx_bin = (npairs.GetMaximumBin() - 1);
    if( (which_vtx_bin < 0) || (which_vtx_bin >= (int)nz0) ){which_vtx_bin = -1;}
    
    unsigned int maxtracks = 100 * _clusters_init.size() / ( 400*_seed_layers) ;
    //if(maxtracks<100){maxtracks=0;}
    maxtracks = 80;
    
    // find maxtracks tracks
    vector<SimpleTrack3D> temptracks;
    vector<Matrix<float,5,5> > covariances;
    for(unsigned int i=0;i<_tracker_vertex.size();++i)
    {
      if( (which_vtx_bin != -1) && (which_vtx_bin != ((int)i) ) ){continue;}
      _tracker_vertex[i]->clear();
      _tracker_vertex[i]->findHelices(_clusters_init, _req_seed, _max_hits_init, temptracks, maxtracks);
      for(unsigned int t=0;t<temptracks.size();++t)
      {
        _tracks.push_back(temptracks[t]);
        covariances.push_back( (_tracker_vertex[i]->getKalmanStates())[t].C );
      }
    }
    
    if(_tracks.size() == 0){return Fun4AllReturnCodes::EVENT_OK;}
    else if(_tracks.size() == 1)
    {
      _vertex[0] = cos(_tracks[0].phi) * _tracks[0].d;
      _vertex[1] = sin(_tracks[0].phi) * _tracks[0].d;
      _vertex[2] = _tracks[0].z0;
    }
    else
    {
      vector<vector<double> > pTmap;
      for(unsigned int i=0;i<_tracks.size();++i)
      {
        if(_tracks[i].kappa == 0.0){continue;}
        double pT = kappaToPt(_tracks[i].kappa);
        pTmap.push_back(vector<double>());
        pTmap.back().push_back(pT);
        pTmap.back().push_back((double)i);
      }
      sort(pTmap.begin(), pTmap.end());
      vector<SimpleTrack3D> vtxtracks;
      vector<Matrix<float,5,5> > vtxcovariances;
      unsigned int maxvtxtracks=100;
      if(_tracks.size() < maxvtxtracks)
      {
        vtxtracks = _tracks;
        vtxcovariances = covariances;
      }
      else
      {
        for(unsigned int i=0;i<maxvtxtracks;++i)
        {
          vtxtracks.push_back(_tracks[ (int)(pTmap[pTmap.size()-1-i][1]) ]);
          vtxcovariances.push_back(covariances[ (int)(pTmap[pTmap.size()-1-i][1]) ] );
        }
      }
      
      
      vector<double> zvertices(3,0.);
      vector<float> temp_vertex(3,0.);
      vector<unsigned int> vtracks(3,0);
      for(unsigned int iter = 0;iter < 3; ++iter)
      {
        temp_vertex[2] = 0.;
        
        TH1D z0_hist("z0_hist","z0_hist", 20, -10., 10.);
        for(unsigned int i=0;i<vtxtracks.size();++i)
        {
          z0_hist.Fill(vtxtracks[i].z0);
        }
        temp_vertex[2] = z0_hist.GetBinCenter( z0_hist.GetMaximumBin() );
        
        _vertexFinder.findVertex(vtxtracks, vtxcovariances, temp_vertex, 3., true);
        _vertexFinder.findVertex(vtxtracks, vtxcovariances, temp_vertex, 0.1, true);
        _vertexFinder.findVertex(vtxtracks, vtxcovariances, temp_vertex, 0.02, false);
        
        
        vector<SimpleTrack3D> ttracks;
        for(unsigned int t=0;t<vtxtracks.size();++t)
        {
          if( fabs(vtxtracks[t].z0 - temp_vertex[2]) < 0.1 ){vtracks[iter] += 1;}
          else{ttracks.push_back(vtxtracks[t]);}
        }
        vtxtracks = ttracks;
        zvertices[iter] = temp_vertex[2];
      }
      _vertex[2] = zvertices[0];
      unsigned int zbest = 0;
      for(unsigned int iter = 1;iter < 3; ++iter)
      {
        if(vtracks[iter] > vtracks[zbest])
        {
          _vertex[2] = zvertices[iter];
          zbest = iter;
        }
      }
    }
    
    
    
    
    
    
    if(verbosity > 0) cout << "PHG4HoughTransform::process_event -- found initial vertex : " << _vertex[0] << " " << _vertex[1] << " " << _vertex[2] << endl;
    
    _tracks.clear();
    
    // shift the vertex to the origin
    for(unsigned int ht=0;ht<_clusters_init.size();++ht)
    {
      _clusters_init[ht].x -= _vertex[0];
      _clusters_init[ht].y -= _vertex[1];
      _clusters_init[ht].z -= _vertex[2];
    }
    for(unsigned int ht=0;ht<_clusters.size();++ht)
    {
      _clusters[ht].x -= _vertex[0];
      _clusters[ht].y -= _vertex[1];
      _clusters[ht].z -= _vertex[2];
    }
    
    

    
  }  // if(_use_vertex)
  
  //----------------------------------
  // Preform the track finding
  //----------------------------------
  _tracker->clear();
  _tracks.clear();
  _timer_initial_hough.get()->restart();
  _tracker->findHelices(_clusters_init, _min_hits_init, _max_hits_init, _tracks);
  _timer_initial_hough.get()->stop();
  
  

  if(verbosity > 0)
  {
    cout << "PHG4HoughTransform::process_event -- full track finding pass found: " << _tracks.size() << " tracks" << endl;
  }    
   
  //----------------------------
  // Re-center event on detector
  //----------------------------

  if(verbosity > 0) cout << "PHG4HoughTransform::process_event -- recentering event on detector..." << endl;
  vector<double> chi_squareds;
  for(unsigned int tt=0;tt<_tracks.size();tt++)
  {
    // move the hits in the track back to their original position                
    for(unsigned int hh=0;hh<_tracks[tt].hits.size();hh++)
    {
      _tracks[tt].hits[hh].x = _tracks[tt].hits[hh].x + _vertex[0];
      _tracks[tt].hits[hh].y = _tracks[tt].hits[hh].y + _vertex[1];
      _tracks[tt].hits[hh].z = _tracks[tt].hits[hh].z + _vertex[2];
//       _tracks[tt].z0 += _vertex[2];
    }
    chi_squareds.push_back(_tracker->getKalmanStates()[tt].chi2);}

  if(verbosity > 0)
  {
    cout << "PHG4HoughTransform::process_event -- final track count: " << _tracks.size() << endl;
  }

  //---------------------------
  // Final vertex determination
  //---------------------------
  
  // final best guess of the primary vertex position here...
  if(verbosity > 0)
  {
    cout<< "PHG4HoughTransform::process_event -- calculating final vertex" << endl;
  }
  
  // sort the tracks by pT
  vector<vector<double> > pTmap;
  for(unsigned int i=0;i<_tracks.size();++i)
  {
    double pT = kappaToPt(_tracks[i].kappa);
    pTmap.push_back(vector<double>());
    pTmap.back().push_back(pT);
    pTmap.back().push_back((double)i);
  }
  sort(pTmap.begin(), pTmap.end());
  vector<SimpleTrack3D> vtxtracks;
  vector<Matrix<float,5,5> > vtxcovariances;
  unsigned int maxvtxtracks=100;
  if(_tracks.size() < maxvtxtracks){vtxtracks = _tracks;}
  else
  {
    for(unsigned int i=0;i<maxvtxtracks;++i)
    {
      vtxtracks.push_back(_tracks[ (int)(pTmap[pTmap.size()-1-i][1]) ]);
      vtxcovariances.push_back( (_tracker->getKalmanStates())[ (int)(pTmap[pTmap.size()-1-i][1]) ].C );
    }
  }
  
  double vx = _vertex[0];
  double vy = _vertex[1];
  double vz = _vertex[2];
  
  _vertex[0] = 0.;
  _vertex[1] = 0.;
  _vertex[2] = 0.;
  
  _vertexFinder.findVertex(vtxtracks, vtxcovariances, _vertex, 0.3, false);
  _vertexFinder.findVertex(vtxtracks, vtxcovariances, _vertex, 0.1, false);
  _vertexFinder.findVertex(vtxtracks, vtxcovariances, _vertex, 0.02, false);
  _vertexFinder.findVertex(vtxtracks, vtxcovariances, _vertex, 0.005, false);
  
  _vertex[0] += vx;
  _vertex[1] += vy;
  _vertex[2] += vz;
  
  if(verbosity > 0)
  {
    cout << "PHG4HoughTransform::process_event -- final vertex: " << _vertex[0] << " " << _vertex[1] << " " << _vertex[2] << endl;
  }

  //--------------------------------
  // Translate back into PHG4 objects
  //--------------------------------

  if(verbosity > 0)
  {
    cout << "PHG4HoughTransform::process_event -- producing PHG4Track objects..." << endl;
  }

  SvtxVertex vertex;
  vertex.set_t0(0.0);
  for (int i=0;i<3;++i) vertex.set_position(i,_vertex[i]);
  vertex.set_chisq(0.0);
  vertex.set_ndof(0); 
  vertex.set_error(0,0,0.0);
  vertex.set_error(0,1,0.0);
  vertex.set_error(0,2,0.0);
  vertex.set_error(1,0,0.0);
  vertex.set_error(1,1,0.0);
  vertex.set_error(1,2,0.0);
  vertex.set_error(2,0,0.0);
  vertex.set_error(2,1,0.0);
  vertex.set_error(2,2,0.0);
  
  // copy out the reconstructed vertex position
  //_g4tracks->setVertex(_vertex[0],_vertex[1],_vertex[2]);
  //_g4tracks->setVertexError(0.0,0.0,0.0);
 
  // at this point we should already have an initial pt and pz guess...
  // need to translate this into the PHG4Track object...

  vector<SimpleHit3D> track_hits;
  int clusterID;
  int clusterLayer;
  float cluster_x;
  float cluster_y;
  float cluster_z;
  //  float dEdx1;
  //  float dEdx2;

  for(unsigned int itrack=0; itrack<_tracks.size();itrack++)
  {
    SvtxTrack track;
    track.setTrackID(itrack);
    track_hits.clear();
    track_hits = _tracks.at(itrack).hits;
    
    for(unsigned int ihit = 0; ihit<track_hits.size();ihit++)
    {
      //      dEdx1=0;
      //      dEdx2=0;
      if( (track_hits.at(ihit).index) >= _g4clusters->size()){continue;}
      SvtxCluster *cluster = _g4clusters->get(track_hits.at(ihit).index);
      clusterID = cluster->get_id();
      clusterLayer = cluster->get_layer();
      cluster_x = cluster->get_x();
      cluster_y = cluster->get_y();
      cluster_z = cluster->get_z();
      if( (clusterLayer < (int)_seed_layers) && (clusterLayer >= 0) )
      {
        track.setClusterID(clusterLayer, clusterID);
        track.setHitPosition(clusterLayer,cluster_x,cluster_y,cluster_z);
      }
    }
    float kappa = _tracks.at(itrack).kappa;
    float d = _tracks.at(itrack).d;
    float phi = _tracks.at(itrack).phi;

    float dzdl = _tracks.at(itrack).dzdl;
    float z0 = _tracks.at(itrack).z0;
    
    track.phi = phi;
    track.kappa = kappa;
    track.d = d;
    track.z0 = z0;
    track.dzdl = dzdl;
    
    float pT = kappaToPt(kappa);

    float x_center = cos(phi)*(d+1/kappa); // x coordinate of circle center
    float y_center = sin(phi)*(d+1/kappa); // y    "      "     "      "

    // find helicity from cross product sign
    short int helicity;
    if((track_hits[0].x-x_center)*(track_hits[track_hits.size()-1].y-y_center) -
       (track_hits[0].y-y_center)*(track_hits[track_hits.size()-1].x-x_center) > 0)
    {
      helicity = 1;
    }
    else
    { 
      helicity = -1;
    }
    float pZ = 0;
    if(dzdl != 1)
    {
      pZ = pT * dzdl / sqrt(1.0 - dzdl*dzdl);
    }
    int ndf = 2*_tracks.at(itrack).hits.size() - 5;
    track.setQuality(chi_squareds[itrack]/((float)ndf));
    track.setChisq(chi_squareds[itrack]);
    track.setNDF(ndf);
    track.set3Momentum( pT*cos(phi-helicity*M_PI/2), pT*sin(phi-helicity*M_PI/2), pZ);

    track.setDCA2D( d );
    track.setDCA2Dsigma(sqrt(_tracker->getKalmanStates()[itrack].C(1,1)));  

    if(_magField > 0)
    {
      track.setCharge( -1.0*helicity );
    }
    else
    {
      track.setCharge( helicity );
    }
    
    for(unsigned int row=0;row<5;++row)
    {
      for(unsigned int col=0;col<5;++col)
      {
        (*(track.getCovariance()))[row][col] = _tracker->getKalmanStates()[itrack].C(row,col);
      }
    }
    
    _g4tracks->insert(track);
    vertex.insert_track(track.getTrackID());

    if (verbosity > 5) {
      cout << "track " << itrack << " quality = "
           << track.getQuality() << endl;
      cout << "px = " << track.get3Momentum(0)
           << " py = " << track.get3Momentum(1)
           << " pz = " << track.get3Momentum(2) << endl;
    }
  } // track loop

  SvtxVertex *vtxptr = _g4vertexes->insert(vertex);
  if (verbosity > 5) vtxptr->identify();
  
  if(verbosity > 0)
  {
    cout << "PHG4HoughTransform::process_event -- leaving process_event" << endl;
  }

  _timer.get()->stop();
  return Fun4AllReturnCodes::EVENT_OK;
}

int PHG4HoughTransform::End(PHCompositeNode *topNode) {
  for (unsigned int i = 0; i < _tracker_vertex.size(); ++i) {
    delete _tracker_vertex[i];
  }
  delete _tracker;
  
  return Fun4AllReturnCodes::EVENT_OK;
}

int PHG4HoughTransform::InitializeGeometry(PHCompositeNode *topNode) {

  //---------------------------------------------------------
  // Grab Run-Dependent Detector Geometry and Configure Hough
  //---------------------------------------------------------

  bool default_geo = false;
  
  PHG4CylinderCellGeomContainer* cellgeos = findNode::getClass<PHG4CylinderCellGeomContainer>(topNode,"CYLINDERCELLGEOM_SVTX");
  PHG4CylinderGeomContainer* laddergeos = findNode::getClass<PHG4CylinderGeomContainer>(topNode,"CYLINDERGEOM_SILICON_TRACKER");
  //PHG4CylinderCellContainer* cells = findNode::getClass<PHG4CylinderCellContainer>(topNode,"G4CELL_SVTX");
  
  if (cellgeos||laddergeos) {
    unsigned int ncelllayers = 0;
    if (cellgeos) ncelllayers += cellgeos->get_NLayers();
    unsigned int nladderlayers = 0;
    if (laddergeos) nladderlayers += laddergeos->get_NLayers();
    _nlayers = ncelllayers + nladderlayers;
    default_geo = false;
  } else {
    cerr << PHWHERE
	 << "Neither CYLINDERCELLGEOM_SVTX nor CYLINDERGEOM_SILICON_TRACKER available, reverting to a default geometry"
	 << std::endl;    
    _nlayers = 6;
    default_geo = true;
  }

  //=================================================//                                                                                                      
  //  Initializing HelixHough objects                //                                                                                                      
  //=================================================//                                                                                            

  _radii.assign(_nlayers, 0.0);
  _smear_xy_layer.assign(_nlayers, 0.0);
  _smear_z_layer.assign(_nlayers, 0.0);
  float sqrt_12 = sqrt(12.);
	
  if (default_geo) {

    // default geometry
    _radii[0] = 2.5;
    _radii[1] = 5.0;
    _radii[2] = 10.0;
    _radii[3] = 14.0;
    _radii[4] = 40.0;
    _radii[5] = 60.0;
    
    _smear_xy_layer[0] = (50.0e-4/sqrt_12);
    _smear_z_layer[0] = (425.0e-4/sqrt_12);
    _smear_xy_layer[1] = (50.0e-4/sqrt_12);
    _smear_z_layer[1] = (425.0e-4/sqrt_12);
    _smear_xy_layer[2] = (80.0e-4/sqrt_12);
    _smear_z_layer[2] = (1000.0e-4/sqrt_12);
    _smear_xy_layer[3] = (80.0e-4/sqrt_12);
    _smear_z_layer[3] = (1000.0e-4/sqrt_12);
    
    for(int il=4; il<_nlayers; ++il) {
      _smear_xy_layer[il] = (80.0e-4/sqrt_12);
      _smear_z_layer[il] = (30000.0e-4/sqrt_12);
    }

    _layer_ilayer_map.clear();
    for (int ilayer = 0; ilayer < _nlayers; ++ilayer) {
      _layer_ilayer_map.insert(make_pair(ilayer,ilayer));
    }
    
  } else {

    // Since the G4 layers don't necessarily correspond to the
    // silicon layers, and don't necessarily start from zero (argh),
    // we create our own layers numbers that are consecutive
    // starting from zero.

    // Now that we have two kinds of layers, I won't know in principle
    // which type is in what order, so I figure that out now...
    
    map<float,int> radius_layer_map;

    if (cellgeos) {
      PHG4CylinderCellGeomContainer::ConstRange layerrange = cellgeos->get_begin_end();
      for(PHG4CylinderCellGeomContainer::ConstIterator layeriter = layerrange.first;
	  layeriter != layerrange.second;
	  ++layeriter) {
	radius_layer_map.insert( make_pair(layeriter->second->get_radius(),
					   layeriter->second->get_layer()) );
      }
    }

    if (laddergeos) {
      PHG4CylinderGeomContainer::ConstRange layerrange = laddergeos->get_begin_end();
      for(PHG4CylinderGeomContainer::ConstIterator layeriter = layerrange.first;
	  layeriter != layerrange.second;
	  ++layeriter) {
	radius_layer_map.insert( make_pair(layeriter->second->get_radius(),
					   layeriter->second->get_layer()) );
      }
    }

    // now that the layer ids are sorted by radius, I can create a storage
    // index, ilayer, that is 0..N-1 and sorted by radius
    
    int ilayer = 0;
    for(map<float,int>::iterator iter = radius_layer_map.begin();
	iter != radius_layer_map.end();
	++iter) {
      _layer_ilayer_map.insert( make_pair(iter->second,ilayer) );
      ++ilayer;
    }   

    // now we extract the information from the cellgeos first
    if (cellgeos) {    
      PHG4CylinderCellGeomContainer::ConstRange begin_end = cellgeos->get_begin_end();
      PHG4CylinderCellGeomContainer::ConstIterator miter = begin_end.first;
      for( ; miter != begin_end.second; miter++) {
	PHG4CylinderCellGeom *cellgeo = miter->second;
      
	if (verbosity > 1) cellgeo->identify();

	_radii[_layer_ilayer_map[cellgeo->get_layer()]] = cellgeo->get_radius();      
	_smear_xy_layer[_layer_ilayer_map[cellgeo->get_layer()]] = cellgeo->get_radius()*cellgeo->get_phistep();
	_smear_z_layer[_layer_ilayer_map[cellgeo->get_layer()]] = cellgeo->get_zstep();     
      }
    }

    if (laddergeos) {    
      PHG4CylinderGeomContainer::ConstRange begin_end = laddergeos->get_begin_end();
      PHG4CylinderGeomContainer::ConstIterator miter = begin_end.first;
      for( ; miter != begin_end.second; miter++) {
	PHG4CylinderGeom *geo = miter->second;
	
	if (verbosity > 1) geo->identify();
	
	_radii[_layer_ilayer_map[geo->get_layer()]] = geo->get_radius();      
	_smear_xy_layer[_layer_ilayer_map[geo->get_layer()]] = geo->get_strip_y_spacing();
	_smear_z_layer[_layer_ilayer_map[geo->get_layer()]] = geo->get_strip_z_spacing();     
      }
    }
  }  

  // set material on each layer
  
  _material.assign(_radii.size(), 0.03);

  map<int, float>::iterator mat_it;
  for (map<int, float>::iterator iter = _user_material.begin();
       iter != _user_material.end();
       ++iter) {
    _material[_layer_ilayer_map[iter->first]] = iter->second;
  }

  float kappa_max = ptToKappa(_min_pT);

  HelixRange top_range( 0.0, 2.*M_PI,
		       -0.2, 0.2,
			0.0, kappa_max,
		       -0.9, 0.9,
		       -1.0*_dcaz_cut, 1.0*_dcaz_cut);
  if (!_use_vertex) {
    top_range.min_z0 = -10.;
    top_range.max_z0 = 10.;
  }
  
  vector<unsigned int> onezoom(5,0);
  vector<vector<unsigned int> > zoomprofile;
  zoomprofile.assign(5,onezoom);
  zoomprofile[0][0] = 16;
  zoomprofile[0][1] = 1;
  zoomprofile[0][2] = 4;
  zoomprofile[0][3] = 8;
  zoomprofile[0][4] = 1;
  
  zoomprofile[1][0] = 16;
  zoomprofile[1][1] = 1;
  zoomprofile[1][2] = 4;
  zoomprofile[1][3] = 4;
  zoomprofile[1][4] = 2;
  
  zoomprofile[2][0] = 4;
  zoomprofile[2][1] = 3;
  zoomprofile[2][2] = 2;
  zoomprofile[2][3] = 1;
  zoomprofile[2][4] = 3;
  
  for (unsigned int i = 2; i <= 3; ++i) {
    zoomprofile[i][0] = 3;
    zoomprofile[i][1] = 3;
    zoomprofile[i][2] = 3;
    zoomprofile[i][3] = 3;
    zoomprofile[i][4] = 3;
  }
    
  _tracker = new sPHENIXTracker(zoomprofile, 1, top_range, _material, _radii, _magField);
  _tracker->setNLayers(_seed_layers);
  _tracker->requireLayers(_req_seed);
  _max_hits_init = _seed_layers*4;
  if(_seed_layers >= 10){_max_hits_init = _seed_layers*2;}
  _min_hits_init = _req_seed;
  if(_seed_layers < 10){ _tracker->setClusterStartBin(1); }
  else{ _tracker->setClusterStartBin(10); }
  _tracker->setRejectGhosts(_reject_ghosts);
  _tracker->setFastChi2Cut(_chi2_cut_fast_par0,
			   _chi2_cut_fast_par1,
			   _chi2_cut_fast_max);
  _tracker->setChi2Cut(_chi2_cut_full);
  _tracker->setChi2RemovalCut(_chi2_cut_full*0.5);
  _tracker->setCellularAutomatonChi2Cut(_ca_chi2_cut);
  _tracker->setPrintTimings(false);
  if(verbosity > 3){_tracker->setPrintTimings(true);}
  _tracker->setVerbosity(verbosity);
  _tracker->setCutOnDca(_cut_on_dca);
  _tracker->setDcaCut(_dca_cut);
  _tracker->setSmoothBack(true);
  _tracker->setBinScale(_bin_scale);
  _tracker->setZBinScale(_z_bin_scale);
  _tracker->setRemoveHits(_remove_hits);
  _tracker->setSeparateByHelicity(true);
  _tracker->setMaxHitsPairs(0);
  _tracker->setCosAngleCut(_cos_angle_cut);
  
  
  vector<vector<unsigned int> > zoomprofile_init;
  zoomprofile_init.assign(4,onezoom);
  for(unsigned int i=0;i<=1;++i)
  {
    zoomprofile_init[i][0] = 8;
    zoomprofile_init[i][1] = 1;
    zoomprofile_init[i][2] = 3;
    zoomprofile_init[i][3] = 4;
    zoomprofile_init[i][4] = 4;
  }
  for(unsigned int i=2;i<=3;++i)
  {
    zoomprofile_init[i][0] = 8;
    zoomprofile_init[i][1] = 1;
    zoomprofile_init[i][2] = 2;
    zoomprofile_init[i][3] = 2;
    zoomprofile_init[i][4] = 2;
  }
  vector<HelixRange> top_range_init;
  unsigned int nphi = 1;
  unsigned int nz0 = 5;
  double phimin = 0.;
  double phi_step = 2.0*M_PI/((double)nphi);
  float kappa_max_init = ptToKappa(_min_pT_init);
  for(unsigned int i=0;i<nphi;++i)
  {
    double z0min = -10.;
    double z0_step = 20./((double)nz0);
    for(unsigned int j=0;j<nz0;++j)
    {
      top_range_init.push_back(HelixRange(phimin, phimin+phi_step,   -0.2, 0.2,   0.0, kappa_max_init,   -0.9, 0.9,   z0min, z0min+z0_step));
      _tracker_vertex.push_back( new sPHENIXTracker(zoomprofile_init, 1, top_range_init.back(), _material, _radii, _magField) );
      if(verbosity > 3){(_tracker_vertex.back())->setPrintTimings(true);}
      (_tracker_vertex.back())->setVerbosity(verbosity);
      (_tracker_vertex.back())->setNLayers(_seed_layers);
      (_tracker_vertex.back())->requireLayers(_req_seed);
      (_tracker_vertex.back())->setClusterStartBin(1);
      (_tracker_vertex.back())->setRejectGhosts(true);
      (_tracker_vertex.back())->setFastChi2Cut(_chi2_cut_fast_par0,
					       _chi2_cut_fast_par1,
					       _chi2_cut_fast_max);
      (_tracker_vertex.back())->setChi2Cut(_chi2_cut_init);
      (_tracker_vertex.back())->setChi2RemovalCut(_chi2_cut_init*0.5);
      (_tracker_vertex.back())->setCellularAutomatonChi2Cut(_ca_chi2_cut);
      (_tracker_vertex.back())->setCutOnDca(false);
      (_tracker_vertex.back())->setSmoothBack(true);
      (_tracker_vertex.back())->setBinScale(_bin_scale);
      (_tracker_vertex.back())->setZBinScale(_z_bin_scale);
      (_tracker_vertex.back())->setRemoveHits(true);
      (_tracker_vertex.back())->setSeparateByHelicity(true);
      (_tracker_vertex.back())->setMaxHitsPairs(0);
      (_tracker_vertex.back())->setCosAngleCut(_cos_angle_cut);
      z0min += z0_step;
    }
    phimin += phi_step;
  }
  
  for(unsigned int ilayer = 0; ilayer < _fit_error_scale.size(); ++ilayer) {
    float scale1 = _fit_error_scale[ilayer];
    float scale2 = _vote_error_scale[ilayer];
    float scale = scale1/scale2;
    _tracker->setHitErrorScale(ilayer, scale);
    for(unsigned int j = 0; j < _tracker_vertex.size(); ++j) {
      _tracker_vertex[j]->setHitErrorScale(ilayer, scale);
    }
  }
  
  return Fun4AllReturnCodes::EVENT_OK;
}

void PHG4HoughTransform::set_material(int layer, float value)
{
  _user_material[layer] = value;
}

int PHG4HoughTransform::CreateNodes(PHCompositeNode *topNode)
{
  // create nodes...
  PHNodeIterator iter(topNode);
  
  PHCompositeNode *dstNode = static_cast<PHCompositeNode*>(iter.findFirst("PHCompositeNode", "DST"));
  if(!dstNode)
  {
    cerr << PHWHERE << "DST Node missing, doing nothing." << endl;
    return Fun4AllReturnCodes::ABORTEVENT;
  }
      
  // Create the SVTX node
  PHCompositeNode* tb_node = dynamic_cast<PHCompositeNode*>(iter.findFirst("PHCompositeNode", "SVTX"));
  if (!tb_node) 
  {
    tb_node = new PHCompositeNode("SVTX");
    dstNode->addNode(tb_node);
    if (verbosity>0) cout << "SVTX node added" << endl;
  }
 	
  _g4tracks = new SvtxTrackMap;
  PHIODataNode<PHObject>* tracks_node = new PHIODataNode<PHObject>(_g4tracks,"SvtxTrackMap","PHObject");
  tb_node->addNode(tracks_node);
  if (verbosity>0) cout << "Svtx/SvtxTrackMap node added" << endl;

  _g4vertexes = new SvtxVertexMap;
  PHIODataNode<PHObject>* vertexes_node = new PHIODataNode<PHObject>(_g4vertexes,"SvtxVertexMap","PHObject");
  tb_node->addNode(vertexes_node);
  if (verbosity>0) cout << "Svtx/SvtxVertexMap node added" << endl;
  
  PHG4CylinderGeomContainer* geoms = getClass<PHG4CylinderGeomContainer>(topNode, "CYLINDERGEOM_SVTX");
  if(!geoms) 
  {
    cerr << PHWHERE << " ERROR: Can't find CYLINDERGEOM_SVTX Node." << endl;
    return Fun4AllReturnCodes::ABORTEVENT;
  }

  return InitializeGeometry(topNode);
}

int PHG4HoughTransform::GetNodes(PHCompositeNode *topNode)
{
  //---------------------------------
  // Get Objects off of the Node Tree
  //---------------------------------
  
  _ghitlist = getClass<PHG4HitContainer>(topNode,"G4HIT_SVTX");
  if(!_ghitlist) 
  {
    cerr << PHWHERE << " ERROR: Can't find node PHG4HitContainer" << endl;
    return Fun4AllReturnCodes::ABORTEVENT;
  }
  
  _g4clusters = getClass<SvtxClusterMap>(topNode,"SvtxClusterMap");
  if(!_g4clusters) 
  {
    cerr << PHWHERE << " ERROR: Can't find node SvtxClusterMap" << endl;
    return Fun4AllReturnCodes::ABORTEVENT;
  }

  // Pull the reconstructed track information off the node tree...
  _g4tracks = getClass<SvtxTrackMap>(topNode, "SvtxTrackMap");
  if(!_g4tracks) 
  {
    cerr << PHWHERE << " ERROR: Can't find SvtxTrackMap." << endl;
    return Fun4AllReturnCodes::ABORTEVENT;
  }

  // Pull the reconstructed track information off the node tree...
  _g4vertexes = getClass<SvtxVertexMap>(topNode, "SvtxVertexMap");
  if(!_g4vertexes) 
  {
    cerr << PHWHERE << " ERROR: Can't find SvtxVertexMap." << endl;
    return Fun4AllReturnCodes::ABORTEVENT;
  }
  
  return Fun4AllReturnCodes::EVENT_OK;
}



