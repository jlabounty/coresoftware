#ifndef PHG4BLOCKCELLRECO_H
#define PHG4BLOCKCELLRECO_H

#include <fun4all/SubsysReco.h>
#include <phool/PHTimeServer.h>
#include <string>
#include <map>

class PHCompositeNode;
class PHG4BlockCell;

class PHG4BlockCellReco : public SubsysReco
{
 public:

  PHG4BlockCellReco(const std::string &name = "BLOCKRECO");

  virtual ~PHG4BlockCellReco(){}
  
  //! module initialization
  int InitRun(PHCompositeNode *topNode);
  
  //! run initialization
  int Init(PHCompositeNode *topNode) {return 0;}
  
    //! event processing
  int process_event(PHCompositeNode *topNode);
  
  //! end of process
  int End(PHCompositeNode *topNode);
  
  void Detector(const std::string &d) {detector = d;}
  void cellsize(const int i, const double sr, const double sz);
  void etaxsize(const int i, const double deltaeta, const double deltax);
  void checkenergy(const int i=1) {chkenergyconservation = i;}

 protected:
  void set_size(const int i, const double sizeA, const double sizeB, const int what);
  int CheckEnergy(PHCompositeNode *topNode);
  static std::pair<double, double> get_etaphi(const double x, const double y, const double z);
  static double get_eta(const double radius, const double z);
  bool lines_intersect( double ax, double ay, double bx, double by, double cx, double cy, double dx, double dy, double* rx, double* ry);
  bool line_and_rectangle_intersect( double ax, double ay, double bx, double by, double cx, double cy, double dx, double dy, double* rr);

  std::map<int, int>  binning;
  std::map<int, std::pair <double,double> > cell_size; // cell size in x/z
  std::map<int, std::pair <double,double> > zmin_max; // zmin/zmax for each layer for faster lookup
  std::map<int, double> xstep;
  std::map<int, double> etastep;
  std::string detector;
  std::string hitnodename;
  std::string cellnodename;
  std::string geonodename;
  std::string seggeonodename;
  std::map<int, std::pair<int, int> > n_x_z_bins;
  PHTimeServer::timer _timer;
  int nbins[2];
  int chkenergyconservation;

};

#endif
