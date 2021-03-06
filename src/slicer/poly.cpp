/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010 Kulitorum
    Copyright (C) 2011-12 martin.dieringer@gmx.de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "poly.h"

#include "layer.h"
#include "config.h"
#include "shape.h"
//#include "infill.h"
#include "printlines.h"
#include "clipping.h"


#include <poly2tri/poly2tri/poly2tri/poly2tri.h>


Poly::Poly(){
  holecalculated = false;
  this->z = -10;
  extrusionfactor = 1.;
}

Poly::Poly(double z, double extrusionfactor){
  this->z = z;
  this->extrusionfactor = extrusionfactor;
  holecalculated = false;
  hole=false;
  //cout << "POLY WITH PLANE"<< endl;
  //plane->printinfo();
  //printinfo();
}

Poly::Poly(const Poly p, double z){
  this->z = z;
  this->extrusionfactor = p.extrusionfactor;
  holecalculated = false;
  hole=false;
  uint count = p.vertices.size();
  vertices.resize(count);
  for (uint i=0; i<count ; i++)
    vertices[i] = p.vertices[i];
}


Poly::~Poly()
{
}

void Poly::cleanup(double epsilon)
{
  vertices = cleaned(vertices, epsilon);
  uint n_vert = vertices.size();
  vector<Vector2d> invert;
  invert.insert(invert.end(),vertices.begin()+n_vert/2,vertices.end());
  invert.insert(invert.end(),vertices.begin(),vertices.begin()+n_vert/2);
  vertices = cleaned(invert, epsilon);
}

// Douglas-Peucker algorithm
vector<Vector2d> Poly::cleaned(const vector<Vector2d> vert, double epsilon) const
{ 
  if (epsilon == 0) return vert;
  uint n_vert = vert.size();
  if (n_vert<3) return vert;
  double dmax = 0;
  //Find the point with the maximum distance from line start-end
  uint index = 0;
  Vector2d normal = (vert.back()-vert.front()).normal();
  normal.normalize();
  if( (normal.length()==0) || ((abs(normal.length())-1)>epsilon) ) return vert;
  for (uint i = 1; i < n_vert-1 ; i++) 
    {
      double dist = abs((vert[i]-vert.front()).dot(normal));
      if (dist >= epsilon && dist > dmax) {
	index = i;
	dmax = dist;
      }
    }
  vector<Vector2d> newvert;
  if (index > 0) // there is a point > epsilon
    {
      // divide at max dist point and cleanup both parts recursively 
      vector<Vector2d> part1;
      part1.insert(part1.end(), vert.begin(), vert.begin()+index+1);
      vector<Vector2d> c1 = cleaned(part1, epsilon);
      vector<Vector2d> part2;
      part2.insert(part2.end(), vert.begin()+index, vert.end());
      vector<Vector2d> c2 = cleaned(part2, epsilon);
      newvert.insert(newvert.end(), c1.begin(), c1.end()-1);
      newvert.insert(newvert.end(), c2.begin(), c2.end());
    }
  else 
    { // all points are nearer than espilon
      newvert.push_back(vert.front());
      newvert.push_back(vert.back());
    }
  return newvert;
}


void Poly::calcHole()
{
  	if(vertices.size() == 0)
	  return;	// hole is undefined
	Vector2d p(-6000, -6000);
	int v=0;
	center = Vector2d(0,0);
	Vector2d q;
	for(size_t vert=0;vert<vertices.size();vert++)
	{
	  q = vertices[vert];
	  center += q;
	  if(q.x > p.x)
	    {
	      p = q;
	      v=vert;
	    }
	  else if(q.x == p.x && q.y > p.y)
	    {
	      p.y = q.y;
	      v=vert;
	    }
	}
	center /= vertices.size();

	// we have the x-most vertex (with the highest y if there was a contest), v 
	Vector2d V1 = getVertexCircular(v-1);
	Vector2d V2 = getVertexCircular(v);
	Vector2d V3 = getVertexCircular(v+1);

	Vector2d Va=V2-V1;
	Vector2d Vb=V3-V2;
	hole = Va.cross(Vb) > 0; 
	holecalculated = true;
}

bool Poly::isHole() 
{
  if (!holecalculated)
    calcHole();
  return hole;
}

Vector2d Poly::getCenter() 
{
  if (!holecalculated) calcHole();
  return center;
}

void Poly::rotate(Vector2d center, double angle) 
{
  double sina = sin(angle);
  double cosa = cos(angle);
  for (uint i = 0; i < vertices.size();  i++) {
    Vector2d mv = vertices[i]-center;
    vertices[i] = Vector2d(mv.x*cosa-mv.y*sina+center.x, 
			   mv.y*cosa+mv.x*sina+center.y);
  }
}

// nearest connection point indices of this and other poly 
void Poly::nearestIndices(const Poly p2, int &thisindex, int &otherindex) const
{
  double mindist = 1000000;
  for (uint i = 1; i < vertices.size(); i++) {
    for (uint j = 1; j < p2.vertices.size(); j++) {
      double d = (vertices[i]-p2.vertices[j]).lengthSquared();
      if (d<mindist) {
	mindist = d;
	thisindex = i;
	otherindex= j;
      }
    }
  }
}

// Find the vertex in the poly closest to point p
uint Poly::nearestDistanceSqTo(const Vector2d p, double &mindist) const
{
  assert(vertices.size() > 0);
  // Start with first vertex as closest
  uint nindex = 0;
  mindist = (vertices[0]-p).lengthSquared();
  // check the rest of the vertices for a closer one.
  for (uint i = 1; i < vertices.size(); i++) {
    double d = (vertices[i]-p).lengthSquared();
    if (d<mindist) {
      mindist= d;
      nindex = i;
    }
  }
  return nindex;
}
// returns length and two points
double Poly::shortestConnectionSq(const Poly p2, Vector2d &start, Vector2d &end) const
{
  double min1 = 100000000, min2 = 100000000;
  int minindex1=0, minindex2=0;
  Vector2d onpoint1, onpoint2;
  // test this vertices
  for (uint i = 0; i < vertices.size(); i++) {
    for (uint j = 0; j < p2.vertices.size(); j++) {
      Vector2d onpoint; // on p2
      // dist from point i to lines on p2
      const double mindist = minimum_distance_Sq(p2.vertices[j], 
						 p2.getVertexCircular(j+1),
						 vertices[i], onpoint);
      if (mindist < min1) {
	min1 = mindist; onpoint1 = onpoint; minindex1 = i;
      }
    }
  }
  // test p2 vertices  
  for (uint i = 0; i < p2.vertices.size(); i++) {
    for (uint j = 0; j < vertices.size(); j++) {
      Vector2d onpoint; // on this
      // dist from p2 point i to lines on this
      const double mindist = minimum_distance_Sq(vertices[j], 
						 getVertexCircular(j+1),
						 p2.vertices[i], onpoint);
      if (mindist < min2) {
	min2 = mindist; onpoint2 = onpoint; minindex2 = i;
      }
    }
  }
  if (min1 < min2) { // this vertex, some point on p2 lines
    start = getVertexCircular(minindex1);
    end = onpoint1;
  } else { // p2 vertex, some point of this lines
    start = p2.getVertexCircular(minindex2);
    end = onpoint2;
  }
  return (end-start).lengthSquared();
}


bool Poly::vertexInside(const Vector2d point, double maxoffset) const
{
  // Shoot a ray along +X and count the number of intersections.
  // If n_intersections is even, return false, else return true
  Vector2d EndP(point.x+10000, point.y);
  int intersectcount = 1; // we want to test if uneven

  for(size_t i=0; i<vertices.size();i++)
    {
      Vector2d P1 = getVertexCircular(i-1);
      Vector2d P2 = vertices[i];
                   
      // Skip horizontal lines, we can't intersect with them, 
      // because the test line is horizontal
      if(P1.y == P2.y)      
	continue;
      
      Intersection hit;
      if(IntersectXY(point,EndP,P1,P2,hit,maxoffset))
	intersectcount++;
    }
  return intersectcount%2;
}

// http://paulbourke.net/geometry/insidepoly/
// not really working
bool Poly::vertexInside2(const Vector2d p, double maxoffset) const
{
  uint c = false;
  //Poly off = Clipping::getOffset(*this,maxoffset).front();
  for (uint i = 0; i < vertices.size();  i++) {
    Vector2d Pi = vertices[i];
    Vector2d Pj = getVertexCircular(i+1);
    if ( ((Pi.y > p.y) != (Pj.y > p.y)) &&
	 (abs(p.x - (Pj.x-Pi.x) * (p.y-Pi.y) / (Pj.y-Pj.y) + Pi.x) > maxoffset) )
      c = !c;
  }
  if (!c) 
    for (uint i = 0; i < vertices.size();  i++) 
      if ((vertices[i]-p).length() < maxoffset) return true; // on a vertex    
  return c;
}

// given poly completely contained in this?
bool Poly::polyInside(const Poly * poly, double maxoffset) const
{
  uint i, count=0;
  for (i = 0; i < poly->vertices.size();  i++) {
    Vector2d P = poly->vertices[i];
    if (vertexInside(P,maxoffset))
      count++;
  }
  return count == poly->vertices.size();
}


void Poly::addVertex(Vector2d v, bool front)
{
  if (front)
    vertices.insert(vertices.begin(),v);
  else
    vertices.push_back(v);
};


Vector2d Poly::getVertexCircular(int index) const
{
  int size = vertices.size();
  index = (index + size) % size;
  //cerr << vertices->size() <<" >  "<< points[pointindex] << endl;
  return vertices[index];
}

Vector3d Poly::getVertexCircular3(int pointindex) const
{
  Vector2d v = getVertexCircular(pointindex);
  return Vector3d(v.x,v.y,z);
}

vector<Vector2d> Poly::getVertexRangeCircular(int from, int to) const
{
  vector<Vector2d> v;
  int size = vertices.size();
  for (int i = from; i<=to; i++) 
    v.push_back(vertices[(i+size)%size]);
  return v;
}


vector<Intersection> Poly::lineIntersections(const Vector2d P1, const Vector2d P2,
					     double maxerr) const
{
  vector<Intersection> HitsBuffer;
  Vector2d P3,P4;
  for(size_t i = 0; i < vertices.size(); i++)
    {  
      P3 = getVertexCircular(i);
      P4 = getVertexCircular(i+1);
      Intersection hit;
      if (IntersectXY(P1,P2,P3,P4,hit,maxerr))
	HitsBuffer.push_back(hit);
    }
  // std::sort(HitsBuffer.begin(),HitsBuffer.end());
  // vector<Vector2d> v(HitsBuffer.size());
  // for(size_t i = 0; i < v.size(); i++)
  //   v[i] = HitsBuffer[i].p;
  return HitsBuffer;
}

double Poly::getZ() const {return z;} 
// double Poly::getLayerNo() const { return plane->LayerNo;}


// length of the line starting at startindex
double Poly::getLinelengthSq(uint startindex) const
{
  double length = (getVertexCircular(startindex+1) - 
		   getVertexCircular(startindex)).lengthSquared();
  return length;
}

double Poly::averageLinelengthSq() const
{
  double l=0;
  for (uint i = 0; i<vertices.size(); i++){
    l+=getLinelengthSq(i);
  }
  return l/vertices.size();
}

// add to lines starting with nearest point to startPoint
void Poly::getLines(vector<Vector3d> &lines, Vector2d &startPoint) const
{
  if (size()<2) return;
  double mindist = 1000000;
  uint index = nearestDistanceSqTo(startPoint, mindist);
  getLines(lines,index);
  startPoint = Vector2d(lines.back().x,lines.back().y);
}
void Poly::getLines(vector<Vector2d> &lines, Vector2d &startPoint) const
{
  if (size()<2) return;
  double mindist = 1000000;
  uint index = nearestDistanceSqTo(startPoint, mindist);
  getLines(lines,index);
  startPoint = Vector2d(lines.back().x,lines.back().y);
}

// add to lines starting with given index
// closed lines sequence if number of vertices > 2
void Poly::getLines(vector<Vector2d> &lines, uint startindex) const
{
  size_t count = vertices.size();
  if (count<2) return; // one point no line
  if (count<3) count--; // two points one line

  for(size_t i=0;i<count;i++)
    {
      lines.push_back(getVertexCircular(i+startindex));
      lines.push_back(getVertexCircular(i+startindex+1));
    }
}
void Poly::getLines(vector<Vector3d> &lines, uint startindex) const
{
  size_t count = vertices.size();
  if (count<2) return; // one point no line
  if (count<3) count--; // two points one line
  for(size_t i=0;i<count;i++)
    {
      lines.push_back(getVertexCircular3(i+startindex));
      lines.push_back(getVertexCircular3(i+startindex+1));
    }
}

vector<Vector2d> Poly::getPathAround(const Vector2d from, const Vector2d to) const
{
  double dist;
  vector<Vector2d> path1, path2;
  // Poly off = Clipping::getOffset(*this, 0, jround).front();
  //cerr << size()<<  " Off " << off.size()<< endl;
  int nvert = size();
  if (nvert==0) return path1;
  int fromind = (int)nearestDistanceSqTo(from, dist);
  int toind = (int)nearestDistanceSqTo(to, dist);
  if (fromind==toind) {
    path1.push_back(vertices[fromind]);
    return path1;
  }
  //calc both direction paths
  if(fromind < toind) {
    for (int i=fromind; i<=toind; i++)
      path1.push_back(getVertexCircular(i));
    for (int i=fromind+nvert; i>=toind; i--)
      path2.push_back(getVertexCircular(i));
  } else {
    for (int i=fromind; i>=toind; i--)
      path1.push_back(getVertexCircular(i));
    for (int i=fromind; i<=toind+nvert; i++)
      path2.push_back(getVertexCircular(i));
  }
  // find shorter one
  double len1=0,len2=0;
  for (uint i=1; i<path1.size(); i++) 
    len1+=(path1[i]-path1[i-1]).lengthSquared();
  for (uint i=1; i<path2.size(); i++) 
    len2+=(path2[i]-path2[i-1]).lengthSquared();
  if (len1 < len2) {
     // path1.insert(path1.begin(),from);
     // path1.push_back(to);
    return path1;
  }
  else{
     // path2.insert(path2.begin(),from);
     // path2.push_back(to);
    return path2;
  }
}


vector<Vector2d> Poly::getMinMax() const{
  double minx=6000,miny=6000;
  double maxx=-6000,maxy=-6000;
  vector<Vector2d> range;
  range.resize(2);
  Vector2d v;
  for (uint i=0; i < vertices.size();i++){
    v = vertices[i];
    if (v.x<minx) minx=v.x;
    if (v.x>maxx) maxx=v.x;
    if (v.y<miny) miny=v.y;
    if (v.y>maxy) maxy=v.y;
  }
  range[0] = Vector2d(minx,miny);
  range[1] = Vector2d(maxx,maxy);
  return range;
}


int Poly::getTriangulation(vector<Triangle> &triangles)  const 
{
  vector<p2t::Point*> points(vertices.size());
  for (guint i=0; i<vertices.size(); i++)  
    points[i] = new p2t::Point(vertices[i].x,vertices[i].y);
  p2t::CDT cdt(points);
  cdt.Triangulate();
  vector<p2t::Triangle*> ptriangles = cdt.GetTriangles();
  //vector<Triangle> triangles(ptriangles.size());
  for (guint i=0; i<ptriangles.size(); i++) {
    Vector3d A(ptriangles[i]->GetPoint(0)->x, ptriangles[i]->GetPoint(0)->y, z);
    Vector3d B(ptriangles[i]->GetPoint(1)->x, ptriangles[i]->GetPoint(1)->y, z);
    Vector3d C(ptriangles[i]->GetPoint(2)->x, ptriangles[i]->GetPoint(2)->y, z);
    triangles.push_back(Triangle(A, B, C));
  }
  return triangles.size();
}



Vector3d rotatedZ(Vector3d v, double angle) 
{
  double sina = sin(angle);
  double cosa = cos(angle);
  return Vector3d(v.x*cosa-v.y*sina,
		  v.y*cosa+v.x*sina, v.z);
}


void Poly::draw(int gl_type, double z, bool randomized) const
{
  Vector2d v;
  uint count = vertices.size();
  glBegin(gl_type);	  
  for (uint i=0;i < count;i++){
    v = getVertexCircular(i);
    if (randomized) v = random_displace(v);
    glVertex3f(v.x,v.y,z);
  }
  glEnd();
}

void Poly::draw(int gl_type, bool reverse, bool randomized) const
{
  Vector3d v;//,vn,m,dir;
  uint count = vertices.size();
  glBegin(gl_type);	  
  for (uint i=0;i < count;i++){
    if (reverse){
      v = getVertexCircular3(-i);
      // vn = getVertexCircular3(-i-1);
    } else {
      v = getVertexCircular3(i);
      // vn = getVertexCircular3(i+1);
    }
    if (randomized) v = random_displace(v);
    glVertex3f(v.x,v.y,v.z);
    // if (gl_type==GL_LINE_LOOP){
    //   m = (v+vn)/2;
    //   dir = m + rotatedZ((vn-v)/10,M_PI/2);
    //   glVertex3f(dir.x,dir.y,z);
    // }
  }
  glEnd();
}

void Poly::drawVertexNumbers() const
{
  Vector3d v;
  for (uint i=0;i < vertices.size();i++){
    v = getVertexCircular3(i);
    glVertex3f(v.x,v.y,v.z);
    ostringstream oss;
    oss << i;
    drawString(v, oss.str());
  }
}

void Poly::drawLineNumbers() const
{
  Vector3d v,v2;
  for (uint i=0;i < vertices.size();i++){
    v = getVertexCircular3(i);
    v2 = getVertexCircular3(i+1);
    ostringstream oss;
    oss << i;
    drawString((v+v2)/2., oss.str());
  }
}


string Poly::info() const
{
  ostringstream ostr;
  ostr <<"Poly at Z="<<z;
  ostr <<", "<< vertices.size() <<" vertices";
  ostr <<", extrf="<< extrusionfactor;
  return ostr.str();
}

