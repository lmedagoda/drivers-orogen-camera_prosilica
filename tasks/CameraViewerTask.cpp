#include "CameraViewerTask.hpp"

#include <rtt/NonPeriodicActivity.hpp>


using namespace camera;


RTT::NonPeriodicActivity* CameraViewerTask::getNonPeriodicActivity()
{ return dynamic_cast< RTT::NonPeriodicActivity* >(getActivity().get()); }


CameraViewerTask::CameraViewerTask(std::string const& name, TaskCore::TaskState initial_state)
    : CameraViewerTaskBase(name, initial_state)
{
}





/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See CameraViewerTask.hpp for more detailed
// documentation about them.

// bool CameraViewerTask::configureHook()
// {
//     return true;
// }
// bool CameraViewerTask::startHook()
// {
//     return true;
// }

// void CameraViewerTask::updateHook()
// {
// }

// void CameraViewerTask::errorHook()
// {
// }
// void CameraViewerTask::stopHook()
// {
// }
// void CameraViewerTask::cleanupHook()
// {
// }

