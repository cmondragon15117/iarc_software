/* I don't think the angle calculation works.
 * The coordinates of the roomba are not accurate.
 */

#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/PoseStamped.h>
#include <time.h>
#include <math.h>

geometry_msgs::PoseStamped poseMsg;
double x_dist = 0;																	//The distance traveled in the x direction
double z_dist = 0;																	//The distance traveled in the z direction

void copterCallback(const geometry_msgs::PoseStamped::ConstPtr& msg)
{
	poseMsg = *msg;
	ROS_INFO_STREAM("I heard: " << poseMsg.pose.position.x);
}

void calcCoords(double total_ang, double& dist){									//Calculates the coordinates of the roomba
	z_dist += (dist) * sin(total_ang);
	x_dist += (dist) * cos(total_ang);
	dist = 0;
}

int main(int argc, char **argv)
{
 	ros::init(argc, argv, "roomba");

 	ros::NodeHandle n;

 	ros::Publisher pubMov = n.advertise<geometry_msgs::Twist>("cmd_vel", 1);		//This is used to publish movement messages

 	ros::Subscriber sub = n.subscribe("copter", 1, copterCallback);					//Subscribe to the copters messages

 	ros::Rate loop_rate(10);														//This determines the length of time between loop iterations
 	geometry_msgs::Twist mov;														//Twist object to publish messages

 	bool forward = true;															//Boolean to determine the direction and switch faster

 	int last_20;																	//The last number on the 20 second interval
 	int last_5;																		//The last number on the 5 second interval
 	int last_1;																		//The last number of the 1 second interval

 	double sim_time;																//The time of the current simulation in seconds
 	double dir;																		//The direction to change on the 5 second intervals
 	double dist = 0;																//The total distance traveled since the last X,Z calculation
 	double total_ang = 0;															//The total angle turned so far

 	while (ros::ok())
 	{
   		//if (forward) mov.linear.x = 0.33;											//The speed is 0.33m/s
    	//else mov.linear.x = -0.33;
    	mov.linear.x = 0;

		sim_time = ros::Time::now().toSec();										//Set sim_time to the current time

		dist += mov.linear.x * (sim_time - (int)sim_time);							//Add the amount traveled in that second

		//if (last_1 != (int)sim_time){												//If a second has passed
		//	last_1 = sim_time;														//Reassign last_1 so you don't add it every iteration
		//	calcCoords(total_ang, dist);
		//}

		//ROS_INFO_STREAM("Time is " << sim_time);									//Print the time to the console
		//ROS_INFO_STREAM("Distance traveled is " << dist);							//Print the total angle turned
		//ROS_INFO_STREAM("Coordinates are (" << x_dist << "," << z_dist << ")");
		ROS_INFO_STREAM("Angle turned is " << total_ang);

		//if ((int)sim_time % 20 == 0 && (int)sim_time != last_20){					//Switch the direction every 20 seconds
		//	forward = !forward;
		//	last_20 = sim_time;
		//}

		if ((int)sim_time % 5 == 0 && (int)sim_time != last_5){						//Change direction every 5 seconds
			// //dir = (rand() % 41) - 20;												//Random number 0-20 (degrees in either direction)
			// dir = (rand() % 21);													//Random number 0-20 (in one direction)
			// //dir = (3.14159/180) * dir;  											//Convert the degrees to radians
			// mov.angular.z = dir;													//Change the direction based on that number
			last_5 = sim_time;
			// ROS_INFO_STREAM("Added " << dir);
			// total_ang += dir;														//Add the angle turned to total_ang
			mov.angular.z = 45;
		}
		else mov.angular.z = 0;														//Otherwise, don't change direction
 
		pubMov.publish(mov);														//Publish the movement message

		ros::spinOnce();
		loop_rate.sleep();															//Wait until the next iteration
	}

	return 0;
}

/*Degrees vs Radians
 *-20<x<20 vs 0<x<20
 *
 */