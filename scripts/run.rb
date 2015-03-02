#!/usr/bin/env ruby

require 'orocos'
require 'vizkit'

Orocos.initialize
Orocos.run 'camera_prosilica::Task' => 'camera_a' do
    camera = Orocos::TaskContext.get 'camera_a'
    camera.camera_id = '148123'
    camera.binning_x = 1
    camera.binning_y = 1
    camera.region_x = 0
    camera.region_y = 0
    camera.width = 640
    camera.height = 480
    camera.trigger_mode = 'freerun'
    camera.exposure = 50000
    camera.exposure_mode = 'manual'
    camera.whitebalance_mode = 'manual'
    camera.fps = 10
    camera.gain = 6
    camera.gain_mode_auto = 0
    camera.sync_out1_mode = "FrameTriggerReady"
    camera.sync_out2_mode = "FrameTriggerReady"
 #   camera.camera_format = :MODE_RGB
    camera.camera_format = :MODE_GRAYSCALE
    camera.output_format = :MODE_GRAYSCALE
#    camera.resize_algorithm = :BAYER_RESIZE
#    camera.scale_x = 0.5
#    camera.scale_y = 0.5
    camera.log_interval_in_sec = 5
    camera.mode = 'Master'
#    camera.disable_frame_raw = false

    camera.configure
    camera.start
    camera.log_all_ports

 #   Vizkit.control camera
    Vizkit.display camera.frame
    #Vizkit.display camera.frame_raw
    Vizkit.exec
end
