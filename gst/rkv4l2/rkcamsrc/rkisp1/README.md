# rkisp1-lib

Those code can be separated from gstreamer plugin and you could use them in the other v4l2 program.

### RKISP1_3A_THREAD_CREATE
create a pthread to init rkisp1-lib

### RKISP1_3A_THREAD_EXIT
block util rkisp1-lib thread exit and destory resources

### RKISP1_3A_THREAD_START
streamon params/stats node, should be called before starting video capture

### RKISP1_3A_THREAD_STOP
streamoff params/stats node, should be called before stoping video capture

### RKISP1_GET_3A_RESULT
return current 3A result, it can be used to check if 3a is converged so user can take a picture.
