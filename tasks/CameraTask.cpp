#include "CameraTask.hpp"

#include <rtt/NonPeriodicActivity.hpp>


using namespace camera;
using namespace base::samples::frame;
using namespace RTT;

RTT::NonPeriodicActivity* CameraTask::getNonPeriodicActivity()
{ return dynamic_cast< RTT::NonPeriodicActivity* >(getActivity().get()); }


CameraTask::CameraTask(std::string const& name)
    : CameraTaskBase(name)
{

}

bool CameraTask::configureHook()
{
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
        log(Error) << "unsupported camera mode: " << _mode.value() << endlog();
        return false;
    }  

    try
    {
        std::auto_ptr<camera::CamGigEProsilica> camera(new camera::CamGigEProsilica());
        log(Info) << "open camera" << endlog();
        camera->open2(camera_id,camera_acess_mode_);
        cam_interface_ = camera.release();
    }
    catch (std::runtime_error e)
    {
        log(Error) << "failed to initialize camera: " << e.what() << endlog();
        return false;
    }
    
    //synchronize camera time with system time
    if(_synchronize_time_interval)
      cam_interface_->synchronizeWithSystemTime(_synchronize_time_interval); 
    
    //set callback fcn
    cam_interface_->setCallbackFcn(triggerFunction,this);
    return true;
}

bool CameraTask::startHook()
{
  invalid_frames_count_ = 0;
  valid_frames_count_ = 0;
  time_save_ = base::Time::now();

  try 
  {
    log(Info) << "configure camera" << endlog(); 
   
     //define CamInterface output frame
    Frame *frame = new Frame;	
    if(camera_acess_mode_!= Monitor)
    {
      if(_output_format.value() == "bayer")
	 camera_frame.init(_width,_height,8,MODE_BAYER_GBRG);    
      else if (_output_format.value() == "rgb8")
	 camera_frame.init(_width,_height,8,MODE_BAYER_GBRG);  
      else if (_output_format.value() == "mono8")
	 camera_frame.init(_width,_height,8,MODE_GRAYSCALE);  
      else
      {
	  log(Error) << "output format "<< _output_format << " is not supported!" << endlog();
	  return false;
      }
      setCameraSettings();
    }
    else
    {
      cam_interface_->setFrameToCameraFrameSettings(camera_frame);
    }
    
    //define orocos_camera output frame
    if(_output_format.value() == "bayer")
	frame->init(camera_frame.getWidth(),camera_frame.getHeight(),8,MODE_BAYER_GBRG); 
    else if (_output_format.value() == "rgb8")
	frame->init(camera_frame.getWidth(),camera_frame.getHeight(),8,MODE_RGB);
    else if (_output_format.value() == "mono8")
	frame->init(camera_frame.getWidth(),camera_frame.getHeight(),8,MODE_GRAYSCALE);  
    else
    {
	log(Error) << "output format "<< _output_format << " is not supported!" << endlog();
	return false;
    }
    
    //init RTT::ReadOnlyPointer for output frame 
    current_frame_.reset(frame);	
    frame = NULL;
    cam_interface_->grab(camera::Continuously,_frame_buffer_size); 
  }
  catch(std::runtime_error e)
  {
      log(Error) << "failed to start camera: " << e.what() << endlog();
      return false;
  }
  return true;
}

void CameraTask::updateHook()
{
  if (cam_interface_->isFrameAvailable())
  {
    try 
    {
       switch(current_frame_->frame_mode)
       {
	 //debayering has to be performed
	 case MODE_RGB:
	   cam_interface_->retrieveFrame(camera_frame);
	   if (camera_frame.getStatus() == STATUS_VALID)
	   {
	     Frame *frame_ptr = current_frame_.write_access();
	     Helper::convertColor(camera_frame,*frame_ptr,MODE_RGB);
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
	 case MODE_GRAYSCALE:
	 case MODE_BAYER_GBRG:
	 {
	   Frame *frame_ptr = current_frame_.write_access();
	   cam_interface_->retrieveFrame(*frame_ptr);
	   if (frame_ptr->getStatus() == STATUS_VALID)
	   {
	    // std::cout << "camera " << _camera_id <<" frame timestamp:" << frame_ptr->time << std::endl;
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
	   log(Error) << "output format is not supported" << endlog();
       }
    }
    catch(std::runtime_error e)
    { 
      log(Error) << "failed to retrieve frame: " << e.what() << endlog();
      return;
    }
  }
  
  //check if statistic shall be displayed
  if(_log_interval_in_sec)
  {
    base::Time time = base::Time::now();
    if((time - time_save_).toSeconds() > _log_interval_in_sec)
    { 
      if(invalid_frames_count_)
	log(Warning) << "camera statistic: " << 
	  ((float)valid_frames_count_)/_log_interval_in_sec << " valid fps; " <<
	  ((float)invalid_frames_count_)/_log_interval_in_sec << " invalid fps; " << endlog();
      else
	log(Info) << "camera statistic: " << 
	  ((float)valid_frames_count_)/_log_interval_in_sec << " valid fps; " <<
	  ((float)invalid_frames_count_)/_log_interval_in_sec << " invalid fps; " << endlog();
      
      time_save_ = time;
      valid_frames_count_ = 0;
      invalid_frames_count_ = 0;
    }
  }
  else	//prevents overflow after > 800 days
  {
      valid_frames_count_ = 0;
      invalid_frames_count_ = 0;
  }
}

void CameraTask::stopHook()
{
  log(Info) << "stop grabbing" << endlog();
  cam_interface_->grab(camera::Stop);
  sleep(1);
}

void CameraTask::cleanupHook()
{
  if(cam_interface_)
  {
    cam_interface_->close();
    delete cam_interface_;
    cam_interface_ = NULL;
  }
}

void CameraTask::setCameraSettings()
{
    //set camera settings
    //sets binning to 1 otherwise high resolution can not be set
    
    cam_interface_->setAttrib(int_attrib::BinningX,1);
    cam_interface_->setAttrib(int_attrib::BinningY,1);
    cam_interface_->setFrameSettings(camera_frame);
    cam_interface_->setAttrib(camera::int_attrib::RegionX,_region_x);
    cam_interface_->setAttrib(camera::int_attrib::RegionY,_region_y);
    cam_interface_->setAttrib(camera::int_attrib::BinningX,_binning_x);
    cam_interface_->setAttrib(camera::int_attrib::BinningY,_binning_y);
    cam_interface_->setAttrib(camera::enum_attrib::ExposureModeToManual);
    cam_interface_->setAttrib(camera::int_attrib::ExposureValue,_exposure);
    cam_interface_->setAttrib(camera::double_attrib::FrameRate,_fps);
    cam_interface_->setAttrib(camera::int_attrib::GainValue,_gain);
    cam_interface_->setAttrib(camera::int_attrib::WhitebalValueBlue,_whitebalance_blue);
    cam_interface_->setAttrib(camera::int_attrib::WhitebalValueRed,_whitebalance_red);
    cam_interface_->setAttrib(camera::int_attrib::WhitebalAutoRate,_whitebalance_auto_rate);
    cam_interface_->setAttrib(camera::int_attrib::WhitebalAutoAdjustTol,_whitebalance_auto_threshold);
    
    if(_whitebalance_mode.value() == "manual")
	cam_interface_->setAttrib(camera::enum_attrib::WhitebalModeToManual);
    else if (_whitebalance_mode.value() == "auto")
	cam_interface_->setAttrib(camera::enum_attrib::WhitebalModeToAuto);
    else if (_whitebalance_mode.value() == "auto_once")
	cam_interface_->setAttrib(camera::enum_attrib::WhitebalModeToAutoOnce);
    else
    {
	throw std::runtime_error("Whitebalance mode "+ _whitebalance_mode.value() + " is not supported!");
    }
    
    if(_exposure_mode.value() == "auto")
	cam_interface_->setAttrib(camera::enum_attrib::ExposureModeToAuto);
    else if(_exposure_mode.value() =="manual")
	cam_interface_->setAttrib(camera::enum_attrib::ExposureModeToManual);
    else if (_exposure_mode.value() =="external")
	cam_interface_->setAttrib(camera::enum_attrib::ExposureModeToExternal);
    else
    {
	throw std::runtime_error("Exposure mode "+ _exposure_mode.value() + " is not supported!");
    }
    
    if(_trigger_mode.value() == "freerun")
	cam_interface_->setAttrib(camera::enum_attrib::FrameStartTriggerModeToFreerun);
    else if (_trigger_mode.value() == "fixed")
	cam_interface_->setAttrib(camera::enum_attrib::FrameStartTriggerModeToFixedRate);
    else if (_trigger_mode.value() == "sync_in1")
	cam_interface_->setAttrib(camera::enum_attrib::FrameStartTriggerModeToSyncIn1);
    else
    {
	throw std::runtime_error("Trigger mode "+ _trigger_mode.value() + " is not supported!");
    }
    
    if(_frame_start_trigger_event.value() == "EdgeRising")
	cam_interface_->setAttrib(camera::enum_attrib::FrameStartTriggerEventToEdgeRising);
    else if (_trigger_mode.value() == "EdgeFalling")
	cam_interface_->setAttrib(camera::enum_attrib::FrameStartTriggerEventToEdgeFalling);
    else if (_trigger_mode.value() == "EdgeAny")
	cam_interface_->setAttrib(camera::enum_attrib::FrameStartTriggerEventToEdgeAny);
    else if (_trigger_mode.value() == "LevelHigh")
	cam_interface_->setAttrib(camera::enum_attrib::FrameStartTriggerEventToLevelHigh);
    else if (_trigger_mode.value() == "LevelLow")
	cam_interface_->setAttrib(camera::enum_attrib::FrameStartTriggerEventToLevelLow);
    else
    {
	throw std::runtime_error("Frame start trigger event "+ _trigger_mode.value() + " is not supported!");
    }
    
    log(Info) << "camera configuration: width="<<_width <<
                                    "; height=" << _height << 
				    "; region_x=" << _region_x << 
				    "; region_y=" << _region_y << 
				    "; Trigger mode=" << _trigger_mode << 
				    "; fps=" << _fps << 
				    "; exposure=" << _exposure << 
				    "; Whitebalance mode=" << _whitebalance_mode << 
				    endlog();
}

void CameraTask::triggerFunction(const void *p)
{
  ((RTT::TaskContext*)p)->getActivity()->trigger();
}

// void CameraTask::errorHook()
// {
// }


