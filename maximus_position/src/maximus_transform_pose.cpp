/*
#include <ros/ros.h>
#include <tf/transform_broadcaster.h>
#include <nav_msgs/Odometry.h>

int main(int argc, char** argv){
  ros::init(argc, argv, "odometry_publisher");

  ros::NodeHandle n;
  ros::Publisher odom_pub = n.advertise<nav_msgs::Odometry>("odom", 50);
  tf::TransformBroadcaster odom_broadcaster;

  double x = 0.0;
  double y = 0.0;
  double th = 0.0;

  double vx = 0.1;
  double vy = -0.1;
  double vth = 0.1;

  ros::Time current_time, last_time;
  current_time = ros::Time::now();
  last_time = ros::Time::now();

  ros::Rate r(10.0);
  while(n.ok()){
    current_time = ros::Time::now();

    //compute odometry in a typical way given the velocities of the robot
    double dt = (current_time - last_time).toSec();
    double delta_x = (vx * cos(th) - vy * sin(th)) * dt;
    double delta_y = (vx * sin(th) + vy * cos(th)) * dt;
    double delta_th = vth * dt;

    x += delta_x;
    y += delta_y;
    th += delta_th;

    //since all odometry is 6DOF we'll need a quaternion created from yaw
    geometry_msgs::Quaternion odom_quat = tf::createQuaternionMsgFromYaw(th);

    //first, we'll publish the transform over tf
    geometry_msgs::TransformStamped odom_trans;
    odom_trans.header.stamp = current_time;
    odom_trans.header.frame_id = "odom";
    odom_trans.child_frame_id = "base_link";

    odom_trans.transform.translation.x = x;
    odom_trans.transform.translation.y = y;
    odom_trans.transform.translation.z = 0.0;
    odom_trans.transform.rotation = odom_quat;

    //send the transform
    odom_broadcaster.sendTransform(odom_trans);

    //next, we'll publish the odometry message over ROS
    nav_msgs::Odometry odom;
    odom.header.stamp = current_time;
    odom.header.frame_id = "odom";

    //set the position
    odom.pose.pose.position.x = x;
    odom.pose.pose.position.y = y;
    odom.pose.pose.position.z = 0.0;
    odom.pose.pose.orientation = odom_quat;

    //set the velocity
    odom.child_frame_id = "base_link";
    odom.twist.twist.linear.x = vx;
    odom.twist.twist.linear.y = vy;
    odom.twist.twist.angular.z = vth;

    //publish the message
    odom_pub.publish(odom);

    last_time = current_time;
    r.sleep();
  }
}
*/

















#include "ros/ros.h"
#include "geometry_msgs/Pose.h"
#include "geometry_msgs/PoseStamped.h"
#include <maximus_position/AvrPose.h>
#include <maximus_position/AvrVel.h>
#include <tf/transform_broadcaster.h>
#include <nav_msgs/Odometry.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>                                         // for in-/output
#include <string.h>                                        // strcat

//Include system headers
#include <cstring>
#include <iostream>
#include <cstdio>

#include <sstream>
#include <math.h>
#include <vector>


class TransformPose {
  public:
    TransformPose();
    void publish_all(void);

     ros::Subscriber avrpose_sub_;
     ros::Publisher pose_pub;
     ros::Subscriber cmd_vel_sub_;
     ros::Publisher odom_pub;
     ros::Publisher avrvel_pub;


  private:
    void poseCallback(const maximus_position::AvrPose::ConstPtr & pose);
    void velCallback(const geometry_msgs::Twist::ConstPtr & vel);
    void rotate(double heading, double attitude, double bank, geometry_msgs::PoseStamped * pose);
     ros::NodeHandle nh;

     geometry_msgs::PoseStamped my_maximus_pose;
     nav_msgs::Odometry my_maximus_odom;
     maximus_position::AvrVel my_maximus_vel;

     tf::TransformBroadcaster odom_broadcaster;

};

TransformPose::TransformPose()
{

    // Joystick suscriber
    avrpose_sub_ = nh.subscribe < maximus_position::AvrPose > ("avrpose", 30, &TransformPose::poseCallback, this);
    // Set a Laser scan sensor for the robot
    pose_pub = nh.advertise < geometry_msgs::PoseStamped > ("avr_maximus_pose", 40);

    cmd_vel_sub_ = nh.subscribe < geometry_msgs::Twist > ("cmd_vel", 30, &TransformPose::velCallback, this);

    odom_pub = nh.advertise < nav_msgs::Odometry > ("odom", 50);

    avrvel_pub = nh.advertise < maximus_position::AvrVel > ("avrvel", 30);

    my_maximus_pose.header.frame_id = "/odom";
    my_maximus_pose.header.stamp = ros::Time::now();

    my_maximus_pose.pose.position.x = 0;
    my_maximus_pose.pose.position.y = 0;
    my_maximus_pose.pose.position.z = 0;
    my_maximus_pose.pose.orientation.x = 0;
    my_maximus_pose.pose.orientation.y = 0;
    my_maximus_pose.pose.orientation.z = 0;
    my_maximus_pose.pose.orientation.w = 0;

    my_maximus_odom.header.frame_id = "/odom";
    my_maximus_odom.child_frame_id = "/base_link";
    my_maximus_odom.header.stamp = ros::Time::now();

    my_maximus_odom.pose.pose.position.x = 0;
    my_maximus_odom.pose.pose.position.y = 0;
    my_maximus_odom.pose.pose.position.z = 0;
    my_maximus_odom.pose.pose.orientation.x = 0;
    my_maximus_odom.pose.pose.orientation.y = 0;
    my_maximus_odom.pose.pose.orientation.z = 0;
    my_maximus_odom.pose.pose.orientation.w = 0;

    my_maximus_odom.twist.twist.linear.x = 0;
    my_maximus_odom.twist.twist.linear.y = 0;
    my_maximus_odom.twist.twist.angular.z = 0;

    my_maximus_vel.vx = 0;
    my_maximus_vel.vy = 0;
    my_maximus_vel.vth = 0;


}

void TransformPose::rotate(double heading, double attitude, double bank, geometry_msgs::PoseStamped * pose)
{
    // Assuming the angles are in radians.
    double c1 = cos(heading / 2);
    double s1 = sin(heading / 2);
    double c2 = cos(attitude / 2);

    double s2 = sin(attitude / 2);
    double c3 = cos(bank / 2);
    double s3 = sin(bank / 2);
    double c1c2 = c1 * c2;
    double s1s2 = s1 * s2;

    pose->pose.orientation.w = c1c2 * c3 - s1s2 * s3;
    pose->pose.orientation.x = c1c2 * s3 + s1s2 * c3;
    pose->pose.orientation.y = s1 * c2 * c3 + c1 * s2 * s3;
    pose->pose.orientation.z = c1 * s2 * c3 - s1 * c2 * s3;
}


void TransformPose::poseCallback(const maximus_position::AvrPose::ConstPtr & pose)
{
    float tmp = 0.0;

    my_maximus_pose.header.stamp = ros::Time::now();

    my_maximus_pose.pose.position.x = pose->x;
    my_maximus_pose.pose.position.y = pose->y;
    my_maximus_pose.pose.position.z = 0;

    //TransformPose::rotate(0, (pose->theta), 0, &my_maximus_pose);
    geometry_msgs::Quaternion quat = tf::createQuaternionMsgFromYaw(pose->theta);
    my_maximus_pose.pose.orientation = quat;

    //first, we'll publish the transform over tf
    geometry_msgs::TransformStamped odom_trans;
    odom_trans.header.stamp = my_maximus_pose.header.stamp;
    odom_trans.header.frame_id = "/odom";
    odom_trans.child_frame_id = "/base_link";

    odom_trans.transform.translation.x = pose->x;
    odom_trans.transform.translation.y = pose->y;
    odom_trans.transform.translation.z = 0.0;
    odom_trans.transform.rotation = quat;

    //send the transform
    odom_broadcaster.sendTransform(odom_trans);


    my_maximus_odom.header.stamp = my_maximus_pose.header.stamp;
    my_maximus_odom.pose.pose.position.x = pose->x;
    my_maximus_odom.pose.pose.position.y = pose->y;
    my_maximus_odom.pose.pose.position.z = 0;
    my_maximus_odom.pose.pose.orientation = quat;

    my_maximus_odom.twist.twist.linear.x = pose->vx;
    my_maximus_odom.twist.twist.linear.y = pose->vy;
    my_maximus_odom.twist.twist.linear.z = 0;
    my_maximus_odom.twist.twist.angular.x = 0;
    my_maximus_odom.twist.twist.angular.y = 0;
    my_maximus_odom.twist.twist.angular.z = pose->vth;


    pose_pub.publish(my_maximus_pose);
    odom_pub.publish(my_maximus_odom);

}

void TransformPose::velCallback(const geometry_msgs::Twist::ConstPtr & vel)
{


    my_maximus_vel.vx = vel->linear.x;
    my_maximus_vel.vy = vel->linear.y;
    my_maximus_vel.vth = vel->angular.z;

    avrvel_pub.publish(my_maximus_vel);

}

void TransformPose::publish_all(void)
{

    // Publish the laser scan values
    TransformPose::pose_pub.publish(my_maximus_pose);
}


int main(int argc, char **argv)
{


    ros::init(argc, argv, "transformPose");
    TransformPose transform_Pose;
    // Refresh rate
    ros::Rate loop_rate(5);                                // 35 with bluetooth
    while (ros::ok()) {
        ros::spinOnce();
        loop_rate.sleep();
    }

    ros::Duration(2.0).sleep();

    ros::shutdown();
}

