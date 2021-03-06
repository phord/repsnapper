/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2012  martin.dieringer@gmx.de

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

#pragma once
#include "types.h"

#include <clipper/clipper/polyclipping/trunk/cpp/clipper.hpp>
// see http://angusj.com/delphi/clipper.php


#include "poly.h"



// Clipper uses non-negative long long integers, so we transform by:
const double CL_FACTOR = 10000; // 1 = 1/10000 mm
const double CL_OFFSET = 10000; // 10 meters

namespace CL = ClipperLib;


enum PolyType{subject,clip};
enum JoinType{jsquare,jmiter,jround};


class Clipping
{
  friend class Poly;

  static CL::Clipper clpr;

  double lastZ;     // remember last added polygon's Z for output
  double lastExtrF; // remember last added polygon's extrusion factor for output

  static CL::JoinType CLType(JoinType type);
  static CL::PolyType CLType(PolyType type);

  static CL::IntPoint ClipperPoint(Vector2d v);
  static Vector2d getPoint(CL::IntPoint p);

  static CL::Polygons CLOffset(CL::Polygons cpolys, int cldist, 
			       CL::JoinType cljtype, double miter_limit=1, bool reverse=false);

public:
  Clipping(){};
  ~Clipping(){clear();};

  void clear(){clpr.Clear();};

  void addPoly(const Poly poly, PolyType type);
  void addPolys(const vector<Poly> poly, PolyType type);
  void addPolys(const vector<ExPoly> expolys, PolyType type);
  void addPolys(const ExPoly poly, PolyType type);

  vector<Poly> intersect();
  vector<ExPoly> ext_intersect();
  vector<Poly> unite();
  vector<Poly> subtract();
  vector<ExPoly> ext_subtract();
  vector<Poly> subtractMerged();
  // vector<Poly> xor();

  static vector<Poly> getMerged(vector<Poly> polys);
  static CL::Polygons getMerged(CL::Polygons cpolys);

  static vector<Poly> getOffset(const Poly poly, double distance, 
				JoinType jtype=jmiter, double miterdist=1);
  static vector<Poly> getOffset(const vector<Poly> polys, double distance, 
				JoinType jtype=jmiter,double miterdist=1);
  static vector<Poly> getOffset(const ExPoly expoly, double distance, 
				JoinType jtype, double miterdist);
  static vector<Poly> getShrinkedCapped(const vector<Poly> polys, double distance, 
					JoinType jtype=jmiter,double miterdist=1);

  //vector< vector<Vector2d> > intersect(const Poly poly1, const Poly poly2) const;

  static Poly getPoly(const CL::Polygon cpoly, double z, double extrusionfactor);
  static vector<Poly> getPolys(const ExPoly expoly);
  static vector<Poly> getPolys(const vector<ExPoly> expolys);
  static vector<Poly> getPolys(const CL::Polygons cpoly, double z, double extrusionfactor);
  static vector<ExPoly> getExPolys(const CL::ExPolygons excpolys, double z,
				   double extrusionfactor);

  static vector<ExPoly> getExPolys(const vector<Poly> polys, 
				   double z, double extrusionfactor);
  static CL::ExPolygons getExClipperPolygons(const vector<Poly> polys);
  
  static CL::Polygon  getClipperPolygon (const Poly poly);
  static CL::Polygons getClipperPolygons(const vector<Poly> polys);
  static CL::Polygons getClipperPolygons(const ExPoly expoly);
  static CL::ExPolygons getClipperPolygons(const vector<ExPoly> expolys);

  static double Area(const Poly poly);
  static double Area(const vector<Poly> polys);

};
