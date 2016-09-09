/*
 * Copyright (c) 2015-2016 United States Government as represented by
 * the National Aeronautics and Space Administration.  No copyright
 * is claimed in the United States under Title 17, U.S.Code. All Other
 * Rights Reserved.
 */
package gov.nasa.larcfm.ACCoRD;

import gov.nasa.larcfm.Util.Pair;
import gov.nasa.larcfm.Util.Position;
import gov.nasa.larcfm.Util.ProjectedKinematics;
import gov.nasa.larcfm.Util.Vect3;
import gov.nasa.larcfm.Util.Velocity;

public class KinematicGsBands extends KinematicRealBands {

	private double horizontal_accel_; // Horizontal acceleration

	public KinematicGsBands(KinematicBandsParameters parameters) {
		super(parameters.getMinGroundSpeed(),
				parameters.getMaxGroundSpeed(),
				parameters.getGroundSpeedStep(),
				parameters.isEnabledRecoveryGroundSpeedBands());
		horizontal_accel_ = parameters.getHorizontalAcceleration();
	}

	public KinematicGsBands(KinematicGsBands b) {
		super(b);
		horizontal_accel_ = b.horizontal_accel_;
	}

	public boolean instantaneous_bands() {
		return horizontal_accel_ == 0;
	}

	public double get_horizontal_accel() {
		return horizontal_accel_;
	}

	public void set_horizontal_accel(double val) {
		if (val != horizontal_accel_) {
			horizontal_accel_ = val;
			reset();
		}
	}

	public double own_val(TrafficState ownship) {
		return ownship.groundSpeed();
	}

	public double time_step(TrafficState ownship) {
		return get_step()/horizontal_accel_;
	}

	public Pair<Vect3, Velocity> trajectory(TrafficState ownship, double time, boolean dir) {    
		Pair<Position,Velocity> posvel;
		if (instantaneous_bands()) {
			double gs = ownship.getVelocity().gs()+(dir?1:-1)*j_step_*get_step(); 
			posvel = Pair.make(ownship.getPosition(),ownship.getVelocity().mkGs(gs));
		} else {
			posvel = ProjectedKinematics.gsAccel(ownship.getPosition(),ownship.getVelocity(),time,
					(dir?1:-1)*horizontal_accel_);
		}
		return Pair.make(ownship.pos_to_s(posvel.first),ownship.vel_to_v(posvel.first,posvel.second));
	}

}
