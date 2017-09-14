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
| ispsrc        |    Device Sources  |  rockchip isp camera source  | [v4l2src](https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-good/html/gst-plugins-good-plugins-v4l2src.html) |

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

### ispsrc
[Pipeline example](https://github.com/rockchip-linux/rk-rootfs-build/blob/master/overlay-debug/usr/local/bin/test_camera.sh)

Most of the properties are the same as that of v4l2src, below are rockchip extend properties:
* `disable-sensor-autoconf` : If true, ispsrc won't configure sensor subdev, the other application should be responsible to configuring sensor subdev : (default : false)
* `rotation` : 90-degree step rotation mode for the inner video frame : (default : 0)
* `hflip` : horizontal flip : (default : false)
* `vflip` : vertical flip : (default : false)
* `dcrop` : crop value for stream dev, should be "left"x"top"x"width"x"height" : (optional)
* `isp-output-crop` : output crop value for isp subdev, should be "left"x"top"x"width"x"height" : (optional)
* `isp-input-crop` : input crop value for isp subdev, should be "left"x"top"x"width"x"height" : (optional)
* `sensor-crop` : crop value for sensor, should be "left"x"top"x"width"x"height" : (optional)

isp v4l2 dev struct:
```
sp/mp stream vdev -> isp subdev -> mipiphy subdev -> sensor subdev
```
