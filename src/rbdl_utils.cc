/*
 * RBDL - Rigid Body Dynamics Library
 * Copyright (c) 2011-2016 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
 *
 * Licensed under the zlib license. See LICENSE for more details.
 */

#include "rbdl/rbdl_utils.h"

#include "rbdl/rbdl_math.h"
#include "rbdl/Model.h"
#include "rbdl/Kinematics.h"

#include <sstream>
#include <iomanip>

namespace RigidBodyDynamics {

namespace Utils {

using namespace std;
using namespace Math;

string get_dof_name (const SpatialVectord &joint_dof) {
  if (joint_dof == SpatialVectord (1., 0., 0., 0., 0., 0.))
    return "RX";
  else if (joint_dof == SpatialVectord (0., 1., 0., 0., 0., 0.))
    return "RY";
  else if (joint_dof == SpatialVectord (0., 0., 1., 0., 0., 0.))
    return "RZ";
  else if (joint_dof == SpatialVectord (0., 0., 0., 1., 0., 0.))
    return "TX";
  else if (joint_dof == SpatialVectord (0., 0., 0., 0., 1., 0.))
    return "TY";
  else if (joint_dof == SpatialVectord (0., 0., 0., 0., 0., 1.))
    return "TZ";

  ostringstream dof_stream(ostringstream::out);
  dof_stream << "custom (" << joint_dof.transpose() << ")";
  return dof_stream.str();
}

string get_body_name (const RigidBodyDynamics::Model &model, unsigned int body_id) {
  if (model.mBodies[body_id].mIsVirtual) {
    // if there is not a unique child we do not know what to do...
    if (model.mu[body_id].size() != 1)
      return "";

    return get_body_name (model, model.mu[body_id][0]);
  }

  return model.GetBodyName(body_id);
}

RBDL_DLLAPI std::string GetModelDOFOverview (const Model &model, const ModelDatad &model_data) {
  stringstream result ("");

  unsigned int q_index = 0;
  for (unsigned int i = 1; i < model.mBodies.size(); i++) {
    if (model.mJoints[i].mDoFCount == 1) {
      result << setfill(' ') << setw(3) << q_index << ": " << get_body_name(model, i) << "_" << get_dof_name (model_data.S[i]) << endl;
      q_index++;
    } else {
      for (unsigned int j = 0; j < model.mJoints[i].mDoFCount; j++) {
        result << setfill(' ') << setw(3) << q_index << ": " << get_body_name(model, i) << "_" << get_dof_name (model.mJoints[i].mJointAxes[j]) << endl;
        q_index++;
      }
    }
  }

  return result.str();
}

std::string print_hierarchy (const RigidBodyDynamics::Model &model, const RigidBodyDynamics::ModelDatad &model_data,
                             unsigned int body_index = 0, int indent = 0) {
  stringstream result ("");

  for (int j = 0; j < indent; j++)
    result << "  ";

  result << get_body_name (model, body_index);

  std::cout << "Body " << body_index << " -> " << get_body_name (model, body_index) << std::endl;

  if (body_index > 0)
    result << " [ ";

  while (model.mBodies[body_index].mIsVirtual) {
    if (model.mu[body_index].size() == 0) {
      result << " end";
      break;
    } else if (model.mu[body_index].size() > 1) {
      cerr << endl << "Error: Cannot determine multi-dof joint as massless body with id " << body_index << " (name: " << model.GetBodyName(body_index) << ") has more than one child:" << endl;
      for (unsigned int ci = 0; ci < model.mu[body_index].size(); ci++) {
        cerr << "  id: " << model.mu[body_index][ci] << " name: " << model.GetBodyName(model.mu[body_index][ci]) << endl;
      }
      abort();
    }

    result << get_dof_name(model_data.S[body_index]) << ", ";

    body_index = model.mu[body_index][0];
  }

  if (body_index > 0)
    result << get_dof_name(model_data.S[body_index]) << " ]";
  result << endl;

  unsigned int child_index = 0;
  for (child_index = 0; child_index < model.mu[body_index].size(); child_index++) {
    result << print_hierarchy (model, model_data, model.mu[body_index][child_index], indent + 1);
  }

  // print fixed children
  for (unsigned int fbody_index = 0; fbody_index < model.mFixedBodies.size(); fbody_index++) {
    if (model.mFixedBodies[fbody_index].mMovableParent == body_index) {
      for (int j = 0; j < indent + 1; j++)
        result << "  ";

      result << model.GetBodyName(model.fixed_body_discriminator + fbody_index) << " [fixed]" << endl;
    }
  }


  return result.str();
}

RBDL_DLLAPI std::string GetModelHierarchy (const Model &model, const ModelDatad &model_data) {
  stringstream result ("");

  result << print_hierarchy (model, model_data);

  return result.str();
}

RBDL_DLLAPI std::string GetNamedBodyOriginsOverview (const Model &model, ModelDatad &model_data) {
  stringstream result ("");

  //VectorNd Q (VectorNd::Zero(model.dof_count));
  VectorNd Q (VectorNd::Zero(model.q_size));
  UpdateKinematicsCustom<double> (model, model_data, &Q, NULL, NULL);

  // Movable bodies
  for (unsigned int body_id = 0; body_id < model.mBodies.size(); body_id++) {
    std::string body_name = model.GetBodyName (body_id);

    if (body_name.size() == 0)
      continue;

    Vector3d position = CalcBodyToBaseCoordinates (model, model_data, Q, body_id, Vector3d (0., 0., 0.), false);

    result << body_name << "(" << body_id << "): " << position.transpose() << endl;
  }

  // Fixed bodies
  for (unsigned int fbody_index = 0; fbody_index < model.mFixedBodies.size(); fbody_index++) {
    //if (model.mFixedBodies[fbody_index].mMovableParent == body_index) {

	  unsigned int fixed_body_id = model.fixed_body_discriminator + fbody_index;
      std::string body_name = model.GetBodyName(fixed_body_id);
      //std::cout << model.mFixedBodies.size() << ", " << model.fixed_body_discriminator << ", " << fixed_body_id << std::endl;

      Vector3d position = CalcBodyToBaseCoordinates (model, model_data, Q, fixed_body_id, Vector3d (0., 0., 0.), false);

      result << body_name << "(" << fbody_index << "," <<fixed_body_id << "): " << position.transpose() << endl;
    //}
  }

  return result.str();
}

RBDL_DLLAPI void CalcCenterOfMass (
    const Model &model,
    ModelDatad &model_data,
    const Math::VectorNd &q, 
    const Math::VectorNd &qdot, 
    double &mass, 
    Math::Vector3d &com, 
    Math::Vector3d *com_velocity, 
    Math::Vector3d *angular_momentum,
    bool update_kinematics) {
  if (update_kinematics)
    UpdateKinematicsCustom<double> (model, model_data, &q, &qdot, NULL);

  for (size_t i = 1; i < model.mBodies.size(); i++) {
    model_data.Ic[i] = model_data.I[i];
    model_data.hc[i] = model_data.Ic[i].toMatrix() * model_data.v[i];
  }

  SpatialRigidBodyInertiad Itot (0., Vector3d (0., 0., 0.), Matrix3d::Zero(3,3));
  SpatialVectord htot (SpatialVectord::Zero());

  for (size_t i = model.mBodies.size() - 1; i > 0; i--) {
    unsigned int lambda = model.lambda[i];

    if (lambda != 0) {
      model_data.Ic[lambda] = model_data.Ic[lambda] + model_data.X_lambda[i].applyTranspose (model_data.Ic[i]);
      model_data.hc[lambda] = model_data.hc[lambda] + model_data.X_lambda[i].applyTranspose (model_data.hc[i]);
    } else {
      Itot = Itot + model_data.X_lambda[i].applyTranspose (model_data.Ic[i]);
      htot = htot + model_data.X_lambda[i].applyTranspose (model_data.hc[i]);
    }
  }

  mass = Itot.m;
  com = Itot.h / mass;
  LOG << "mass = " << mass << " com = " << com.transpose() << " htot = " << htot.transpose() << std::endl;

  if (com_velocity) 
    *com_velocity = Vector3d (htot[3] / mass, htot[4] / mass, htot[5] / mass);

  if (angular_momentum) {
    htot = Xtrans (com).applyAdjoint (htot);
    angular_momentum->set (htot[0], htot[1], htot[2]);
  }
}

void CalcCenterOfMass (
    Model &model,
    const Math::VectorNd &q,
    const Math::VectorNd &qdot,
    double &mass,
    Math::Vector3d &com,
    Math::Vector3d *com_velocity,
    Math::Vector3d *angular_momentum,
    bool update_kinematics) {

  CalcCenterOfMass (
      model,
      *model.getModelData(),
      q,
      qdot,
      mass,
      com,
      com_velocity,
      angular_momentum,
      update_kinematics);
}

RBDL_DLLAPI double CalcPotentialEnergy (
    Model &model,
    ModelDatad &model_data,
    const Math::VectorNd &q, 
    bool update_kinematics) {
  double mass;
  Vector3d com;
  CalcCenterOfMass (model, model_data, q, VectorNd::Zero (model.qdot_size), mass, com, NULL, NULL, update_kinematics);

  Vector3d g = - Vector3d (model.gravity[0], model.gravity[1], model.gravity[2]);
  LOG << "pot_energy: " << " mass = " << mass << " com = " << com.transpose() << std::endl;

  return mass * com.dot(g);
}

RBDL_DLLAPI double CalcKineticEnergy (
    Model &model, 
    ModelDatad &model_data,
    const Math::VectorNd &q, 
    const Math::VectorNd &qdot, 
    bool update_kinematics) {
  if (update_kinematics)
    UpdateKinematicsCustom<double> (model, model_data, &q, &qdot, NULL);

  double result = 0.;

  for (size_t i = 1; i < model.mBodies.size(); i++) {
    result += 0.5 * model_data.v[i].transpose() * (model_data.I[i] * model_data.v[i]);
  }
  return result;
}

}
}
