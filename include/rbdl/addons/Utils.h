#ifndef _RBDL_UTILS_
#define _RBDL_UTILS_

#include <Eigen/Dense>
#include <exception>
#include <rbdl/Kinematics.h>
#include <rbdl/Model.h>

//Transform a spatial velocity vector to linear and angular vel
inline void spatialVelocity2vector(const RigidBodyDynamics::Math::SpatialVector v0,
                                   const RigidBodyDynamics::Math::Vector3d &pointOfForce,
                                   Eigen::Vector3d &linear_vel,
                                   Eigen::Vector3d &angular_vel){

  linear_vel = v0.block<3,1>(3, 0) - pointOfForce.cross(v0.block<3,1>(0, 0));
  angular_vel = v0.block<3,1>(0, 0);
}

inline  RigidBodyDynamics::Math::SpatialVector force2spatialVector(const Eigen::Vector3d &pointOfForce, const Eigen::Vector3d &force){
  RigidBodyDynamics::Math::SpatialVector result;

  Eigen::Vector3d n0;
  n0 = pointOfForce.cross(force);

  result(0) = n0(0);
  result(1) = n0(1);
  result(2) = n0(2);

  result(3) = force(0);
  result(4) = force(1);
  result(5) = force(2);

  return result;
}

inline Eigen::Isometry3d getBodyToBaseTransform(RigidBodyDynamics::Model &model, const Eigen::VectorXd &Q, const std::string &name,
                                                const Eigen::Vector3d &tip_position, bool update){

  assert(model.q_size == Q.rows());
  unsigned int id = model.GetBodyId(name.c_str());

  Eigen::Vector3d position = RigidBodyDynamics::CalcBodyToBaseCoordinates(model, Q, id, tip_position, update);
  Eigen::Matrix3d rotation = RigidBodyDynamics::CalcBodyWorldOrientation(model, Q, id, update).transpose();

  Eigen::Isometry3d temp;
  temp.setIdentity();
  temp = ( Eigen::AngleAxisd(rotation) );
  temp.translation() = position;
  return temp;

}

inline Eigen::Isometry3d getBodyToBaseTransform(RigidBodyDynamics::Model &model, const Eigen::VectorXd &Q, const std::string &name, bool update){

  return getBodyToBaseTransform(model, Q, name, Eigen::Vector3d::Zero(), update);
}

inline Eigen::Isometry3d getBodyToBaseTransform(RigidBodyDynamics::Model &model, const std::string &name){

  return getBodyToBaseTransform(model, Eigen::Vector3d::Zero(model.dof_count), name, Eigen::Vector3d::Zero(), false);
}

inline Eigen::Isometry3d getBodyToBaseTransform(RigidBodyDynamics::Model &model, const std::string &name,  const Eigen::Vector3d &tip_position){

  return getBodyToBaseTransform(model, Eigen::Vector3d::Zero(model.dof_count), name, tip_position, false);
}

inline Eigen::Vector3d getBodyLinearVelocity(RigidBodyDynamics::Model &model, const std::string &name, const Eigen::Vector3d &tip_position){
  unsigned int id = model.GetBodyId(name.c_str());
  return RigidBodyDynamics::CalcPointVelocity(model, Eigen::VectorXd::Zero(model.q_size),
                                              Eigen::VectorXd::Zero(model.qdot_size),
                                              id, tip_position, false);
}

inline Eigen::Vector3d getBodyAngularVelocity(RigidBodyDynamics::Model &model, const std::string &name){
  unsigned int id = model.GetBodyId(name.c_str());
  return RigidBodyDynamics::CalcPointAngularVelocity(model, Eigen::VectorXd::Zero(model.q_size),
                                                     Eigen::VectorXd::Zero(model.qdot_size),
                                                     id, Eigen::Vector3d::Zero(), false);
}

inline std::pair<Eigen::Vector3d, Eigen::Vector3d> getBodyVelocity(RigidBodyDynamics::Model &model, const std::string &name, const Eigen::Vector3d &tip_position){
  return std::make_pair(getBodyLinearVelocity(model, name, tip_position),
                        getBodyAngularVelocity(model, name));
}

inline Eigen::Vector3d getBodyLinearAcceleration(RigidBodyDynamics::Model &model, const std::string &name, Eigen::Vector3d &tip_position){
  unsigned int id = model.GetBodyId(name.c_str());
  return RigidBodyDynamics::CalcPointAcceleration(model, Eigen::VectorXd::Zero(model.q_size),
                                                  Eigen::VectorXd::Zero(model.qdot_size),
                                                  Eigen::VectorXd::Zero(model.q_size),
                                                  id, tip_position, false);
}

inline Eigen::Vector3d getBodyAngularAcceleration(RigidBodyDynamics::Model &model, const std::string &name, Eigen::Vector3d &tip_position){
  unsigned int id = model.GetBodyId(name.c_str());
  return RigidBodyDynamics::CalcPointAngularAcceleration(model, Eigen::VectorXd::Zero(model.q_size),
                                                         Eigen::VectorXd::Zero(model.qdot_size),
                                                         Eigen::VectorXd::Zero(model.q_size),
                                                         id, tip_position, false);
}

inline std::pair<Eigen::Vector3d, Eigen::Vector3d> getBodyAcceleration(RigidBodyDynamics::Model &model, const std::string &name, Eigen::Vector3d &tip_position){
  return std::make_pair(getBodyLinearAcceleration(model, name, tip_position),
                        getBodyAngularAcceleration(model, name, tip_position));
}

// This method does not update the internal state of the model, it querys directly the internal data structure
inline Eigen::Isometry3d getBodyTransform(RigidBodyDynamics::Model &model, const std::string &name){
  unsigned int id = model.GetBodyId(name.c_str());
  RigidBodyDynamics::Math::SpatialTransform tf;
  //Add forces (check if contact force is not in a fix link)
  if(id >= model.fixed_body_discriminator){
    tf = model.mFixedBodies[id - model.fixed_body_discriminator].mParentTransform;
  }
  else{
    tf = model.X_lambda[id];
  }

  Eigen::Matrix3d Et = tf.E.transpose();

  Eigen::Isometry3d temp;
  temp.setIdentity();
  temp = ( Eigen::AngleAxisd(Et) );
  temp.translation() = tf.r;
  return temp;

}

inline unsigned int getParentBodyId(RigidBodyDynamics::Model &model, const std::string &name){
  unsigned int id = model.GetBodyId(name.c_str());
  //Add forces (check if contact force is not in a fix link)
  if(id >= model.fixed_body_discriminator){
    return model.mFixedBodies[id - model.fixed_body_discriminator].mMovableParent;
  }
  else{
    return model.lambda[id];
  }
}


inline void createGeneralizedVector(const Eigen::Vector3d &floatingBasePosition, const Eigen::Quaterniond &floatingBaseOrientation,
                                    const Eigen::VectorXd &jointStates, Eigen::VectorXd &state){

  if(state.rows() != jointStates.rows() + 7){
    throw std::runtime_error("Missmatch between vectors creating generalized vector");
  }

  state.segment(0, 3) = floatingBasePosition;
  state(3) = floatingBaseOrientation.x();
  state(4) = floatingBaseOrientation.y();
  state(5) = floatingBaseOrientation.z();
  state(state.rows() - 1) = floatingBaseOrientation.w();

  state.segment(6, jointStates.rows()) = jointStates;
}

inline void createGeneralizedVelocityVector(const Eigen::Vector3d &floatingBaseLinearVelocity, const Eigen::Vector3d &floatingBaseAngularVelocity,
                                            const Eigen::VectorXd &jointStatesVelocity, Eigen::VectorXd &state){

  if(state.rows() != jointStatesVelocity.rows() + 6){
    throw std::runtime_error("Missmatch between vectors creating generalized vector");
  }

  state.segment(0, 3) = floatingBaseLinearVelocity;
  state.segment(3, 3) = floatingBaseAngularVelocity;
  state.segment(6, jointStatesVelocity.rows()) = jointStatesVelocity;
}

inline void createGeneralizedAccelerationVector(const Eigen::Vector3d &floatingBaseLinearAcceleration, const Eigen::Vector3d &floatingBaseAngularAcceleration,
                                                const Eigen::VectorXd &jointStatesAcceleration, Eigen::VectorXd &state){

  if(state.rows() != jointStatesAcceleration.rows() + 6){
    throw std::runtime_error("Missmatch between vectors creating generalized vector");
  }

  state.segment(0, 3) = floatingBaseLinearAcceleration;
  state.segment(3, 3) = floatingBaseAngularAcceleration;
  state.segment(6, jointStatesAcceleration.rows()) = jointStatesAcceleration;
}




#endif
