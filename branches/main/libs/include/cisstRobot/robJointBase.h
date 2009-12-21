/*

  Author(s): Simon Leonard
  Created on: November 11 2009

  (C) Copyright 2008 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#ifndef _robJointBase_h
#define _robJointBase_h

#include <iostream>

#include <cisstRobot/robDefines.h>
#include <cisstRobot/robExport.h>

//! Joint types
/**
   These are used to identify the type of joints.
*/
enum robJointType{ 
  robJointHinge,       // revolute joint
  robJointSlider,      // prismatic joint
  robJointUniversal,   // universal joint
  robJointBallSocket   // ball and socket joint
};

//! Joint modes
/**
   Each joint can be active (powered by a motor) or passive
*/
enum robJointMode{    
  robJointActive,
  robJointPassive
};

class CISST_EXPORT robJointBase {

private:

  //! The type of the joint
  /**
     Determine if the joint is a hinge, slider, universal, ball and socket, etc.
  */
  robJointType type;
  
  //! The mode of the joint
  /**
     Determine if the joint is active (actuated) or passive
  */
  robJointMode mode;

  //! The position offset (added to each joint value)
  double qoffset;

  //! The minimum joint value
  double qmin;

  //! The maximum joint value
  double qmax;

  //! The absolute maximum force or torque
  double ftmax;
  
public:

  //! Default constructor
  robJointBase();

  //! Default destructor
  virtual ~robJointBase();

  //! Return the type of the joint
  /**
     \return The type of the joint (hinge, slider, universal, ball and socket)
  */
  robJointType JointType() const;

  //! Return the mode of the joint
  /**
     \return The mode of the joint (active or passive)
  */
  robJointMode JointMode() const;

  //! Return the joint position
  /**
     If supported, this returns the joint position.
     \return The joint angular or linear position (no unit)
  */
  virtual double GetJointPosition() const;

  //! Return the joint velocity
  /**
     If supported, this returns the joint velocity.
     \return The joint angular or linear velocity (no unit)
  */
  virtual double GetJointVelocity() const;

  //! Return the joint force or torque
  /**
     If supported, this returns the joint force or torque.
     \return The joint torque or force (no unit)
  */
  virtual double GetJointForceTorque() const;

  //! Set the joint position
  /**
     If supported, this sets the joint position. The position is NOT clipped to
     the position limits.
     \param q The new joint angular or linear position
  */
  virtual void SetJointPosition( double q );

  //! Set the joint velocity
  /**
     If supported, this sets the joint velocity. The velocity is NOT clipped to
     the velocity limit.
     \param qd The new joint angular or linear velocity.
  */
  virtual void SetJointVelocity( double qd );

  //! Set the force/torque
  /**
     If supported, this sets the force/torque. The new value is NOT clipped to
     the force/torque limit.
     \param ft The new force/torque
  */
  virtual void SetJointForceTorque( double ft );

  //! Return the offset position
  /**
     \return The offset position of the joint. This value has no unit.
  */
  double PositionOffset() const;

  //! Return the minimum position
  /**
     \return The minimum position of the joint. This value has no unit.
  */
  double PositionMin() const;
  
  //! Return the maximum position
  /**
     \return The maximum position of the joint. This value has no unit.
  */
  double PositionMax() const;
  
  //! Return the maximum force/torque
  /**
     \return The absolute value for the maximum force or torque that can be
             applied by the joint.
  */
  double ForceTorqueMax() const;
  
  //! Read from an input stream
  /**
     Use this method to configure the parameters of the joints from an input
     stream. The parameters are in the following order: type, mode, position 
     offset, min position, max position, max force/torque.
     \param is[in] The input stream
  */
  robError Read( std::istream& is );

  //! Read from an input stream
  /**
     Use this method to write the parameters of the joints to an output
     stream. This method can be overloaded for more specific joints.
     \param os[in] The output stream
  */
  robError Write( std::ostream& os ) const;

};

#endif
