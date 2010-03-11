#include "CameraTask.hpp"

#include <rtt/NonPeriodicActivity.hpp>


using namespace camera;


RTT::NonPeriodicActivity* CameraTask::getNonPeriodicActivity()
{ return dynamic_cast< RTT::NonPeriodicActivity* >(getActivity().get()); }


CameraTask::CameraTask(std::string const& name)
    : CameraTaskBase(name)
{
}





/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See CameraTask.hpp for more detailed
// documentation about them.

// bool CameraTask::configureHook()
// {
//     return true;
// }
// bool CameraTask::startHook()
// {
//     return true;
// }

// void CameraTask::updateHook()
// {
// }

// void CameraTask::errorHook()
// {
// }
// void CameraTask::stopHook()
// {
// }
// void CameraTask::cleanupHook()
// {
// }

