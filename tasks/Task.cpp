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
        error(UNSUPPORTED_PARAMETER);
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
        error(NO_CAMERA);
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

//set camera settings
void Task::setCameraSettings()
{
    //do not change camera settings if we have no control
    if(camera_access_mode == Monitor)
        return;

    //sets binning to 1 otherwise high resolution can not be set
    if(cam_interface->isAttribAvail(int_attrib::BinningX))
    {
        cam_interface->setAttrib(int_attrib::BinningX,1);
        cam_interface->setAttrib(int_attrib::BinningY,1);
    }
    else
        RTT::log(RTT::Warning) << "Binning is not supported by the camera" << RTT::endlog();

    //setting resolution and color mode
    try
    {
        cam_interface->setFrameSettings(*camera_frame);
    }
    catch(std::runtime_error e)
    {
        RTT::log(RTT::Error) << "failed to configure camera: " << e.what() << RTT::endlog();
        error(CONFIGURE_ERROR);
        return;
    }

    //setting Region
    if(cam_interface->isAttribAvail(int_attrib::RegionX))
    {
        cam_interface->setAttrib(camera::int_attrib::RegionX,_region_x);
        cam_interface->setAttrib(camera::int_attrib::RegionY,_region_y);
    }
    else
        RTT::log(RTT::Warning) << "Region is not supported by the camera" << RTT::endlog();

    //setting Binning
    if(cam_interface->isAttribAvail(int_attrib::BinningX))
    {
        cam_interface->setAttrib(camera::int_attrib::BinningX,_binning_x);
        cam_interface->setAttrib(camera::int_attrib::BinningY,_binning_y);
    }

    //setting ExposureValue
    if(cam_interface->isAttribAvail(int_attrib::ExposureValue))
        cam_interface->setAttrib(camera::int_attrib::ExposureValue,_exposure);
    else
        RTT::log(RTT::Warning) << "ExposureValue is not supported by the camera" << RTT::endlog();

    //setting FrameRate
    if(cam_interface->isAttribAvail(double_attrib::FrameRate))
        cam_interface->setAttrib(camera::double_attrib::FrameRate,_fps);
    else
        RTT::log(RTT::Warning) << "FrameRate is not supported by the camera" << RTT::endlog();

    //setting GainValue
    if(cam_interface->isAttribAvail(int_attrib::GainValue))
        cam_interface->setAttrib(camera::int_attrib::GainValue,_gain);
    else
        RTT::log(RTT::Warning) << "GainValue is not supported by the camera" << RTT::endlog();

    //setting WhitebalValueBlue
    if(cam_interface->isAttribAvail(int_attrib::WhitebalValueBlue))
        cam_interface->setAttrib(camera::int_attrib::WhitebalValueBlue,_whitebalance_blue);
    else
        RTT::log(RTT::Warning) << "WhitebalValueBlue is not supported by the camera" << RTT::endlog();

    //setting WhitebalValueRed
    if(cam_interface->isAttribAvail(int_attrib::WhitebalValueRed))
        cam_interface->setAttrib(camera::int_attrib::WhitebalValueRed,_whitebalance_red);
    else
        RTT::log(RTT::Warning) << "WhitebalValueRed is not supported by the camera" << RTT::endlog();

    //setting WhitebalAutoRate
    if(cam_interface->isAttribAvail(int_attrib::WhitebalAutoRate))
        cam_interface->setAttrib(camera::int_attrib::WhitebalAutoRate,_whitebalance_auto_rate);
    else
        RTT::log(RTT::Warning) << "WhitebalAutoRate is not supported by the camera" << RTT::endlog();

    //setting WhitebalAutoAdjustTol
    if(cam_interface->isAttribAvail(int_attrib::WhitebalAutoAdjustTol))
        cam_interface->setAttrib(camera::int_attrib::WhitebalAutoAdjustTol,_whitebalance_auto_threshold);
    else
        RTT::log(RTT::Warning) << "WhitebalAutoAdjustTol is not supported by the camera" << RTT::endlog();

    //setting _whitebalance_mode
    if(_whitebalance_mode.value() == "manual")
    {
        if(cam_interface->isAttribAvail(camera::enum_attrib::WhitebalModeToManual))
            cam_interface->setAttrib(camera::enum_attrib::WhitebalModeToManual);
        else
            RTT::log(RTT::Warning) << "WhitebalModeToManual is not supported by the camera" << RTT::endlog();
    }
    else if (_whitebalance_mode.value() == "auto")
    {
        if(cam_interface->isAttribAvail(camera::enum_attrib::WhitebalModeToAuto))
            cam_interface->setAttrib(camera::enum_attrib::WhitebalModeToAuto);
        else
            RTT::log(RTT::Warning) << "WhitebalModeToAuto is not supported by the camera" << RTT::endlog();
    }
    else if (_whitebalance_mode.value() == "auto_once")
    {
        if(cam_interface->isAttribAvail(camera::enum_attrib::WhitebalModeToAutoOnce))
            cam_interface->setAttrib(camera::enum_attrib::WhitebalModeToAutoOnce);
        else
            RTT::log(RTT::Warning) << "WhitebalModeToAutoOnce is not supported by the camera" << RTT::endlog();
    }
    else if(_whitebalance_mode.value() == "none")
    {
        //do nothing
    }
    else
    {
        RTT::log(RTT::Error) << "Whitebalance mode "+ _whitebalance_mode.value() + " is not supported!" << RTT::endlog();
        error(UNKOWN_PARAMETER);
        return;
    }

    //setting _exposure_mode
    if(_exposure_mode.value() == "auto")
    {
        if(cam_interface->isAttribAvail(camera::enum_attrib::ExposureModeToAuto))
            cam_interface->setAttrib(camera::enum_attrib::ExposureModeToAuto);
        else
            RTT::log(RTT::Warning) << "ExposureModeToAuto is not supported by the camera" << RTT::endlog();
    }
    else if(_exposure_mode.value() =="manual")
    {
        if(cam_interface->isAttribAvail(camera::enum_attrib::ExposureModeToManual))
            cam_interface->setAttrib(camera::enum_attrib::ExposureModeToManual);
        else
            RTT::log(RTT::Warning) << "ExposureModeToManual is not supported by the camera" << RTT::endlog();
    }
    else if (_exposure_mode.value() =="external")
    {
        if(cam_interface->isAttribAvail(camera::enum_attrib::ExposureModeToExternal))
            cam_interface->setAttrib(camera::enum_attrib::ExposureModeToExternal);
        else
            RTT::log(RTT::Warning) << "ExposureModeToExternal is not supported by the camera" << RTT::endlog();
    }
    else if(_exposure_mode.value() == "none")
    {
        //do nothing
    }
    else
    {
        RTT::log(RTT::Error) << "Exposure mode "+ _exposure_mode.value() + " is not supported!" << RTT::endlog();
        error(UNKOWN_PARAMETER);
        return;
    }

    //setting _trigger_mode
    if(_trigger_mode.value() == "freerun")
    {
        if(cam_interface->isAttribAvail(camera::enum_attrib::FrameStartTriggerModeToFreerun))
            cam_interface->setAttrib(camera::enum_attrib::FrameStartTriggerModeToFreerun);
        else
            RTT::log(RTT::Warning) << "FrameStartTriggerModeToFreerun is not supported by the camera" << RTT::endlog();
    }
    else if (_trigger_mode.value() == "fixed")
    {
        if(cam_interface->isAttribAvail(camera::enum_attrib::FrameStartTriggerModeToFixedRate))
            cam_interface->setAttrib(camera::enum_attrib::FrameStartTriggerModeToFixedRate);
        else
            RTT::log(RTT::Warning) << "FrameStartTriggerModeToFixedRate is not supported by the camera" << RTT::endlog();
    }
    else if (_trigger_mode.value() == "sync_in1")
    {
        if(cam_interface->isAttribAvail(camera::enum_attrib::FrameStartTriggerModeToSyncIn1))
            cam_interface->setAttrib(camera::enum_attrib::FrameStartTriggerModeToSyncIn1);
        else
            RTT::log(RTT::Warning) << "FrameStartTriggerModeToSyncIn1 is not supported by the camera" << RTT::endlog();
    }
    else if(_trigger_mode.value() == "none")
    {
        //do nothing
    }
    else
    {
        RTT::log(RTT::Error) << "Trigger mode "+ _trigger_mode.value() + " is not supported!" + " is not supported!" << RTT::endlog();
        error(UNKOWN_PARAMETER);
        return;
    }

    //setting _frame_start_trigger_event
    if(_frame_start_trigger_event.value() == "EdgeRising")
    {
        if(cam_interface->isAttribAvail(camera::enum_attrib::FrameStartTriggerEventToEdgeRising))
            cam_interface->setAttrib(camera::enum_attrib::FrameStartTriggerEventToEdgeRising);
        else
            RTT::log(RTT::Warning) << "FrameStartTriggerEventToEdgeRising is not supported by the camera" << RTT::endlog();
    }
    else if (_frame_start_trigger_event.value() == "EdgeFalling")
    {
        if(cam_interface->isAttribAvail(camera::enum_attrib::FrameStartTriggerEventToEdgeFalling))
            cam_interface->setAttrib(camera::enum_attrib::FrameStartTriggerEventToEdgeFalling);
        else
            RTT::log(RTT::Warning) << "FrameStartTriggerEventToEdgeFalling is not supported by the camera" << RTT::endlog();
    }
    else if (_frame_start_trigger_event.value() == "EdgeAny")
    {
        if(cam_interface->isAttribAvail(camera::enum_attrib::FrameStartTriggerEventToEdgeAny))
            cam_interface->setAttrib(camera::enum_attrib::FrameStartTriggerEventToEdgeAny);
        else
            RTT::log(RTT::Warning) << "FrameStartTriggerEventToEdgeAny is not supported by the camera" << RTT::endlog();
    }
    else if (_frame_start_trigger_event.value() == "LevelHigh")
    {
        if(cam_interface->isAttribAvail(camera::enum_attrib::FrameStartTriggerEventToLevelHigh))
            cam_interface->setAttrib(camera::enum_attrib::FrameStartTriggerEventToLevelHigh);
        else
            RTT::log(RTT::Warning) << "FrameStartTriggerEventToLevelHigh is not supported by the camera" << RTT::endlog();
    }
    else if (_frame_start_trigger_event.value() == "LevelLow")
    {
        if(cam_interface->isAttribAvail(camera::enum_attrib::FrameStartTriggerEventToLevelLow))
            cam_interface->setAttrib(camera::enum_attrib::FrameStartTriggerEventToLevelLow);
        else
            RTT::log(RTT::Warning) << "FrameStartTriggerEventToLevelLow is not supported by the camera" << RTT::endlog();
    }
    else if(_frame_start_trigger_event.value() == "none")
    {
        //do nothing
    }
    else
    {
        RTT::log(RTT::Error) << "Frame start trigger event "+ _frame_start_trigger_event.value() + " is not supported!" << RTT::endlog();
        error(UNKOWN_PARAMETER);
        return;
    }

    RTT::log(RTT::Info) << "camera configuration: width="<<_width <<
        "; height=" << _height << 
        "; region_x=" << _region_x << 
        "; region_y=" << _region_y << 
        "; Trigger mode=" << _trigger_mode << 
        "; fps=" << _fps << 
        "; exposure=" << _exposure << 
        "; Whitebalance mode=" << _whitebalance_mode << 
        RTT::endlog();
}

bool Task::startHook()
{
  if (! TaskBase::startHook())
     return false;

  return true;
}

void Task::updateHook()
{
    TaskBase::updateHook();
}

void Task::stopHook()
{ 
    TaskBase::stopHook();
}

void Task::cleanupHook()
{
    TaskBase::cleanupHook();
}
// void Task::errorHook()
// {
// }


