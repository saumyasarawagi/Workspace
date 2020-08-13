#include <ros/ros.h>
#include <geometry_msgs/PoseStamped.h>
#include <mavros_msgs/CommandBool.h>
#include <mavros_msgs/SetMode.h>
#include <mavros_msgs/State.h>
#include <mavros_msgs/CommandTOL.h>
#include <std_msgs/Float64.h>
#include <geometry_msgs/TwistStamped.h>
#include <sensor_msgs/Imu.h>
#include <geometry_msgs/AccelWithCovarianceStamped.h>
#include <geometry_msgs/Wrench.h>
#include <gazebo_msgs/ApplyBodyWrench.h>
#include <geometry_msgs/Vector3.h>
#include <geometry_msgs/Point.h>
#include <geometry_msgs/Twist.h>
#include <string>
#include <ros/time.h>


mavros_msgs::State current_state;
geometry_msgs::PoseStamped current_pose;
geometry_msgs::TwistStamped current_velocity;
sensor_msgs::Imu imu_data;
geometry_msgs::AccelWithCovarianceStamped current_accel;
geometry_msgs::Wrench wrench_body;
gazebo_msgs::ApplyBodyWrench impulse;

void state_cb(const mavros_msgs::State::ConstPtr& msg){
    current_state = *msg;
    bool connected = current_state.connected;
    bool armed = current_state.armed;
}

void pose_cb(const geometry_msgs::PoseStamped::ConstPtr& msg){
    current_pose = *msg;
    //ROS_INFO("x: %f y: %f z: %f", current_pose.pose.position.x, current_pose.pose.position.y, current_pose.pose.position.z);  
}

void velocity_cb(const geometry_msgs::TwistStamped::ConstPtr& msg){
    current_velocity =*msg;
    //ROS_INFO("u: %f v: %f w: %f p: %f q: %f r: %f", current_velocity.twist.linear.x, current_velocity.twist.linear.y, current_velocity.twist.linear.z, current_velocity.twist.angular.x, current_velocity.twist.angular.y, current_velocity.twist.angular.z);
}

void accel_cb(const geometry_msgs::AccelWithCovarianceStamped::ConstPtr& msg){
    current_accel =*msg;
     ROS_INFO("check");
    //ROS_INFO("al_x: %f al_y: %f al_z: %f", current_accel.accel.accel.linear.x, current_accel.accel.accel.linear.y, current_accel.accel.accel.linear.z);
}

void imu_cb(const sensor_msgs::Imu::ConstPtr& msg){
    imu_data =*msg;
    ROS_INFO("a_x: %f a_y: %f a_z: %f", imu_data.linear_acceleration.x, imu_data.linear_acceleration.y, imu_data.linear_acceleration.z);
}


int main(int argc, char **argv)
{
    ros::init(argc, argv, "impulse_node");
    ros::NodeHandle nh;

    ros::Subscriber state_sub = nh.subscribe<mavros_msgs::State>("mavros/state", 10, state_cb);
    ros::Subscriber currentPos = nh.subscribe<geometry_msgs::PoseStamped>("mavros/local_position/pose",10, pose_cb);
    ros::Subscriber currentVelocity = nh.subscribe<geometry_msgs::TwistStamped>("mavros/local_position/velocity_local",10, velocity_cb);
    ros::Subscriber currentAccel = nh.subscribe<geometry_msgs::AccelWithCovarianceStamped>("mavros/local_position/accel",10, accel_cb);
    ros::Subscriber currentImu = nh.subscribe<sensor_msgs::Imu>("mavros/imu/data",10, imu_cb);
    ros::ServiceClient set_mode_client = nh.serviceClient<mavros_msgs::SetMode>("mavros/set_mode");
    ros::ServiceClient send_impulse = nh.serviceClient<gazebo_msgs::ApplyBodyWrench>("gazebo/apply_body_wrench");

    //the setpoint publishing rate MUST be faster than 2Hz
    ros::Rate rate(20.0);

    // wait for FCU connection
    while(ros::ok() && !current_state.connected){
        ros::spinOnce();
        rate.sleep();
    }

    wrench_body.force.x=-10;
    wrench_body.force.y=0;
    wrench_body.force.z=0;
    wrench_body.torque.x=0;
    wrench_body.torque.y-0;
    wrench_body.torque.z=0;
    impulse.request.body_name = "test::base_link";
    impulse.request.wrench = wrench_body;
    impulse.request.duration = ros::Duration(0.001);
    impulse.request.reference_frame = "test::base_link";
    impulse.request.reference_point.z = -0.128;

    //send a few setpoints before starting
    for(int i = 100; ros::ok() && i > 0; --i){
        //send_impulse.call(impulse);
        ros::spinOnce();
        rate.sleep();
    }

    mavros_msgs::SetMode offb_set_mode;
    offb_set_mode.request.custom_mode = "OFFBOARD";
    ros::Time last_request = ros::Time::now();
    ros::Time last_request_impulse = ros::Time::now();

    while(ros::ok()){
        if( current_state.mode != "OFFBOARD" &&
            (ros::Time::now() - last_request > ros::Duration(5.0))){
            if( set_mode_client.call(offb_set_mode) &&
                offb_set_mode.response.mode_sent){
                //ROS_INFO("Offboard enabled");
            }
            last_request = ros::Time::now();
        } 
        
       if(ros::Time::now() - last_request_impulse > ros::Duration(0.005)){
		send_impulse.call(impulse);
                ROS_INFO("Impulse sent %d", impulse.response.success);
		last_request_impulse = ros::Time::now();}

  }  return 0;
}
