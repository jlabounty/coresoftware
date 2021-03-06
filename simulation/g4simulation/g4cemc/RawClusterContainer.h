#ifndef RAWCLUSTERCONTAINER_H__
#define RAWCLUSTERCONTAINER_H__

#include <phool/PHObject.h>
#include <phool/phool.h>
#include <iostream>
#include <map>

class RawCluster;

class RawClusterContainer : public PHObject 
{

 public:

  typedef std::map<unsigned int,RawCluster *> Map;
  typedef Map::iterator Iterator;
  typedef Map::const_iterator ConstIterator;
  typedef std::pair<Iterator, Iterator> Range;
  typedef std::pair<ConstIterator, ConstIterator> ConstRange;

  RawClusterContainer() {}
  virtual ~RawClusterContainer() {}

  void Reset();
  int isValid() const;
  void identify(std::ostream& os=std::cout) const;
  ConstIterator AddCluster(RawCluster *clus);
  RawCluster *getCluster(const unsigned int id);
  //! return all clusters
  ConstRange getClusters( void ) const;

  unsigned int size() const {return _clusters.size();}
  double getTotalEdep() const;

 protected:
  Map _clusters;

  ClassDef(RawClusterContainer,1)
};

#endif /* RAWCLUSTERCONTAINER_H__ */
