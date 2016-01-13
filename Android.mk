# don't include LOCAL_PATH for submodules

include $(CLEAR_VARS)
LOCAL_MODULE    := terrain
LOCAL_CFLAGS    := -Wall
LOCAL_SRC_FILES := terrain/terrain_tile.c terrain/terrain_util.c terrain/terrain_log.c

LOCAL_LDLIBS    := -Llibs/armeabi \
                   -llog

LOCAL_SHARED_LIBRARIES := libtexgz

include $(BUILD_SHARED_LIBRARY)
