#include "collapse_small_triangles.h"

#include <igl/bounding_box_diagonal.h>
#include <igl/doublearea.h>
#include <igl/edge_lengths.h>
#include <igl/colon.h>
#include <igl/faces_first.h>

#include <limits>

#include <iostream>

void collapse_small_triangles(
  const Eigen::MatrixXd & V,
  const Eigen::MatrixXi & F,
  const double eps,
  Eigen::MatrixXi & FF)
{
  using namespace Eigen;
  using namespace igl;
  using namespace std;

  // Compute bounding box diagonal length
  double bbd = bounding_box_diagonal(V,F);
  MatrixXd l;
  edge_lengths(V,F,l);
  VectorXd dblA;
  doublearea(l,dblA);

  // Minimum area tolerance
  const double min_dblarea = 2.0*eps*bbd*bbd;

  Eigen::VectorXi FIM = colon<int>(0,V.rows()-1);
  int num_edge_collapses = 0;
  // Loop over triangles
  for(int f = 0;f<F.rows();f++)
  {
    if(dblA(f) < min_dblarea)
    {
      double minl = 0;
      int minli = -1;
      // Find shortest edge
      for(int e = 0;e<3;e++)
      {
        if(minli==-1 || l(f,e)<minl)
        {
          minli = e;
          minl = l(f,e);
        }
      }
      double maxl = 0;
      int maxli = -1;
      // Find longest edge
      for(int e = 0;e<3;e++)
      {
        if(maxli==-1 || l(f,e)>maxl)
        {
          maxli = e;
          maxl = l(f,e);
        }
      }
      // Be sure that min and max aren't the same
      maxli = (minli==maxli?(minli+1)%3:maxli);

      // Collapse min edge maintaining max edge: i-->j
      // Q: Why this direction?
      int i = maxli;
      int j = ((minli+1)%3 == maxli ? (minli+2)%3: (minli+1)%3);
      assert(i != minli);
      assert(j != minli);
      assert(i != j);
      FIM(F(f,i)) = FIM(F(f,j));
      num_edge_collapses++;
    }
  }

  // Reindex faces
  MatrixXi rF = F;
  // Loop over triangles
  for(int f = 0;f<rF.rows();f++)
  {
    for(int i = 0;i<rF.cols();i++)
    {
      rF(f,i) = FIM(rF(f,i));
    }
  }

  FF.resize(rF.rows(),rF.cols());
  int num_face_collapses=0;
  // Only keep uncollapsed faces
  {
    int ff = 0;
    // Loop over triangles
    for(int f = 0;f<rF.rows();f++)
    {
      bool collapsed = false;
      // Check if any indices are the same
      for(int i = 0;i<rF.cols();i++)
      {
        for(int j = i+1;j<rF.cols();j++)
        {
          if(rF(f,i)==rF(f,j))
          {
            collapsed = true;
            num_face_collapses++;
            break;
          }
        }
      }
      if(!collapsed)
      {
        FF.row(ff++) = rF.row(f);
      }
    }
    // Use conservative resize
    FF.conservativeResize(ff,FF.cols());
  }
  //cout<<"num_edge_collapses: "<<num_edge_collapses<<endl;
  //cout<<"num_face_collapses: "<<num_face_collapses<<endl;
  if(num_edge_collapses == 0)
  {
    // There must have been a "collapsed edge" in the input
    assert(num_face_collapses==0);
    // Base case
    return;
  }

  //// force base case
  //return;

  MatrixXi recFF = FF;
  return collapse_small_triangles(V,recFF,eps,FF);
}
