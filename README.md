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
* `display-ratio` :  Enable the aspect ratio display : (default : true)

### rgaconvert
[Pipeline example](https://github.com/rockchip-linux/rk-rootfs-build/blob/master/overlay-debug/usr/local/bin/test_rga.sh)

Most of the properties are the same as that of v4l2 transform, below are rockchip extend properties:
* `rotation` : 90-degree step rotation mode for the inner video frame : (default : 0)
* `hflip` : horizontal flip : (default : false)
* `vflip` : vertical flip : (default : false)
* `input-crop` : [Selection-crop](https://01.org/linuxgraphics/gfx-docs/drm/media/uapi/v4l/selection-api-003.html), should be "left"x"top"x"width"x"height": (optional)
* `output-crop` : [Selection-compose](https://01.org/linuxgraphics/gfx-docs/drm/media/uapi/v4l/selection-api-003.html), should be "left"x"top"x"width"x"height" : (optional) 
* `vpu-stride` : Use 4 alignment for input height, to handle VPU buffer correctly. Note if it's enabled, input-crop are unavailable.  : (default : false) 


### rkcamsrc
[Pipeline example](https://github.com/rockchip-linux/rk-rootfs-build/blob/master/overlay-debug/usr/local/bin/test_camera.sh)

Most of the properties are the same as that of v4l2src, below are rockchip extend properties:
* `disable-autoconf` : If false, this plugin will init pad format/selection for isp_subdev/sensor, to make the media pipeline work out-of-box: (default : false)
* `tuning-xml-path` : tuning xml file, needed by 3A : (default : "/etc/cam_iq.xml")
* `isp-mode` : "0A" to disable 3A, "2A" to enable AWB/AE, ~~"3A" to enable AWB/AE/AF~~ : (default : "false")
* `input-crop` : [Selection-crop](https://01.org/linuxgraphics/gfx-docs/drm/media/uapi/v4l/selection-api-003.html), should be "left"x"top"x"width"x"height": (optional)

> NOTE: DO NOT RELY ON `disable-autoconf=false`!  
> This feature is only used to make debug conveniently.  
> rkcamsrc plugin is not designed as a CamHal. It's more like `v4l2-ctl`, just a simple capture program.  
> Since the use cases are divers, please handle `media-controller` and `pad format/selection` in APP level.
