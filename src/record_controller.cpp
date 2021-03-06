#include "record_controller.h"

using std::vector;
using std::string;

namespace img_proc
{
RecordController::RecordController(string prefix, vector<string>& joints)
: record_server_(nh_, action_name, boost::bind(&RecordController::fullRecord, this, _1), false)
, record_seq_(0)
{
	for(int i = 0; i < joints.size(); i++)
	{
		client_ = nh_.serviceClient<std_srvs::Trigger>("record_feature");
		string topic = prefix + joints[i] + topic_suffix;
		joint_pub_.push_back(nh_.advertise<std_msgs::Float64>(topic, 2));
		record_server_.start();
	}
}

void RecordController::fullRecord(const img_proc::RecordGoalConstPtr &goal)
{
	feedback_.percent_finished = 0;
	record_seq_ = 0;
	total_ = pow(goal->num_per_joint, joint_pub_.size());
	if((result_.success = recursiveFullRecord(goal->num_per_joint, 0)))
	{
		record_server_.setSucceeded(result_);
	}
}

bool RecordController::recursiveFullRecord(int numPerJoint, int index)
{
	ros::Publisher publisher = joint_pub_[index];
	for(int i = 0; i < numPerJoint; i++)
	{
		if (record_server_.isPreemptRequested() || !ros::ok())
		{
			ROS_INFO("%s: Preempted", action_name.c_str());
			return false;
		}
		std_msgs::Float64 msg;
		msg.data = pi * (2. * i / numPerJoint - 1);
		publisher.publish(msg);
		if(index < joint_pub_.size() - 1)
		{
			if(!recursiveFullRecord(numPerJoint, index + 1))
			{
				return false;
			}
		}
		else
		{
			std_srvs::Trigger srv;
			if(!client_.call(srv))
			{
				ROS_ERROR("Failed to call service record_feature");
				return false;
			}
			else
			{
				feedback_.percent_finished = ++record_seq_ / total_ * 100;
				record_server_.publishFeedback(feedback_);
			}
		}
	}
	return true;
}
}

int main(int argc, char **argv)
{
	ros::init(argc, argv, "record_controller");
	vector<string> joints;
	for(int i = 2; i < argc; i++)
	{
		joints.push_back(argv[i]);
	}
	img_proc::RecordController rc(argv[1], joints);
	ros::spin();
	return 0;
}
