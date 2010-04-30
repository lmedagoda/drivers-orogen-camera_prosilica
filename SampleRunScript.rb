#! /usr/bin/env ruby
# -*- coding: utf-8 -*-

require 'orocos'
include Orocos
Orocos.initialize

Orocos.run 'camera' do
  Orocos.log_all_ports
  camera = TaskContext.get 'camera'
  camera.camera_id = '105984'
  camera.binning_x = 3
  camera.binning_y = 3
  camera.width =640
  camera.height =360
  camera.trigger_mode = 'fixed'
  camera.exposure = 5000
  camera.exposure_mode_auto = 1
  camera.fps = 1
  camera.gain = 0
  camera.gain_mode_auto = 0
  camera.output_format = 'mono8'
  #camera.output_format = 'bayer'
  camera.log_interval_in_sec = 5
  camera.mode = 'Master'
  camera.synchronize_time_interval = 2000000
  #camera.whitebalance_mode = 'auto' 
  #camera.whitebalance_auto_rate = 100
  #camera.whitebalance_auto_threshold = 5
  #camera.whitebalance_red = 100
  #camera.whitebalance_blue = 100

  viewer = TaskContext.get 'camera_viewer'
  viewer.frame.connect_to camera.frame

  camera.configure  	
  camera.start
  viewer.start

  for i in (1..10000)
	sleep 0.01
  end 

  STDERR.puts "shutting down"
end

