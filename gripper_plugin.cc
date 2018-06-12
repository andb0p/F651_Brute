#ifndef _VELODYNE_PLUGIN_HH_
#define _VELODYNE_PLUGIN_HH_

#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/transport/transport.hh>
#include <gazebo/msgs/msgs.hh>

#include <thread>
#include "ros/ros.h"
#include "ros/callback_queue.h"
#include "ros/subscribe_options.h"
#include "std_msgs/Float32.h"

namespace gazebo
{
	// A plugin to control the gripper.	
	class GripperPlugin : public ModelPlugin {
	
	// Constructor	
	public: GripperPlugin() {}

	public: virtual void Load(physics::ModelPtr _model, sdf::ElementPtr _sdf)
	{
		this->model = _model; 																			// Store model pointer
		this->sdf = _sdf;																				// Store sdf pointer

		this->joint1 = this->model->GetJoint("joint_grip1");											// Store joint pointers
		this->joint2 = this->model->GetJoint("joint_grip2");
		this->joint3 = this->model->GetJoint("joint_grip3");
		
		this->pid1 = common::PID(10, 1, 1.1);															// Store PID pointers
		this->pid2 = common::PID(10, 1, 1.1);
		this->pid3 = common::PID(10, 1, 1.1);

		this->model->GetJointController()->SetPositionPID(this->joint1->GetScopedName(), this->pid1);	// Apply PIDs
		this->model->GetJointController()->SetPositionPID(this->joint2->GetScopedName(), this->pid2);
		this->model->GetJointController()->SetPositionPID(this->joint3->GetScopedName(), this->pid3);

		this->model->GetJointController()->SetPositionTarget(this->joint1->GetScopedName(), 0);			// Set initial positions
		this->model->GetJointController()->SetPositionTarget(this->joint2->GetScopedName(), 0);
		this->model->GetJointController()->SetPositionTarget(this->joint3->GetScopedName(), 0);
		
// ¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤ ROS-related ¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤
		if (!ros::isInitialized())																		// Initialize ros, if it has not already been initialized.
		{	
			int argc = 0;
			char **argv = NULL;
			ros::init(argc, argv, "gazebo_client",
			ros::init_options::NoSigintHandler);
		}

		this->rosNode.reset(new ros::NodeHandle("gazebo_client"));										// Create a ROS node

		ros::SubscribeOptions so1 =																		// Create a ROS topic
			ros::SubscribeOptions::create<std_msgs::Float32>(
			"/" + this->model->GetName() + "/grip_rad", 1,
			boost::bind(&GripperPlugin::OnRosMsg_grip, this, _1),
			ros::VoidPtr(), &this->rosQueue1);

		this->rosSub1 = this->rosNode->subscribe(so1);													// Subscribe to the ROS topic.

		this->rosQueueThread1 =																			// Spin up the queue helper thread.
		  std::thread(std::bind(&GripperPlugin::QueueThread, this));
// ¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤ /ROS-related ¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤
	}

	// -------GAZEBO-related-------
	private: physics::JointPtr joint1; 						// Pointer to joint1.
	private: physics::JointPtr joint2; 						// Pointer to joint2.
	private: physics::JointPtr joint3; 						// Pointer to joint3.

	private: common::PID pid1;								// A PID controller for joint1.
	private: common::PID pid2;								// A PID controller for joint2.
	private: common::PID pid3;								// A PID controller for joint3.
	
	private: physics::ModelPtr model; 						// Pointer to the model.
	private: sdf::ElementPtr sdf;
	
	// -------ROS-related-------
	private: std::unique_ptr<ros::NodeHandle> rosNode; 		// A node use for ROS transport
	private: ros::Subscriber rosSub1;						// A ROS subscriber
	private: ros::CallbackQueue rosQueue1;					// A ROS callbackqueue that helps process messages
	private: std::thread rosQueueThread1;					// A thread the keeps running the rosQueue

	// ROS callback function	
	public: void OnRosMsg_grip(const std_msgs::Float32ConstPtr &_msg)
	{
		this->model->GetJointController()->SetPositionTarget(this->joint1->GetScopedName(), _msg->data);
		this->model->GetJointController()->SetPositionTarget(this->joint2->GetScopedName(), _msg->data);
		this->model->GetJointController()->SetPositionTarget(this->joint3->GetScopedName(), _msg->data);
	}

	// ROS helper function that processes messages
	private: void QueueThread()
	{
		static const double timeout = 0.01;
		while (this->rosNode->ok())
		{
			this->rosQueue1.callAvailable(ros::WallDuration(timeout));
		}
	}

  };

  // Tell Gazebo about this plugin, so that Gazebo can call Load on this plugin.
  GZ_REGISTER_MODEL_PLUGIN(GripperPlugin)
}
#endif
