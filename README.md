## Status

| Plugin       | Type  |  Comments  |
| :----:  | :----:  | :----:  |
| rkximagesink    | Video Render (sink) |   kmssink on X11, for overlay display |
| kmssink        |   Video Render (sink)   |  Customized [kmssink](https://github.com/GStreamer/gst-plugins-bad/tree/master/sys/kms), for overlay display   |
| ~~rgaconvert~~        |    Video Converter   |  Customized [v4l2 transform](https://github.com/GStreamer/gst-plugins-good/blob/master/sys/v4l2/gstv4l2transform.c), for video colorspace,format,size conversion  |
| ~~rgacomposite~~ | Video Compositing | Video compoiste |
| ~~ispsrc~~        |    Device Sources  |  Customized [v4l2src](https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-good/html/gst-plugins-good-plugins-v4l2src.html), for rockchip isp driver  |

## Tips
* rgaconvert, rgacomposite, ispsrc are not yet completed. v4l2 transform, v4l2src can be used, with limited functions avaliable.  
* rkximagesink, kmssink usually show video on a overlay plane, but it's also possible to show them at bottom layer. 

## Usage

### rkximagesink
[Pipeline example](https://github.com/rockchip-linux/rk-rootfs-build/blob/master/overlay-debug/usr/local/bin/test_dec-gst.sh)

### rgaconvert
[Pipeline example](https://github.com/rockchip-linux/rk-rootfs-build/blob/master/overlay-debug/usr/local/bin/test_rga.sh)

### ispsrc
[Pipeline example](https://github.com/rockchip-linux/rk-rootfs-build/blob/master/overlay-debug/usr/local/bin/test_camera.sh)