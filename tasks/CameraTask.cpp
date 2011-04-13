#include "CameraTask.hpp"

#include "frame_helper/FrameHelper.h"

using namespace camera;
using namespace base::samples;
using namespace frame_helper;

CameraTask::CameraTask(std::string const& name)
    : CameraTaskBase(name),stat_frame_rate(0),stat_invalid_frame_rate(0),stat_valid_frame_rate(0)
{
}

CameraTask::~CameraTask()
{
    if(cam_interface_)
    {
        cam_interface_->close();
        delete cam_interface_;
        cam_interface_ = NULL;
    }
}
utopr
bool CameraTask::configureHook()
{
    if (! CameraTaskBase::configureHook())
      return false;
  
    //convert camera_id to unsigned long
    unsigned long camera_id;
    std::stringstream ss(_camera_id.value());
    ss >> camera_id;
    
    if(_mode.value() == "Master")
	camera_acess_mode_ = Master;
    else if (_mode.value() == "Monitor")
        camera_acess_mode_ = Monitor;
    else if (_mode.value() ==  "MasterMulticast")
	camera_acess_mode_ = MasterMulticast;
    else
    {
        RTT::log(RTT::Error) << "unsupported camera mode: " << _mode.value() << RTT::endlog();
        return false;
    }  

    try
    {
        std::auto_ptr<camera::CamGigEProsilica> camera(new camera::CamGigEProsilica(_package_size));
        RTT::log(RTT::Info) << "open camera" << RTT::endlog();
        camera->open2(camera_id,camera_acess_mode_);
        cam_interface_ = camera.release();
    }
    catch (std::runtime_error e)
    {
        RTT::log(RTT::Error) << "failed to initialize camera: " << e.what() << RTT::endlog();
        return false;
    }
    
    //synchronize camera time with system time
 //   if(_synchronize_time_interval)
 //     cam_interface_->synchronizeWithSystemTime(_synchronize_time_interval); 
    
    //set callback fcn
    cam_interface_->setCallbackFcn(triggerFunction,this);
    return true;
}

bool CameraTask::startHook()
{
  if (! CameraTaskBase::startHook())
     return false;

  invalid_frames_count_ = 0;
  valid_frames_count_ = 0;
  time_save_ = base::Time::now();

  try 
  {
    RTT::log(RTT::Info) << "configure camera" << RTT::endlog(); 
   
     //define CamInterface output frame
    frame::Frame *frame = new frame::Frame;	
    if(camera_acess_mode_!= Monitor)
    {
      if(_output_format.value() == "bayer8")
	 camera_frame.init(_width,_height,8,frame::MODE_BAYER);    
      else if (_output_format.value() == "rgb8")
	 camera_frame.init(_width,_height,8,frame::MODE_BAYER);  
      else if (_output_format.value() == "mono8")
	 camera_frame.init(_width,_height,8,frame::MODE_GRAYSCALE);  
      else
      {
	  RTT::log(RTT::Error) << "output format "<< _output_format.value() << " is not supported!" << RTT::endlog();
	  return false;
      }
      setCameraSettings();
    }
    else
    {
      cam_interface_->setFrameToCameraFrameSettings(camera_frame);
    }
    
    //define orocos_camera output frame
    if(_output_format.value() == "bayer8")
	frame->init(camera_frame.getWidth(),camera_frame.getHeight(),8,frame::MODE_BAYER); 
    else if (_output_format.value() == "rgb8")
	frame->init(camera_frame.getWidth(),camera_frame.getHeight(),8,frame::MODE_RGB);
    else if (_output_format.value() == "mono8")
	frame->init(camera_frame.getWidth(),camera_frame.getHeight(),8,frame::MODE_GRAYSCALE);  
    else
    {
	RTT::log(RTT::Error) << "output format "<< _output_format.value() << " is not supported!" << RTT::endlog();
	return false;
    }
    
    //init RTT::ReadOnlyPointer for output frame 
    current_frame_.reset(frame);	
    frame = NULL;
    RTT::log(RTT::Info) << cam_interface_->doDiagnose() << RTT::endlog();
    cam_interface_->grab(camera::Continuously,_frame_buffer_size); 
  }
  catch(std::runtime_error e)
  {
      RTT::log(RTT::Error) << "failed to start camera: " << e.what() << RTT::endlog();
      return false;
  }
  return true;
}

inline void CameraTask::setExtraAttributes(frame::Frame *frame_ptr)
{
  if(_log_interval_in_sec)
  {
      base::Time time = base::Time::now();
      float time_diff = (time - time_save_).toSeconds();

       if(time_diff > _log_interval_in_sec)
       {
          stat_valid_frame_rate  = ((float)valid_frames_count_)/time_diff;
          stat_invalid_frame_rate  = ((float)invalid_frames_count_)/time_diff;
        //  time_save_ = time;
          valid_frames_count_ = 0;
          invalid_frames_count_ = 0;
          //get camera frame rate
          if(cam_interface_->isAttribAvail(double_attrib::StatFrameRate))
            stat_frame_rate = cam_interface_->getAttrib(double_attrib::StatFrameRate);
       }

      frame_ptr->setAttribute<float>("StatFps",stat_frame_rate);
      frame_ptr->setAttribute<float>("StatValidFps",stat_valid_frame_rate);
      frame_ptr->setAttribute<float>("StatInValidFps",stat_invalid_frame_rate);
  }
  else  //prevents overflow after > 800 days
  {
     valid_frames_count_ = 0;
     invalid_frames_count_ = 0;
  }
}

void CameraTask::updateHook()
{
  CameraTaskBase::updateHook();

  if (cam_interface_->isFrameAvailable())
  {
    switch(current_frame_->frame_mode)
    {
      //debayering has to be performed
      case frame::MODE_RGB:
	try 
	{
	  cam_interface_->retrieveFrame(camera_frame);
	}
	catch(std::runtime_error e)
	{ 
          RTT::log(RTT::Warning) << "failed to retrieve frame: " << e.what() << RTT::endlog();
	  if(_clear_buffer_if_frame_drop)
	    cam_interface_->skipFrames();
	}
	
	if (camera_frame.getStatus() == frame::STATUS_VALID)
	{
          frame::Frame *frame_ptr = current_frame_.write_access();
	  FrameHelper::convertColor(camera_frame,*frame_ptr);
	  setExtraAttributes(frame_ptr);
	  current_frame_.reset(frame_ptr);
	  _frame.write(current_frame_);
	  valid_frames_count_++;
	}
	else
	{
	  invalid_frames_count_++;
	}
	break;
      //no debayering
      case frame::MODE_GRAYSCALE:
      case frame::MODE_BAYER:
      case frame::MODE_BAYER_GBRG:
      case frame::MODE_BAYER_GRBG:
      case frame::MODE_BAYER_RGGB:
      case frame::MODE_BAYER_BGGR:
      {
        frame::Frame *frame_ptr = current_frame_.write_access();
	try
	{
	  cam_interface_->retrieveFrame(*frame_ptr);
	}
	catch(std::runtime_error e)
	{ 
          RTT::log(RTT::Warning) << "failed to retrieve frame: " << e.what() << RTT::endlog();
	  if(_clear_buffer_if_frame_drop)
	    cam_interface_->skipFrames();
	}
	if (frame_ptr->getStatus() == frame::STATUS_VALID)
	{
	  setExtraAttributes(frame_ptr);
	  current_frame_.reset(frame_ptr);
	  _frame.write(current_frame_);
	  valid_frames_count_++;
	}
	else
	{
	  current_frame_.reset(frame_ptr);
	  invalid_frames_count_++;
	}
	break;
      }
      default:
	RTT::log(RTT::Error) << "output format is not supported" << RTT::endlog();
    }
  }
  if (cam_interface_->isFrameAvailable())
    this->getActivity()->trigger();
}

void CameraTask::stopHook()
{ 
  CameraTaskBase::stopHook();

  RTT::log(RTT::Info) << "stop grabbing" << RTT::endlog();
  cam_interface_->grab(camera::Stop);
  sleep(1);
}

void CameraTask::cleanupHook()
{
  CameraTaskBase::cleanupHook();
  if(cam_interface_)
  {
    cam_interface_->close();
    delete cam_interface_;
    cam_interface_ = NULL;
  }
}

 //set camera settings
void CameraTask::setCameraSettings()
{
    //sets binning to 1 otherwise high resolution can not be set
    if(cam_interface_->isAttribAvail(int_attrib::BinningX))
    {
      cam_interface_->setAttrib(int_attrib::BinningX,1);
      cam_interface_->setAttrib(int_attrib::BinningY,1);
    }
    else
      RTT::log(RTT::Warning) << "Binning is not supported by the camera" << RTT::endlog();
    
    //setting resolution and color mode
    cam_interface_->setFrameSettings(camera_frame);
    
    //setting Region
    if(cam_interface_->isAttribAvail(int_attrib::RegionX))
    {
      cam_interface_->setAttrib(camera::int_attrib::RegionX,_region_x);
      cam_interface_->setAttrib(camera::int_attrib::RegionY,_region_y);
    }
    else
      RTT::log(RTT::Warning) << "Region is not supported by the camera" << RTT::endlog();
    
    //setting Binning
    if(cam_interface_->isAttribAvail(int_attrib::BinningX))
    {
      cam_interface_->setAttrib(camera::int_attrib::BinningX,_binning_x);
      cam_interface_->setAttrib(camera::int_attrib::BinningY,_binning_y);
    }
    
    //setting ExposureValue
    if(cam_interface_->isAttribAvail(int_attrib::ExposureValue))
      cam_interface_->setAttrib(camera::int_attrib::ExposureValue,_exposure);
    else
      RTT::log(RTT::Warning) << "ExposureValue is not supported by the camera" << RTT::endlog();
    
    //setting FrameRate
    if(cam_interface_->isAttribAvail(double_attrib::FrameRate))
      cam_interface_->setAttrib(camera::double_attrib::FrameRate,_fps);
    else
      RTT::log(RTT::Warning) << "FrameRate is not supported by the camera" << RTT::endlog();
    
    //setting GainValue
    if(cam_interface_->isAttribAvail(int_attrib::GainValue))
      cam_interface_->setAttrib(camera::int_attrib::GainValue,_gain);
    else
      RTT::log(RTT::Warning) << "GainValue is not supported by the camera" << RTT::endlog();
    
     //setting WhitebalValueBlue
    if(cam_interface_->isAttribAvail(int_attrib::WhitebalValueBlue))
      cam_interface_->setAttrib(camera::int_attrib::WhitebalValueBlue,_whitebalance_blue);
    else
      RTT::log(RTT::Warning) << "WhitebalValueBlue is not supported by the camera" << RTT::endlog();
    
    //setting WhitebalValueRed
    if(cam_interface_->isAttribAvail(int_attrib::WhitebalValueRed))
      cam_interface_->setAttrib(camera::int_attrib::WhitebalValueRed,_whitebalance_red);
    else
      RTT::log(RTT::Warning) << "WhitebalValueRed is not supported by the camera" << RTT::endlog();
    
    //setting WhitebalAutoRate
    if(cam_interface_->isAttribAvail(int_attrib::WhitebalAutoRate))
      cam_interface_->setAttrib(camera::int_attrib::WhitebalAutoRate,_whitebalance_auto_rate);
    else
      RTT::log(RTT::Warning) << "WhitebalAutoRate is not supported by the camera" << RTT::endlog();
    
    //setting WhitebalAutoAdjustTol
    if(cam_interface_->isAttribAvail(int_attrib::WhitebalAutoAdjustTol))
      cam_interface_->setAttrib(camera::int_attrib::WhitebalAutoAdjustTol,_whitebalance_auto_threshold);
    else
      RTT::log(RTT::Warning) << "WhitebalAutoAdjustTol is not supported by the camera" << RTT::endlog();
    
    //setting _whitebalance_mode
    if(_whitebalance_mode.value() == "manual")
    {
	if(cam_interface_->isAttribAvail(camera::enum_attrib::WhitebalModeToManual))
	  cam_interface_->setAttrib(camera::enum_attrib::WhitebalModeToManual);
	else
	  RTT::log(RTT::Warning) << "WhitebalModeToManual is not supported by the camera" << RTT::endlog();
    }
    else if (_whitebalance_mode.value() == "auto")
    {
	if(cam_interface_->isAttribAvail(camera::enum_attrib::WhitebalModeToAuto))
	  cam_interface_->setAttrib(camera::enum_attrib::WhitebalModeToAuto);
	else
	  RTT::log(RTT::Warning) << "WhitebalModeToAuto is not supported by the camera" << RTT::endlog();
    }
    else if (_whitebalance_mode.value() == "auto_once")
    {
	if(cam_interface_->isAttribAvail(camera::enum_attrib::WhitebalModeToAutoOnce))
	  cam_interface_->setAttrib(camera::enum_attrib::WhitebalModeToAutoOnce);
	else
	  RTT::log(RTT::Warning) << "WhitebalModeToAutoOnce is not supported by the camera" << RTT::endlog();
    }
    else if(_whitebalance_mode.value() == "none")
    {
      //do nothing
    }
    else
    {
	throw std::runtime_error("Whitebalance mode "+ _whitebalance_mode.value() + " is not supported!");
    }
    
    //setting _exposure_mode
    if(_exposure_mode.value() == "auto")
    {
	if(cam_interface_->isAttribAvail(camera::enum_attrib::ExposureModeToAuto))
	  cam_interface_->setAttrib(camera::enum_attrib::ExposureModeToAuto);
	else
	  RTT::log(RTT::Warning) << "ExposureModeToAuto is not supported by the camera" << RTT::endlog();
    }
    else if(_exposure_mode.value() =="manual")
    {
	if(cam_interface_->isAttribAvail(camera::enum_attrib::ExposureModeToManual))
	  cam_interface_->setAttrib(camera::enum_attrib::ExposureModeToManual);
	else
	  RTT::log(RTT::Warning) << "ExposureModeToManual is not supported by the camera" << RTT::endlog();
    }
    else if (_exposure_mode.value() =="external")
    {
	if(cam_interface_->isAttribAvail(camera::enum_attrib::ExposureModeToExternal))
	  cam_interface_->setAttrib(camera::enum_attrib::ExposureModeToExternal);
	else
	  RTT::log(RTT::Warning) << "ExposureModeToExternal is not supported by the camera" << RTT::endlog();
    }
    else if(_exposure_mode.value() == "none")
    {
      //do nothing
    }
    else
    {
	throw std::runtime_error("Exposure mode "+ _exposure_mode.value() + " is not supported!");
    }
    
    //setting _trigger_mode
    if(_trigger_mode.value() == "freerun")
    {
	if(cam_interface_->isAttribAvail(camera::enum_attrib::FrameStartTriggerModeToFreerun))
	  cam_interface_->setAttrib(camera::enum_attrib::FrameStartTriggerModeToFreerun);
	else
	  RTT::log(RTT::Warning) << "FrameStartTriggerModeToFreerun is not supported by the camera" << RTT::endlog();
    }
    else if (_trigger_mode.value() == "fixed")
    {
	if(cam_interface_->isAttribAvail(camera::enum_attrib::FrameStartTriggerModeToFixedRate))
	  cam_interface_->setAttrib(camera::enum_attrib::FrameStartTriggerModeToFixedRate);
	else
	  RTT::log(RTT::Warning) << "FrameStartTriggerModeToFixedRate is not supported by the camera" << RTT::endlog();
    }
    else if (_trigger_mode.value() == "sync_in1")
    {
	if(cam_interface_->isAttribAvail(camera::enum_attrib::FrameStartTriggerModeToSyncIn1))
	  cam_interface_->setAttrib(camera::enum_attrib::FrameStartTriggerModeToSyncIn1);
	else
	  RTT::log(RTT::Warning) << "FrameStartTriggerModeToSyncIn1 is not supported by the camera" << RTT::endlog();
    }
    else if(_trigger_mode.value() == "none")
    {
      //do nothing
    }
    else
    {
	throw std::runtime_error("Trigger mode "+ _trigger_mode.value() + " is not supported!");
    }
    
    //setting _frame_start_trigger_event
    if(_frame_start_trigger_event.value() == "EdgeRising")
    {
	if(cam_interface_->isAttribAvail(camera::enum_attrib::FrameStartTriggerEventToEdgeRising))
	  cam_interface_->setAttrib(camera::enum_attrib::FrameStartTriggerEventToEdgeRising);
	else
	  RTT::log(RTT::Warning) << "FrameStartTriggerEventToEdgeRising is not supported by the camera" << RTT::endlog();
    }
    else if (_frame_start_trigger_event.value() == "EdgeFalling")
    {
	if(cam_interface_->isAttribAvail(camera::enum_attrib::FrameStartTriggerEventToEdgeFalling))
	  cam_interface_->setAttrib(camera::enum_attrib::FrameStartTriggerEventToEdgeFalling);
	else
	  RTT::log(RTT::Warning) << "FrameStartTriggerEventToEdgeFalling is not supported by the camera" << RTT::endlog();
    }
    else if (_frame_start_trigger_event.value() == "EdgeAny")
    {
	if(cam_interface_->isAttribAvail(camera::enum_attrib::FrameStartTriggerEventToEdgeAny))
	  cam_interface_->setAttrib(camera::enum_attrib::FrameStartTriggerEventToEdgeAny);
	else
	  RTT::log(RTT::Warning) << "FrameStartTriggerEventToEdgeAny is not supported by the camera" << RTT::endlog();
    }
    else if (_frame_start_trigger_event.value() == "LevelHigh")
    {
	if(cam_interface_->isAttribAvail(camera::enum_attrib::FrameStartTriggerEventToLevelHigh))
	  cam_interface_->setAttrib(camera::enum_attrib::FrameStartTriggerEventToLevelHigh);
	else
	  RTT::log(RTT::Warning) << "FrameStartTriggerEventToLevelHigh is not supported by the camera" << RTT::endlog();
    }
    else if (_frame_start_trigger_event.value() == "LevelLow")
    {
	if(cam_interface_->isAttribAvail(camera::enum_attrib::FrameStartTriggerEventToLevelLow))
	  cam_interface_->setAttrib(camera::enum_attrib::FrameStartTriggerEventToLevelLow);

	  RTT::log(RTT::Warning) << "FrameStartTriggerEventToLevelLow is not supported by the camera" << RTT::endlog();
    }
    else if(_frame_start_trigger_event.value() == "none")
    {
      //do nothing
    }
    else
    {
	throw std::runtime_error("Frame start trigger event "+ _frame_start_trigger_event.value() + " is not supported!");
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

void CameraTask::triggerFunction(const void *p)
{
  ((RTT::TaskContext*)p)->getActivity()->trigger();
}


//methods interface for the orocos module
bool CameraTask::setDoubleAttrib(camera::double_attrib::CamAttrib const & type, double value)
{
   try
   {
    cam_interface_->setAttrib(type,value);
   }
   catch(std::runtime_error e)
   {
    return false;
   }
   return true;
}

bool CameraTask::setEnumAttrib(camera::enum_attrib::CamAttrib const & type)
{
   try
   {
    cam_interface_->setAttrib(type);
   }
   catch(std::runtime_error e)
   {
    return false;
   }
   return true;
}

bool CameraTask::setIntAttrib(camera::int_attrib::CamAttrib const & type, int value)
{
   try
   {
    cam_interface_->setAttrib(type,value);
   }
   catch(std::runtime_error e)
   {
    return false;
   }
   return true;
}

bool CameraTask::setStringAttrib(camera::str_attrib::CamAttrib const & type, std::string const & value)
{
   try
   {
    cam_interface_->setAttrib(type,value);
   }
   catch(std::runtime_error e)
   {
    return false;
   }
   return true;
}

double CameraTask::getDoubleAttrib(camera::double_attrib::CamAttrib const & type)
{
   try
   {
    return cam_interface_->getAttrib(type);
   }
   catch(std::runtime_error e)
   {
    return -1;
   }
   return -1;
}

bool CameraTask::isEnumAttribSet(camera::enum_attrib::CamAttrib const & type)
{
   try
   {
    return cam_interface_->isAttribSet(type);
   }
   catch(std::runtime_error e)
   {
    return false;
   }
   return false;
}

int CameraTask::getIntAttrib(camera::int_attrib::CamAttrib const & type)
{
   try
   {
    return cam_interface_->getAttrib(type);
   }
   catch(std::runtime_error e)
   {
    return -1;
   }
   return -1;
}

std::string CameraTask::getStringAttrib(camera::str_attrib::CamAttrib const & type)
{
   try
   {
    return cam_interface_->getAttrib(type);
   }
   catch(std::runtime_error e)
   {
    return "";
   }
   return "";
}

boost::int32_t CameraTask::getIntRangeMin(camera::int_attrib::CamAttrib const & type)
{
   try
   {
     int imax,imin;
     cam_interface_->getRange(type,imin,imax);
     return imin;
   }
   catch(std::runtime_error e)
   {
    return -1;
   }
   return -1;
}

boost::int32_t CameraTask::getIntRangeMax(camera::int_attrib::CamAttrib const & type)
{
   try
   {
     int imax,imin;
     cam_interface_->getRange(type,imin,imax);
     return imax;
   }
   catch(std::runtime_error e)
   {
    return -1;
   }
   return -1;
}

double CameraTask::getDoubleRangeMin(camera::double_attrib::CamAttrib const & type)
{
   try
   {
     double dmax,dmin;
     cam_interface_->getRange(type,dmin,dmax);
     return dmin;
   }
   catch(std::runtime_error e)
   {
    return -1;
   }
   return -1;
}

double CameraTask::getDoubleRangeMax(camera::double_attrib::CamAttrib const & type)
{
   try
   {
     double dmax,dmin;
     cam_interface_->getRange(type,dmin,dmax);
     return dmax;
   }
   catch(std::runtime_error e)
   {
    return -1;
   }
   return -1;
}




// void CameraTask::errorHook()
// {
// }


