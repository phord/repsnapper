/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2011-12  martin.dieringer@gmx.de

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

#include <vector>
//#include <list>

#include "stdafx.h"
#include "settings.h"


class PLine; // see below



// 3D printline for making GCode
class PLine3
{
 public: 
  PLine3(PLine pline, double z);
  ~PLine3(){};

  Vector3d from, to;
  double speed;
  double extrusionfactor; 
  double absolute_extrusion; // additional absolute extrusion /mm (retract/repush f.e.)
  Vector3d arcIJK;  // if is an arc
  short arc; // -1: ccw arc, 1: cw arc, 0: not an arc
  double arcangle;

  int getCommands(Vector3d &lastpos, vector<Command> &commands, double extrusion,
		  double minspeed, double maxspeed, double movespeed,
		  double maxEspeed) const;
  // not used
  string GCode(Vector3d &lastpos, double &lastE, double feedrate, 
	       double minspeed, double maxspeed, double movespeed, 
	       bool relativeE) const;
  double length() const;
  double time() const;
  string info() const;
};


// single 2D printline
class PLine
{
  friend class PLine3;
  friend class Printlines;
  PLine(const Vector2d from, const Vector2d to, double speed, 
	double feedrate);
  PLine(const Vector2d from, const Vector2d to, double speed, 
	double feedrate, short arc, Vector2d arccenter, double angle);
  Vector2d from, to;
  double speed; // mm/min (!)
  double feedrate; // relative extrusion feedrate 
  double absolute_feed; // additional absolute feed /mm (retract/repush f.e.)
  double angle; // angle of line
  Vector2d arccenter;
  short arc;  // -1: ccw arc, 1: cw arc, 0: not an arcx

  void addAbsoluteExtrusionAmount(double amount);
  double calcangle() const;
  double calcangle(const PLine rhs) const;
  double lengthSq() const;
  double length() const;
  double time() const;  // time in minutes
  PLine3 getPrintline(double z) const;
  bool is_noop() const;
  string info() const;
 public: 
  ~PLine(){};
};


// a bunch of printlines: lines with feedrate
// optimize for corners etc.
class Printlines
{
    
  double z;
  double Zoffset; // global offset for generated PLine3s, always added at setZ()

  string name;

  void addPoly(vector<PLine> &lines, const Poly poly, int startindex=0, 
	       double speed=1, double movespeed=1) const;
  void addLine(vector<PLine> &lines, Vector2d from, Vector2d to, 
	       double speed=1, double movespeed=1, double feedrate=1.0) const;

 public:
  Printlines(double z_offset=0);
  ~Printlines(){};
  
  void setName(string s){name=s;};

  Vector2d lastPoint() const;

  void makeLines(const vector<Poly> polys, 
		 bool displace_startpoint, 
		 const Settings::SlicingSettings &slicing,
		 const Settings::HardwareSettings &hardware,
		 Vector2d &startPoint,
		 vector<PLine> &lines);
    
  void optimize(const Settings::HardwareSettings &hardware,
		const Settings::SlicingSettings &slicing,
		double slowdowntime,
		vector<PLine> &lines);

  uint makeArcs(const Settings::SlicingSettings &slicing,
		 vector<PLine> &lines) const;
  uint makeIntoArc(guint fromind, guint toind, vector<PLine> &lines) const;

  uint makeAntioozeRetraction(const Settings::SlicingSettings &slicing,
			      vector<PLine> &lines) const;

  // slow down to total time needed (cooling)
  double slowdownTo(double totalseconds, vector<PLine> &lines) ; // returns slowdownfactor
  void setSpeedFactor(double speedfactor, vector<PLine> &lines) const;

  // keep movements inside polys when possible (against stringing)
  void clipMovements(vector<Poly> *polys, vector<PLine> &lines,
		     double maxerr=0.01) const;

  void getLines(const vector<PLine> lines,
		vector<Vector2d> &linespoints) const;
  void getLines(const vector<PLine> lines,
		vector<Vector3d> &linespoints) const;
  void getLines(const vector<PLine> lines,
		vector<PLine3> &plines) const;

  double totalLength(const vector<PLine> lines) const;
  double totalSeconds(const vector<PLine> lines) const;
  double totalSecondsExtruding(const vector<PLine> lines) const;

  // every added poly will set this
  void setZ(double z) {this->z = z + Zoffset;};
  double getZ() const {return z;};

  double getSlowdownFactor() const {return slowdownfactor;};

  /* string GCode(Vector3d &lastpos, double &E, double feedrate,  */
  /* 	       double minspeed, double maxspeed, double movespeed,  */
  /* 	       bool relativeE) const; */
  string info() const;


 private:
  void optimizeLinedistances(double maxdist, vector<PLine> &lines) const;
  void mergelines(PLine &l1, PLine &l2, double maxdist) const;
  double distance(const Vector2d p, const PLine l2) const;
  void optimizeCorners(double linewidth, double linewidthratio, double optratio,
		       vector<PLine> &lines) const;
  bool capCorner(PLine &l1, PLine &l2, double linewidth, double linewidthratio, 
		 double optratio) const;

  uint divideline(uint lineindex, const vector<Vector2d> points,
		  vector<PLine> &lines) const;
  uint divideline(uint lineindex, const double t, vector<PLine> &lines) const;


  Vector2d arcCenter(const PLine l1, const PLine l2, 
		     double maxerr) const;

  double slowdownfactor; // result of slowdown/setspeedfactor. not used here.
  
  /* string GCode(PLine l, Vector3d &lastpos, double &E, double feedrate,  */
  /* 	       double minspeed, double maxspeed, double movespeed,  */
  /* 	       bool relativeE) const; */

  typedef vector<PLine>::const_iterator lineCIt ;
  typedef vector<PLine>::iterator lineIt ;
  //list<line>::iterator lIt;
};
