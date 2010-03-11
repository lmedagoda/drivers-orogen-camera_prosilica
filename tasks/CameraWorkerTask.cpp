#include "CameraWorkerTask.hpp"

#include <rtt/NonPeriodicActivity.hpp>


using namespace camera;


RTT::NonPeriodicActivity* CameraWorkerTask::getNonPeriodicActivity()
{ return dynamic_cast< RTT::NonPeriodicActivity* >(getActivity().get()); }


CameraWorkerTask::CameraWorkerTask(std::string const& name, TaskCore::TaskState initial_state)
    : CameraWorkerTaskBase(name, initial_state)
{
}





/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See CameraWorkerTask.hpp for more detailed
// documentation about them.

// bool CameraWorkerTask::configureHook()
// {
//     return true;
// }
// bool CameraWorkerTask::startHook()
// {
//     return true;
// }

// void CameraWorkerTask::updateHook()
// {
// }

// void CameraWorkerTask::errorHook()
// {
// }
// void CameraWorkerTask::stopHook()
// {
// }
// void CameraWorkerTask::cleanupHook()
// {
// }

