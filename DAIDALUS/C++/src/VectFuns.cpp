/*
 * VectFuns.cpp
 * 
 * Copyright (c) 2011-2015 United States Government as represented by
 * the National Aeronautics and Space Administration.  No copyright
 * is claimed in the United States under Title 17, U.S.Code. All Other
 * Rights Reserved.
 */

#include "Util.h"
#include "Units.h"
#include "Vect2.h"
#include "Vect3.h"
#include "Velocity.h"
#include "VectFuns.h"
#include "Constants.h"
#include "format.h"
#include <cmath>
#include <float.h>


namespace larcfm {


bool VectFuns::LoS(const Vect3& so, const Vect3& si, double D, double H) {
	Vect3 s = so.Sub(si);
	return s.x*s.x + s.y*s.y < D*D && std::abs(s.z) < H;
}

bool VectFuns::rightOfLine(const Vect2& so, const Vect2& vo, const Vect2& si) {
	return si.Sub(so).dot(vo.PerpR()) >= 0;
}

bool VectFuns::collinear(const Vect3& p0, const Vect3& p1, const Vect3& p2) {
	Vect3 v01 = p0.Sub(p1);
	Vect3 v02 = p1.Sub(p2);
	return v01.parallel(v02);
}

bool VectFuns::collinear(const Vect2& p0, const Vect2& p1, const Vect2& p2) {
	// area of triangle is zero -- from Wolfram Mathworld
	return p0.x*(p1.y-p2.y) + p1.x*(p2.y-p0.y) + p2.x*(p0.y-p1.y) == 0;
}

Vect2 VectFuns::midPoint(const Vect2& p0, const Vect2& p1) {
	return p0.Add(p1).Scal(0.5);
}

Vect3 VectFuns::midPoint(const Vect3& p0, const Vect3& p1) {
	return p0.Add(p1).Scal(0.5);
}

// f should be between 0 and 1 to interpolate
Vect3 VectFuns::interpolate(const Vect3& v1, const Vect3& v2, double f) {
	Vect3 dv = v2.Sub(v1);
	return v1.Add(dv.Scal(f));
}

// f should be between 0 and 1 to interpolate
Velocity VectFuns::interpolateVelocity(const Velocity& v1, const Velocity& v2, double f) {
        double newtrk = v1.trk() + f*(v2.trk() - v1.trk());
        double newgs = v1.gs() + f*(v2.gs() - v1.gs());
        double newvs = v1.vs() + f*(v2.vs() - v1.vs());
        return Velocity::mkTrkGsVs(newtrk,newgs,newvs);
}



bool VectFuns::divergentHorizGt(const Vect2& s, const Vect2& vo, const Vect2& vi, double minRelSpeed) {
	Vect2 v = vo.Sub(vi);
	bool rtn = s.dot(v) > 0 && v.norm() > minRelSpeed;
	return rtn;
}

bool VectFuns::divergentHorizGt(const Vect3& s, const Vect3& vo, const Vect3& vi, double minRelSpeed) {
   return divergentHorizGt(s.vect2(), vo.vect2(), vi.vect2(), minRelSpeed);
}

bool VectFuns::divergent(const Vect2& so, const Vect2& vo, const Vect2& si, const Vect2& vi) {
	  return so.Sub(si).dot(vo.Sub(vi)) > 0;
}

bool VectFuns::divergent(const Vect3& so, const Velocity& vo, const Vect3& si, const Velocity& vi) {
	  return so.Sub(si).dot(vo.Sub(vi)) > 0;
}




// time of closest approach
double VectFuns::tau(const Vect3& s, const Vect3& vo, const Vect3& vi) {
	double rtn;
	Vect3 v = vo.Sub(vi);
	double nv = v.norm();
	if (Util::almost_equals(nv,0.0)) {
		rtn = std::numeric_limits<double>::max();        // pseudo infinity
	} else
		rtn = -s.dot(v)/(nv*nv);
	return rtn;
}// tau

double VectFuns::distAtTau(const Vect3& s, const Vect3& vo, const Vect3& vi, bool futureOnly) {
	double tau = VectFuns::tau(s,vo,vi);
	if (tau < 0 && futureOnly)
		return s.norm();                 // return distance now
	else {
		Vect3 v = vo.Sub(vi);
		Vect3 sAtTau = s.Add(v.Scal(tau));
		return sAtTau.norm();
	}
}

/**
 * Computes 2D intersection point of two lines, but also finds z component (projected by time from line 1)
 * @param s0 starting point of line 1
 * @param v0 direction vector for line 1
 * @param s1 starting point of line 2
 * @param v1 direction vector of line 2
 * @return Pair (2-dimensional point of intersection with 3D projection, relative time of intersection, relative to the so3)
 * If the lines are parallel, this returns the pair (0,NaN).
 */
std::pair<Vect3,double> VectFuns::intersection(const Vect3& so3, const Velocity& vo3, const Vect3& si3, const Velocity& vi3) {
	Vect2 so = so3.vect2();
	Vect2 vo = vo3.vect2();
	Vect2 si = si3.vect2();
	Vect2 vi = vi3.vect2();
	Vect2 ds = si.Sub(so);
	if (vo.det(vi) == 0) {
		//f.pln(" $$$ intersection: lines are parallel");
		return std::pair<Vect3,double>(Vect3::ZERO,  std::numeric_limits<double>::quiet_NaN());
	}
	double tt = ds.det(vi)/vo.det(vi);
	//f.pln(" $$$ intersection: tt = "+tt);
	return std::pair<Vect3,double>(so3.Add(vo3.Scal(tt)),tt);
}
// returns intersection point and time of intersection relative to position so1
// for time return value, it assumes that aircraft travels from so1 to so2 in dto seconds and from si1 to si2 in dti seconds
// a negative time indicates that the intersection occurred in the past (relative to directions of travel of so1)
std::pair<Vect3,double> VectFuns::intersection(const Vect3& so1, const Vect3& so2, double dto, const Vect3& si1, const Vect3& si2) {
	Velocity vo3 = Velocity::genVel(so1, so2, dto);
	Velocity vi3 = Velocity::genVel(si1, si2, dto);      // its ok to use any time here,  all times are relative to so
	return intersection(so1,vo3,si1,vi3);
}



/**
 * Computes 2D intersection point of two lines, but also finds z component (projected by time from line 1)
 * @param s0 starting point of line 1
 * @param v0 direction vector for line 1
 * @param s1 starting point of line 2
 * @param v1 direction vector of line 2
 * @return Pair (2-dimensional point of intersection with 3D projection, relative time of intersection, relative to the so3)
 * If the lines are parallel, this returns the pair (0,NaN).
 */
double  VectFuns::timeOfIntersection(const Vect3& so3, const Velocity& vo3, const Vect3& si3, const Velocity& vi3) {
	Vect2 so = so3.vect2();
	Vect2 vo = vo3.vect2();
	Vect2 si = si3.vect2();
	Vect2 vi = vi3.vect2();
	Vect2 ds = si.Sub(so);
	if (vo.det(vi) == 0) {
		//f.pln(" $$$ intersection: lines are parallel");
		return std::numeric_limits<double>::quiet_NaN();
	}
	double tt = ds.det(vi)/vo.det(vi);
	//f.pln(" $$$ intersection: tt = "+tt);
	return tt;
}


/**
 * Returns true if x is "behind" so , that is, x is within the region behind the perpendicular line to vo through so.
 */
bool VectFuns::behind(const Vect2& x, const Vect2& so, const Vect2& vo) {
	return Util::turnDelta(vo.track(), x.Sub(so).track()) > M_PI/2.0;
}

/**
 * Returns values indicating whether the ownship state will pass in front of or behind the intruder (from a horizontal perspective)
 * @param so ownship position
 * @param vo ownship velocity
 * @param si intruder position
 * @param vi intruder velocity
 * @return 1 if ownship will pass in front (or collide, from a horizontal sense), -1 if ownship will pass behind, 0 if divergent or parallel
 */
int VectFuns::passingDirection(const Vect3& so, const Velocity& vo, const Vect3& si, const Velocity& vi) {
	double toi = timeOfIntersection(so,vo,si,vi);
	double tii = timeOfIntersection(si,vi,so,vo); // these values may have opposite sign!
//fpln("toi="+toi);
//fpln("int = "+	intersection(so,vo,si,vi));
	if (ISNAN(toi) || toi < 0 || tii < 0) return 0;
	Vect3 so3 = so.linear(vo, toi);
	Vect3 si3 = si.linear(vi, toi);
//fpln("so3="+so3);
//fpln("si3="+si3);
	if (behind(so3.vect2(), si3.vect2(), vi.vect2())) return -1;
	return 1;
}


//	static int dirForBehind(Vect2 so, Vect2 vo, Vect2 si, Vect2 vi) {
//		if (divergent(so,vo,si,vi)) return 0;
//		double sdetvi = so.Sub(si).det(vi);
//		double toi = 0.0;
//		if (sdetvi != 0.0) toi = -vo.det(vi)/sdetvi;
//		Vect2 nso = so.AddScal(toi,vo);
//		Vect2 nsi = si.AddScal(toi,vi);
//		int ahead = Util::sign(nso.Sub(nsi).dot(vi)); // Are we ahead of intruder at crossing pt
//		int onRight = Util::sign(nsi.Sub(nso).det(vo)); // Are we ahead of intruder at crossing pt
//		return ahead*onRight;
//	}

int VectFuns::dirForBehind(const Vect2& so, const Vect2& vo, const Vect2& si, const Vect2& vi) {
	if (divergent(so,vo,si,vi)) return 0;
	return (rightOfLine(si, vi, so) ? -1 : 1);
}

int VectFuns::dirForBehind(const Vect3& so, const Velocity& vo, const Vect3& si, const Velocity& vi) {
     return dirForBehind(so.vect2(),vo.vect2(),si.vect2(),vi.vect2());
}





}
