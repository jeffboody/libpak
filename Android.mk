# don't include LOCAL_PATH for submodules

include $(CLEAR_VARS)
LOCAL_MODULE    := pak
LOCAL_CFLAGS    := -Wall
LOCAL_SRC_FILES := libpak/pak_file.c libpak/pak_log.c

LOCAL_LDLIBS    := -Llibs/armeabi \
                   -llog

include $(BUILD_SHARED_LIBRARY)
