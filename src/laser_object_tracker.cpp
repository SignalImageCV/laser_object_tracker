/*********************************************************************
*
* BSD 3-Clause License
*
*  Copyright (c) 2019, Piotr Pokorski
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1. Redistributions of source code must retain the above copyright notice, this
*     list of conditions and the following disclaimer.
*
*  2. Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3. Neither the name of the copyright holder nor the names of its
*     contributors may be used to endorse or promote products derived from
*     this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
*  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
*  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

#include "laser_object_tracker/data_types/laser_scan_fragment.hpp"
#include "laser_object_tracker/segmentation/adaptive_breakpoint_detection.hpp"
#include "laser_object_tracker/segmentation/breakpoint_detection.hpp"
#include "laser_object_tracker/visualization/laser_object_tracker_visualization.hpp"

laser_object_tracker::data_types::LaserScanFragment::LaserScanFragmentFactory factory;
laser_object_tracker::data_types::LaserScanFragment fragment;

void laserScanCallback(const sensor_msgs::LaserScan::Ptr& laser_scan) {
    ROS_INFO("Received laser scan");
    fragment = factory.fromLaserScan(std::move(*laser_scan));

    ROS_INFO("Fragment has %d elements.", fragment.size());
}

int main(int ac, char** av) {
    ros::init(ac, av, "laser_object_detector");
    ros::NodeHandle pnh("~");

    ROS_INFO("Initializing segmentation");
    laser_object_tracker::segmentation::AdaptiveBreakpointDetection segmentation(0.7, 0.1);
    ROS_INFO("Initializing visualization");
    laser_object_tracker::visualization::LaserObjectTrackerVisualization visualization(pnh, "base_link");
    ROS_INFO("Initializing subscriber");
    ros::Subscriber subscriber_laser_scan = pnh.subscribe("/scan/front/filtered", 1, laserScanCallback);

    ros::Rate rate(10.0);
    ROS_INFO("Done initialization");
    while (ros::ok())
    {
        ros::spinOnce();

        if (!fragment.empty())
        {
            visualization.publishPointCloud(fragment);
            auto segments = segmentation.segment(fragment);
            ROS_INFO("Detected %d segments", segments.size());
            visualization.publishPointClouds(segments);
        }
        else
        {
            ROS_WARN("Received laser scan is empty");
        }

        rate.sleep();
    }

    return 0;
}
