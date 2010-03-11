#include "CameraViewerTask.hpp"

#include <rtt/NonPeriodicActivity.hpp>


using namespace camera;


RTT::NonPeriodicActivity* CameraViewerTask::getNonPeriodicActivity()
{ return dynamic_cast< RTT::NonPeriodicActivity* >(getActivity().get()); }


CameraViewerTask::CameraViewerTask(std::string const& name, TaskCore::TaskState initial_state)
    : CameraViewerTaskBase(name, initial_state)
{
}


// bool CameraViewerTask::configureHook()
// {
//     return true;
// }

bool CameraViewerTask::startHook()
{
  cv::namedWindow("image",CV_WINDOW_AUTOSIZE);
  cv::waitKey(2);
  return true;

}

void CameraViewerTask::updateHook()
{
      if(_frame.read(current_frame_))
      {
 	if(current_frame_->getStatus() == camera::STATUS_VALID)
 	  cv::imshow("image",current_frame_->convertToCvMat());
 	else
 	  RTT::log(RTT::Warning) << "invalid frame" << RTT::endlog();
 	cv::waitKey(2);
      }
}


// void CameraViewerTask::errorHook()
// {
// }
// void CameraViewerTask::stopHook()
// {
// }
// void CameraViewerTask::cleanupHook()
// {
// }

