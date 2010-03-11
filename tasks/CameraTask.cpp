#include "CameraTask.hpp"

#include <rtt/NonPeriodicActivity.hpp>


using namespace camera;
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
   
  try
  {
       std::auto_ptr<camera::CamGigEProsilica> camera(new camera::CamGigEProsilica());
       log(Info) << "open camera" << endlog();
       camera->open2(camera_id,camera::Master);
       cam_interface_ = camera.release();
  }
  catch(std::runtime_error e)
  {
      log(Error) << "failed to initialize camera: " << e.what() << endlog();
      return false;
  }
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
    
    //init camera and RTT::ReadOnlyPointer for output frame 
    camera::Frame *frame = new camera::Frame;	
    if(_output_format.value() == "bayer")
	frame->init(_width,_height,8,camera::MODE_BAYER_GBRG); 
    else if (_output_format.value() == "rgb8")
	frame->init(_width,_height,8,camera::MODE_RGB); 
    else
    {
	log(Error) << "output format "<< _output_format << " is not supported!" << endlog();
	return false;
    }
    current_frame_.reset(frame);	
    frame = 0;
    
    bayer_frame.init(_width,_height,8,camera::MODE_BAYER_GBRG);		
    
    //set camera settings
    //sets binning to 1 otherwise high resolution can not be set
    camera.setAttrib(int_attrib::BinningX,1);
    camera.setAttrib(int_attrib::BinningY,1);
    cam_interface_->setFrameSettings(bayer_frame);
    cam_interface_->setAttrib(camera::int_attrib::RegionX,_region_x);
    cam_interface_->setAttrib(camera::int_attrib::RegionY,_region_y);
    cam_interface_->setAttrib(camera::int_attrib::BinningX,_binning_x);
    cam_interface_->setAttrib(camera::int_attrib::BinningY,_binning_y);
    cam_interface_->setAttrib(camera::enum_attrib::ExposureModeToManual);
    cam_interface_->setAttrib(camera::int_attrib::ExposureValue,_exposure);
    cam_interface_->setAttrib(camera::double_attrib::FrameRate,_fps);
    cam_interface_->setAttrib(camera::int_attrib::GainValue,_gain);
    if(_exposure_mode_auto)
	cam_interface_->setAttrib(camera::enum_attrib::ExposureModeToAuto);
    else
	cam_interface_->setAttrib(camera::enum_attrib::ExposureModeToManual);
    
    if(_gain_mode_auto)
      cam_interface_->setAttrib(camera::enum_attrib::GainModeToAuto);
    else
      cam_interface_->setAttrib(camera::enum_attrib::GainModeToManual);
    
    if(_trigger_mode.value() == "freerun")
	cam_interface_->setAttrib(camera::enum_attrib::FrameStartTriggerModeToFreerun);
    else if (_trigger_mode.value() == "fixed")
	cam_interface_->setAttrib(camera::enum_attrib::FrameStartTriggerModeToFixedRate);
    else if (_trigger_mode.value() == "sync_in1")
	cam_interface_->setAttrib(camera::enum_attrib::FrameStartTriggerModeToSyncIn1);
    else
    {
	log(Error) << "Trigger mode "<< _trigger_mode << " is not supported!" << endlog();
	return false;
    }
    
    log(Info) << "camera configuration: width="<<_width <<
                                    "; height=" << _height << 
				    "; region_x=" << _region_x << 
				    "; region_y=" << _region_y << 
				    "; Trigger mode=" << _trigger_mode << 
				    "; fps=" << _fps << 
				    "; exposure=" << _exposure << 
				    endlog();
    
    log(Info) << "start grabbing" << endlog();
    cam_interface_->grab(camera::Continuously,10); 
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
	 case camera::MODE_RGB:
	   cam_interface_->retrieveFrame(bayer_frame);
	   if (bayer_frame.getStatus() == camera::STATUS_VALID)
	   {
	     camera::Frame *frame_ptr = current_frame_.write_access();
	     camera::Helper::convertColor(bayer_frame,*frame_ptr,camera::MODE_RGB);
	     current_frame_.reset(frame_ptr);
	     _frame.write(current_frame_);
	     valid_frames_count_++;
	   }
	   else
	   {
	     invalid_frames_count_++;
	   }
	   break;
	 case camera::MODE_BAYER_GBRG:
	 {
	   camera::Frame *frame_ptr = current_frame_.write_access();
	   cam_interface_->retrieveFrame(*frame_ptr);
	   if (frame_ptr->getStatus() == camera::STATUS_VALID)
	   {
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
  
  //check if statistic shall be logged now
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

// void CameraTask::errorHook()
// {
// }


