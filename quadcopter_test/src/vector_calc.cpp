#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include "vector_calc.h"
#include <geometry_msgs/PoseStamped.h>
#include <nav_msgs/Path.h>
#define RVEL 3
/* Consider vector (x,y,z) to be the dirn to go, publish twist as (x,y,z). This will make the qc go in the particular dirn. As for it to stop we check if the distance b/w it's current position and the original position is equal to the distance b/w the roomba and it's original position. or stop when it is a certain distance away. (using shortest distance formula).
*/
/* qcp = quad copter position
   rp = roomba position
*/
// What if I put the subscriber into the calculator function? 
vector::vector():
	nh(),
	loop_rate(5)
{
	state = 0;
	timecheck = 0;//begin = 0;//zero;//ros::Duration(0.0);
	checker = 1;
	initdec = 1;
	publ = nh.advertise<geometry_msgs::Twist>("cmd_vel", 5); // publishing the values to move the qc, currently publishes a velociy in a direction 
	pubviz = nh.advertise<nav_msgs::Path>("pathing",5); //publish path for rviz	
	subqcp = nh.subscribe("ground_truth_to_tf/pose", 5, &vector::callbackqcp, this); // subscribe to qc pose
	subrp = nh.subscribe("current_roomba/pose", 5, &vector::callbackrp, this); // subscribe to roomba pose, revise after Harsh chooses ros topic
	ROS_INFO_STREAM("Initialized Topics!");
}

vector::~vector()
{
}

void vector::callbackqcp(const geometry_msgs::PoseStamped::ConstPtr& posqcp)
{
	feedbackMsgqcp = *posqcp; // copy values from pointer into feedback for qcp
} 

void vector::callbackrp(const geometry_msgs::PoseStamped::ConstPtr& posrp)
{
	feedbackMsgrp = *posrp; // copy values from pointer into feedback for rp
	if(initdec == 1)
	{	
		feedbackMsgrp2.pose.position.x = 0;
		feedbackMsgrp2.pose.position.y = 0;
		feedbackMsgrp2.pose.position.z = 0;
		initdec = 0;
		
	}	
	if(checker==1) // constantly alters new and old position.once 1:new & 2:old. Next 1:old & 2:new.
	{
		feedbackMsgrp1 = *posrp;
		dxbydt = (feedbackMsgrp1.pose.position.x - feedbackMsgrp2.pose.position.x)/0.2;
		dybydt = (feedbackMsgrp1.pose.position.y - feedbackMsgrp2.pose.position.y)/0.2;
		//dxbydt = dxbydt/0.2; // dividing to get rate of change. 0.2 b/c the freq is 5. Hence time diff b/w two pos. is 0.2s.
		//dybydt = dybydt/0.2;
		checker = 0;
		
	}
	else
	{
		feedbackMsgrp2 = *posrp;
		dxbydt = feedbackMsgrp2.pose.position.x - feedbackMsgrp1.pose.position.x;
		dybydt = feedbackMsgrp2.pose.position.y - feedbackMsgrp1.pose.position.y;
		dxbydt = dxbydt/0.2; // dividing to get rate of change. 0.2 b/c the freq is 5. Hence time diff b/w two pos. is 0.2s.
		dybydt = dybydt/0.2;
		checker = 1;
		
	}
		
}

void vector::calculate()
{
		
		int i = 0;
		int npts = 20; //no. of pts for rviz path
		int timerun = 1; //no. of seconds to reach roomba once above it.		
		float x1 = 0;
		float y1 = 0;
		float z1 = 0;
		float z2 = 0;
		float vmag = 0;
		float vmag2 = 0;
		float constvelsq = 0; // constant velocity squared for the velocity you want it to go in
		float unitx1 = 0;
		float unity1 = 0;
		float unitz1 = 0;
		float unitx2 = 0;
		float unity2 = 0;
		float unitz2 = 0;
		float c = 0;
		float d = 0;
		float disp = 0; // vector variables to point direction
		float disp2 = 0;
		float dpts = 0;
		float rx = 0;
		float ry = 0;
		float rz = 0; // distance b/w 2 pts in rviz path, rviz coordinates
		x1 = feedbackMsgrp.pose.position.x - feedbackMsgqcp.pose.position.x; // calculating x,y,z values
		y1 = feedbackMsgrp.pose.position.y - feedbackMsgqcp.pose.position.y;				
		z1 = (feedbackMsgrp.pose.position.z+1) - feedbackMsgqcp.pose.position.z; // 1m above rp
		z2 = (feedbackMsgrp.pose.position.z+0.3) - feedbackMsgqcp.pose.position.z;
		vmag = sqrt( x1*x1 + y1*y1 + z1*z1); // magnitude of qc velocity
		vmag2 = sqrt( x1*x1 + y1*y1 + z2*z2);
		unitx1 = x1/vmag;
		unity1 = y1/vmag;
		unitz1 = z1/vmag;
		unitx2 = x1/vmag2; //  only z value changes in case 2
		unity2 = y1/vmag2;
		unitz2 = z2/vmag2;
		disp = sqrt( x1*x1 + y1*y1 + z1*z1 );
		disp2 = sqrt( x1*x1 + y1*y1 + z2*z2 );
		
		
		dpts=disp/npts; //dpts=disp/nps;
		nav_msgs::Path viz;
		viz.poses.resize(npts);
	


		geometry_msgs::PoseStamped path_point;

		for(i=0; i<npts; i++)
		{
			rx = (i+1)*x1/npts;
			ry = (i+1)*y1/npts;
			rz = (i+1)*z1/npts;
			path_point.pose.position.x = rx + feedbackMsgqcp.pose.position.x;
			path_point.pose.position.y = ry + feedbackMsgqcp.pose.position.y;
			path_point.pose.position.z = rz + feedbackMsgqcp.pose.position.z;
			path_point.pose.orientation.x = 0;
			path_point.pose.orientation.y = 0;
			path_point.pose.orientation.z = 0;
			path_point.pose.orientation.w = 1;
			viz.poses[i] = path_point;

		}

		viz.header.frame_id = "world";
		viz.header.stamp = ros::Time::now();
		if(state == 0)
		{
			if(disp>4)
			{
				constvelsq = 4;
			}
			else if(disp>1)
			{
				constvelsq = 1;
			}
			else if(disp>0.15)  // 0.8 best option for not shaking. New displacement bring it 0.2 height.
			{
				constvelsq = 0.25;
			}
			else
			{
				//constvelsq = 0;
				state = 1;
			}
			c = sqrt(constvelsq/((unitx1*unitx1)+(unity1*unity1)+(unitz1*unitz1)) );
			msg.linear.x = c*unitx1;
			msg.linear.y = c*unity1;
			msg.linear.z = c*unitz1;
		}
		else if(state == 1)
		{
			goto setting;
			back:
			ros::Duration seconds(1);
			if(ros::Time::now()-begin<seconds) // always sticks to this since begin gets redefined every time.
			{
				//constvelsq = 0.5; *
			}
			else
			{
				//constvelsq = 0; // check without this
				state = 2;
			}
			//d = sqrt( constvelsq/((unitx2*unitx2)+(unity2*unity2)+(unitz2*unitz2)) ); *
			//msg.linear.x = d*unitx2; *
			//msg.linear.y = d*unity2; *
			//msg.linear.z = d*unitz2; *
			msg.linear.x = x2;
			msg.linear.y = y2;
			msg.linear.z = z2;
				
			/*while(ros::Time::now()-begin<seconds) //stays for 1 second but doesnt go down.
			{
				ros::spinOnce();
				d = sqrt( 0.375/((unitx2*unitx2)+(unity2*unity2)+(unitz2*unitz2)) );
				msg.linear.x = d*unitx2;
				msg.linear.y = d*unity2;
				msg.linear.z = d*unitz2;
				publ.publish(msg);  // needs to publish else it'll stay in loop and not publish anything.
				pubviz.publish(viz);
			}
			state = 2; */
		}
		else if( state == 2)
		{
			if(disp>0.2)
			{			
				constvelsq = 0.25;
			}
			
			else  
			{
				constvelsq = 0;
				state = 3;
			}
			c = sqrt( constvelsq/((unitx1*unitx1)+(unity1*unity1)+(unitz1*unitz1)) );
			msg.linear.x = c*unitx1;
			msg.linear.y = c*unity1;
			msg.linear.z = c*unitz1;
		}
		else
		{
			state = 0;
			timecheck = 0;//begin = 0;//zero;//ros::Duration(0.0);
		}			
		ROS_INFO_STREAM("Distance b/w qc and rp:"<<vmag2);
		ROS_INFO_STREAM("Stage:"<<state);
		std::cout << "Distance b/w qc and rp:" <<vmag2<< std::endl;
		std::cout << "Stage:"<<state<< std::endl;
		std::cout <<x2<<y2<<z2<< std::endl;
		std::cout << "Velocity: ("<<msg.linear.x<<","<<msg.linear.y<<","<<msg.linear.z<<")"<< std::endl	;
		std::cout << "Vel Mag:"<<sqrt(msg.linear.x*msg.linear.x + msg.linear.y*msg.linear.y + msg.linear.z*msg.linear.z)<<std::endl;	
		publ.publish(msg);
		pubviz.publish(viz);
		
		if(state == 5)
		{	
			setting: if(timecheck == 0)//zero)//ros::Duration(0.0))
				 {
					 x2 = (feedbackMsgrp.pose.position.x + dxbydt*timerun) - feedbackMsgqcp.pose.position.x;
					 y2 = (feedbackMsgrp.pose.position.y + dybydt*timerun) - feedbackMsgqcp.pose.position.y;
					 begin = ros::Time::now();
					 timecheck = 1;
				 }
			 goto back;	
		}		
	

	
	/*if(vmag<RVEL) // if vmag is less then roomba velocity !! Find roomba vel (say rvel = 3)
	{
		while(vmag<RVEL)
		{
			x1 = x1*1.5;
			y1 = y1*1.5;
			z1 = z1*1.5;
			vmag = sqrt( x1*x1 + y1*y1 + z1*z1);
		}
	}
	msg.linear.x = x1;
	msg.linear.y = y1;
	msg.linear.z = z1;
	publ.publish(msg);
	ros::spinOnce();
        loop_rate.sleep();
	*/
}

void vector::run()
{	
	while(ros::ok())
	{
		calculate();
		ros::spinOnce();
        	loop_rate.sleep();
	}

}

	/* To keep a constan velocity say 3. 
	vmag = sqrt( x1*x1 + y1*y1 + z1*z1);
	unitx1 = x1/vmag;	gives unit vectors
	unity1 = y1/vmag;
	unitz1 = z1/vmag;
	c = sqrt( 9/((unitx1*unitx1)+(unity1*unity1)+(unitz1*unitz1)) ); find the constant to * to unit vectors
	msg.linear.x = c*unitx1;
	msg.linear.y = c*unity1;
	msg.linear.z = c*unitz1;
	*/

// Another METHOD: Fly right above the roomba then go down on to the roomba. 
/* Make sure that the velocity of qc is greater than the velocity of roomba so that it reaches the roomba and doesnt just fly behind it*/
/* Assumptions made: z vector will never be greater than 2, (3m max height, qc must be about 1m above the roomba). x,y vector wouldn't be more than 3 or 4 since the camera will choose the closest roomba.*/
/* NEW: take distance b/w original qcp and rp. Then take distance b/w new qcp and rp. Take the ratio : (new/original). This ratio has to be taken everytime. Multiply the twist velocity values (which is equal to the vector) by the ratio (twis.linear.x = x1.ratio). This makes the velocity lower and lower everytime. Include a threshold velocity so that it doesnt go slower than a certain amount or stop. 
*/
/* twist linear velocities as x1,y1,z1. Theoretically, with these values the qc should reach he roomba's position in exactly one second. However practically the qc may noot be able to go in this speed, so dividing it by a constant would make it more feasible. 
Eg: dividing it by 2 would make it reach the roomba's position in 2 seconds.
There is a chance of error with air drag, etc.
*/
/* Say we take a rate of 2 Hz. Then, in half a second the quad copter would go half the distance and would not reach the roomba's position. ( considering we take twist values equal to vector). Therefore, the quadcopter would never reach the roomba's position but it will keep getting closer
*/