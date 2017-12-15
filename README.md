## About

This is a set of GStreamer plugins about graphics/camera for rockchip platform.<br>
Most of them were modified based on upstream existing plugin.

This software has been tested only with kernel after 4.4.

## Status

| Elements       | Type  |  Comments  | Origin |
| :----:  | :----:  | :----:  | :----:  |
| rkximagesink    | Video Render (sink) |   kmssink on X11, for overlay display | ximagesink + kmssink |
| kmssink        |   Video Render (sink)   | overlay display   | [kmssink](https://github.com/GStreamer/gst-plugins-bad/tree/master/sys/kms) |
| rgaconvert       |    Video Converter   | video colorspace,format,size conversion  | [v4l2 transform](https://github.com/GStreamer/gst-plugins-good/blob/master/sys/v4l2/gstv4l2transform.c) |
| rkcamsrc        |    Device Sources  |  rockchip isp camera source  | [v4l2src](https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-good/html/gst-plugins-good-plugins-v4l2src.html) |

## Usage

### rkximagesink
[Pipeline example](https://github.com/rockchip-linux/rk-rootfs-build/blob/master/overlay-debug/usr/local/bin/test_dec-gst.sh)

* `display` : X Display name, for window display : (optional)
* `connector-id` : DRM connector id, for drm display : (optional)

### rgaconvert
[Pipeline example](https://github.com/rockchip-linux/rk-rootfs-build/blob/master/overlay-debug/usr/local/bin/test_rga.sh)

Most of the properties are the same as that of v4l2 transform, below are rockchip extend properties:
* `rotation` : 90-degree step rotation mode for the inner video frame : (default : 0)
* `hflip` : horizontal flip : (default : false)
* `vflip` : ve sink pad rtical flip : (default : false)
* `input-crop` : [Selection-crop](https://01.org/linuxgraphics/gfx-docs/drm/media/uapi/v4l/selection-api-003.html), should be "left"x"top"x"width"x"height": (optional)
* `output-crop` : [Selection-compose](https://01.org/linuxgraphics/gfx-docs/drm/media/uapi/v4l/selection-api-003.html), should be "left"x"top"x"width"x"height" : (optional) 
* `vpu-stride` : Use 4 alignment for input height, to handle VPU buffer correctly. Note if it's enabled, input-crop are unavailable.  : (default : false) 


### rkcamsrc
[Pipeline example](https://github.com/rockchip-linux/rk-rootfs-build/blob/master/overlay-debug/usr/local/bin/test_camera.sh)

Most of the properties are the same as that of v4l2src, below are rockchip extend properties:
* `disable-autoconf` : If false, this plugin will set pad format/crop window for isp_subdev/mainpath/selfpath, to make the media pipeline work out-of-box: (default : false)
* `disable-3A` : If false, 3A will be enabled, it will read IQ params from `/etc/cam_iq.xml` : (default : true)
* `video-compose` : compose value for stream dev, should be "left"x"top"x"width"x"height" : (optional)
* `video-crop` : crop value for stream dev, should be "left"x"top"x"width"x"height" : (optional)
* `output-crop` : output crop value for isp subdev, should be "left"x"top"x"width"x"height" : (optional)
* `input-crop` : input crop value for isp subdev, should be "left"x"top"x"width"x"height" : (optional)
* `sensor-crop` : crop value for sensor, should be "left"x"top"x"width"x"height" : (optional)

> NOTE: rkcamsrc won't modify media-enities, also won't do anything about sensor.  
> To meeting the demands of a complex hardware, do them in app level.