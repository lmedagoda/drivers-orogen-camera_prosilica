/*
Could not load previous license
*/

#ifndef CAMERAREMOTEINTERFACE_H
#define CAMERAREMOTEINTERFACE_H

#include "camera_interface/CamInterface.h"
#include <rtt/TaskContext.hpp>
#include <rtt/Command.hpp>
#include <rtt/Method.hpp>

namespace camera
{
  class CameraRemoteInterface : public CamInterface
  {
    private:
    RTT::Method<bool(camera::int_attrib::CamAttrib,int)> _setIntAttrib;
    RTT::Method<bool(camera::enum_attrib::CamAttrib)> _setEnumAttrib;
    RTT::Method<bool(camera::double_attrib::CamAttrib,double)> _setDoubleAttrib;
    RTT::Method<bool(camera::str_attrib::CamAttrib,std::string)> _setStrAttrib;
    
    RTT::Method<int(camera::int_attrib::CamAttrib)> _getIntAttrib;
    RTT::Method<double(camera::double_attrib::CamAttrib)> _getDoubleAttrib;
    RTT::Method<std::string(camera::str_attrib::CamAttrib)> _getStrAttrib;
    RTT::Method<bool(camera::enum_attrib::CamAttrib)> _isEnumAttribSet;
    
    RTT::Method<int(camera::int_attrib::CamAttrib)> _getIntRangeMin;
    RTT::Method<int(camera::int_attrib::CamAttrib)> _getIntRangeMax;
    
    RTT::Method<double(camera::double_attrib::CamAttrib)> _getDoubleRangeMin;
    RTT::Method<double(camera::double_attrib::CamAttrib)> _getDoubleRangeMax;
    
    RTT::TaskContext* camera_task_context;
    
    public:
    CameraRemoteInterface(){};
    
    void checkTaskContext()
    {
      if(!camera_task_context)
	throw std::runtime_error("CameraRemoteInterface: the camera TaskContext is not set!!!");
    }
    
    //! connects to the camera orocos module
    /*!
      \param task_context pointer to the task context of the orocos_camera 
    */
    virtual void open(RTT::TaskContext *task_context)
    {
      camera_task_context = task_context;
      checkTaskContext();
    
      //binding functions
      _setIntAttrib = camera_task_context->methods()->getMethod<bool(int_attrib::CamAttrib,int)>("setIntAttrib");
      _setEnumAttrib = camera_task_context->methods()->getMethod<bool(enum_attrib::CamAttrib)>("setEnumAttrib");
      _setDoubleAttrib = camera_task_context->methods()->getMethod<bool(double_attrib::CamAttrib,double)>("setDoubleAttrib");
      _setStrAttrib = camera_task_context->methods()->getMethod<bool(str_attrib::CamAttrib,std::string)>("setStringAttrib");
      
      _getIntAttrib = camera_task_context->methods()->getMethod<int(int_attrib::CamAttrib)>("getIntAttrib");
      _isEnumAttribSet = camera_task_context->methods()->getMethod<bool(enum_attrib::CamAttrib)>("isEnumAttribSet");
      _getDoubleAttrib = camera_task_context->methods()->getMethod<double(double_attrib::CamAttrib)>("getDoubleAttrib");
      _getStrAttrib = camera_task_context->methods()->getMethod<std::string(str_attrib::CamAttrib)>("getStringAttrib");
      
      _getIntRangeMin = camera_task_context->methods()->getMethod<int(int_attrib::CamAttrib)>("getIntRangeMin");
      _getIntRangeMax = camera_task_context->methods()->getMethod<int(int_attrib::CamAttrib)>("getIntRangeMax");
      _getDoubleRangeMin = camera_task_context->methods()->getMethod<double(double_attrib::CamAttrib)>("getDoubleRangeMin");
      _getDoubleRangeMax = camera_task_context->methods()->getMethod<double(double_attrib::CamAttrib)>("getDoubleRangeMax");
      
    }
    
    virtual void getRange(const double_attrib::CamAttrib attrib,double &dmin,double &dmax)
    {
      checkTaskContext();
      if(!_getDoubleRangeMin.ready())
	throw std::runtime_error("CameraRemoteInterface: _getDoubleRangeMin is not ready!");
      if(!_getDoubleRangeMax.ready())
	throw std::runtime_error("CameraRemoteInterface: _getDoubleRangeMin is not ready!");
      
      dmax = _getDoubleRangeMax(attrib);
      dmin = _getDoubleRangeMin(attrib);
    };
	
    virtual void getRange(const int_attrib::CamAttrib attrib,int &imin,int &imax)
    {
      checkTaskContext();
      if(!_getIntRangeMin.ready())
	throw std::runtime_error("CameraRemoteInterface: _getIntRangeMin is not ready!");
      if(!_getIntRangeMax.ready())
	throw std::runtime_error("CameraRemoteInterface: _getIntRangeMax is not ready!");
      
      imax = _getIntRangeMax(attrib);
      imin = _getIntRangeMin(attrib);
    };
    
    virtual bool setAttrib(const int_attrib::CamAttrib attrib,const int value)
    {
      checkTaskContext();
      if(!_setIntAttrib.ready())
	throw std::runtime_error("CameraRemoteInterface: _setIntAttrib is not ready!");
      _setIntAttrib(attrib,value);
    };

    virtual bool setAttrib(const double_attrib::CamAttrib attrib, const double value)
    {
      checkTaskContext();
      if(!_setDoubleAttrib.ready())
	throw std::runtime_error("CameraRemoteInterface: _setDoubleAttrib is not ready!");
      _setDoubleAttrib(attrib,value);
    };
    
    virtual bool setAttrib(const str_attrib::CamAttrib attrib,const std::string value)
    {
      checkTaskContext();
      if(!_setStrAttrib.ready())
	throw std::runtime_error("CameraRemoteInterface: _setStrAttrib is not ready!");
      _setStrAttrib(attrib,value);
    };

    virtual bool setAttrib(const enum_attrib::CamAttrib attrib)
    {
      checkTaskContext();
      if(!_setEnumAttrib.ready())
	throw std::runtime_error("CameraRemoteInterface: _setEnumAttrib is not ready!");
      _setEnumAttrib(attrib);
    };

    virtual int getAttrib(const int_attrib::CamAttrib attrib)
    {
      checkTaskContext();
      if(!_getIntAttrib.ready())
	throw std::runtime_error("CameraRemoteInterface: _getIntAttrib is not ready!");
      return _getIntAttrib(attrib);
    };

    virtual double getAttrib(const double_attrib::CamAttrib attrib)
    {
      checkTaskContext();
      if(!_getDoubleAttrib.ready())
	throw std::runtime_error("CameraRemoteInterface: _getDoubleAttrib is not ready!");
      return _getDoubleAttrib(attrib);
    };

    virtual std::string getAttrib(const str_attrib::CamAttrib attrib)
    {
      checkTaskContext();
      if(!_getStrAttrib.ready())
	throw std::runtime_error("CameraRemoteInterface: _getStrAttrib is not ready!");
      return _getStrAttrib(attrib);
    };

    virtual bool isAttribSet(const enum_attrib::CamAttrib attrib)
    {
      checkTaskContext();
      if(!_isEnumAttribSet.ready())
	throw std::runtime_error("CameraRemoteInterface: _isEnumAttribSet is not ready!");
      return _isEnumAttribSet(attrib);
    }; 
  };
}

#endif // CAMERAREMOTEINTERFACE_H
