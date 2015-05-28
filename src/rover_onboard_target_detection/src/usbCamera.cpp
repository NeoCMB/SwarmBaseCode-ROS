/*
 * Author: Karl A. Stolleis
 * Maintainer: Karl A. Stolleis
 * Email: karl.a.stolleis@nasa.gov; kurt.leucht@nasa.gov
 * NASA Center: Kennedy Space Center
 * Mail Stop: NE-C1
 * 
 * Project Name: Swarmie Robotics NASA Center Innovation Fund
 * Principal Investigator: Cheryle Mako
 * Email: cheryle.l.mako@nasa.gov
 * 
 * Date Created: June 6, 2014
 * Safety Critical: NO
 * NASA Software Classification: D
 * 
 * This software is copyright the National Aeronautics and Space Administration (NASA)
 * and is distributed under the GNU LGPL license.  All rights reserved.
 * Permission to use, copy, modify and distribute this software is granted under
 * the LGPL and there is no implied warranty for this software.  This software is provided
 * "as is" and NASA or the authors are not responsible for indirect or direct damage
 * to any user of the software.  The authors and NASA are under no obligation to provide
 * maintenence, updates, support or modifications to the software.
 * 
 * Revision Log:
 *      
 */

#include "usbCamera.h"

enum Default {
        DEFAULT_CAMERA_INDEX = 0,
        DEFAULT_FPS = 1
    };

USBCamera::USBCamera(int frameRate, int cameraIndex, string hostname):
    it(nh),
    videoStream(cameraIndex) {
    
        nh.param<int>("cameraIndex", cameraIndex, DEFAULT_CAMERA_INDEX);
        nh.param<int>("fps", fps, frameRate);
        
        if (not videoStream.isOpened()) {
            ROS_ERROR_STREAM("Failed to open camera device!");
            ros::shutdown();
        }
        
        ros::Duration period = ros::Duration(1. / fps);

        rawImgPublish = it.advertise((hostname + "/camera/image"), 2);

        rosImage = boost::make_shared<cv_bridge::CvImage>();
        rosImage->encoding = sensor_msgs::image_encodings::MONO8;

        timer = nh.createTimer(period, &USBCamera::capture, this);
}

void USBCamera::capture(const ros::TimerEvent& te) {
        videoStream >> cvImageColor;
        cv::cvtColor(cvImageColor, cvImageBW, cv::COLOR_BGR2GRAY);  
        
        cv::resize(cvImageBW, cvImageLR, cv::Size(320,240), cv::INTER_LINEAR);
        
        rosImage->image = cvImageLR;
        if (not rosImage->image.empty()) {
            rosImage->header.stamp = ros::Time::now();
            rawImgPublish.publish(rosImage->toImageMsg());
        }
    }

USBCamera::~USBCamera(){
    videoStream.release();
}

