/*
 * @Author: 王培荣
 * @Date: 2020-01-03 15:30:25
 * @LastEditTime : 2020-01-14 22:29:11
 * @LastEditors  : Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /catkin_ws/src/orbslam_semantic_nav_ros/aip-cpp/src/gesture_pub_node.cpp
 */ 
#include <ros/ros.h>
#include <std_msgs/String.h> 
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp> 
#include <stdlib.h>
#include <iostream> 
#include <sensor_msgs/Image.h>
#include <cv_bridge/cv_bridge.h> 
#include "slam_semantic_nav_ros/Gesture.h"
#include "base/base.h"
#include "body_analysis.h"

ros::Subscriber sub_img;
ros::Publisher pub_voice;
ros::Publisher pub_gesture;
std::string rospackage_path;

std::string appid = "18165604";
std::string AK = "EXXFhKHgb8uAGvo2yu9qAf4g";
std::string SK = "KTxGCvBRK6yvSAN1DHOh24Cl1Wk3G0jU";
std::string image_topic;

aip::Bodyanalysis client(appid, AK, SK);
slam_semantic_nav_ros::Gesture gesture_msg;

void img_callback(const sensor_msgs::ImageConstPtr &msgRGB)
{   
    try
    { 
        cv_bridge::CvImagePtr cv_image = cv_bridge::toCvCopy(msgRGB);
        Json::Value result;
        std::string image;
        std::string path = rospackage_path + "data/face.jpg";
        cv::imwrite(path.c_str(), cv_image->image);
        aip::get_file_content(path.c_str(), &image);

        
        // 调用手势识别
        result = client.gesture(image, aip::null);

        
        std::cout << result << std::endl <<
        "classname:  " << result["result"][0]["classname"] << std::endl << 
        "probability:  " << result["result"][0]["probability"] << std::endl << std::endl << std::endl;  
        cv::Mat img = cv_image->image;
        for(int i = 0; i < result["result_num"].asInt(); i++){
            if(result["result"][i]["classname"].asString() != "Face"){
                int left = result["result"][i]["left"].asInt();
                int top = result["result"][i]["top"].asInt();
                int height = result["result"][i]["height"].asInt();
                int width = result["result"][i]["width"].asInt();
                cv::putText(img, result["result"][i]["classname"].asString(), cv::Point2i(left, top), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 255));
                cv::rectangle(img, cv::Rect(left, top, width, height), cv::Scalar(255, 0, 0),1, cv::LINE_8,0);
                gesture_msg.classname = result["result"][i]["classname"].asString();
                gesture_msg.left = left; gesture_msg.top = top; gesture_msg.height = height; gesture_msg.width = width;
                pub_gesture.publish(gesture_msg);
                std_msgs::String voice_word;
                voice_word.data = result["result"][i]["classname"].asString();
                pub_voice.publish(voice_word); 
            } 
        }
        cv::namedWindow("gesture"); 
        cv::imshow("gesture", img);
        cv::waitKey(10); 
    }
    catch (cv_bridge::Exception& e)
    {
        ROS_ERROR("cv_bridge exception: %s", e.what());
        return;
    }
}
 

int main(int argc, char ** argv){
    ros::init(argc,argv,"gesture_pub");
    ros::NodeHandle nh;
    ros::start();
    if(argc != 2)
    {
        std::cerr << std::endl << "缺少参数" << std::endl << 
        "Usage: rosrun slam_semantic_nav_ros gesture_pub_node config_file_path" << std::endl;
        return 1;
    }
    cv::FileStorage fsSettings(argv[1], cv::FileStorage::READ);
    if(!fsSettings.isOpened()){
        std::cerr << std::endl <<
            std::endl << 
            "---------------------------------------------" << std::endl << 
            "---------------------------------------------" << std::endl << 
            "您的文件路径设置错误了，请在roslaunch中修改配置文件的路径！！！" << std::endl <<
            std::endl <<
            std::endl <<
            "祝您实验取得成功。" << std::endl << 
            "---------------------------------------------" << std::endl << 
            "---------------------------------------------" << std::endl;
            exit(1);
    }
    fsSettings["rospackage_path"] >> rospackage_path;

    fsSettings["baidu_gesture_appid"] >> appid;
    fsSettings["baidu_gesture_AK"] >> AK;
    fsSettings["baidu_gesture_SK"] >> SK;
    fsSettings["baidu_picture_topic"] >> image_topic;
    
    sub_img = nh.subscribe(image_topic, 1, img_callback);
    pub_voice = nh.advertise<std_msgs::String> ("/voiceWords", 1);
    pub_gesture = nh.advertise<slam_semantic_nav_ros::Gesture> ("/GestureSignal", 1);
    ros::spin(); 
    ros::shutdown(); 
    return 0;
}

