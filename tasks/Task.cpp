#include "Task.hpp"

#include "frame_helper/FrameHelper.h"

using namespace camera;
using namespace camera_prosilica;
using namespace base::samples;
using namespace frame_helper;

Task::Task(std::string const& name): TaskBase(name)
{
}

Task::~Task()
{
}


//opens the camera 
bool Task::configureHook()
{
    if (! TaskBase::configureHook())
      return false;
  
    //convert camera_id to unsigned long
    unsigned long camera_id;
    std::stringstream ss(_camera_id.value());
    ss >> camera_id;
    
    if(_mode.value() == "Master")
	camera_access_mode = Master;
    else if (_mode.value() == "Monitor")
        camera_access_mode = Monitor;
    else if (_mode.value() ==  "MasterMulticast")
	camera_access_mode = MasterMulticast;
    else
    {
        RTT::log(RTT::Error) << "unsupported camera mode: " << _mode.value() << RTT::endlog();
      //  error(UNSUPPORTED_PARAMETER);
        return false;
    }  

    try
    {
        std::auto_ptr<camera::CamGigEProsilica> camera(new camera::CamGigEProsilica(_package_size));
        RTT::log(RTT::Info) << "open camera" << RTT::endlog();
        camera->open2(camera_id,camera_access_mode);
        cam_interface = camera.release();
    }
    catch (std::runtime_error e)
    {
        RTT::log(RTT::Error) << "failed to initialize camera: " << e.what() << RTT::endlog();
       // error(NO_CAMERA);
        return false;
    }
    
    //synchronize camera time with system time
 //   if(_synchronize_time_interval)
 //     cam_interface->synchronizeWithSystemTime(_synchronize_time_interval); 
    
    //set callback fcn
    cam_interface->setCallbackFcn(triggerFunction,this);
    return true;
}

void Task::triggerFunction(const void *p)
{
    ((RTT::TaskContext*)p)->getActivity()->trigger();
}

void Task::configureCamera()
{
    //do not change camera settings if we have no control
    if(camera_access_mode == Monitor)
        return;
    ::camera_base::Task::configureCamera();
}

//bool Task::startHook()
//{
//  if (! TaskBase::startHook())
//     return false;
//
//  return true;
//}
//
//void Task::updateHook()
//{
//    TaskBase::updateHook();
//}
//
//void Task::stopHook()
//{ 
//    TaskBase::stopHook();
//}
//
//void Task::cleanupHook()
//{
//    TaskBase::cleanupHook();
//}
//void Task::errorHook()
//{
//}


