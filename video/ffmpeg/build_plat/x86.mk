LIBS += $(LIB_DIR)/libsirius.a

# ffmpeg
LIBS += $(LIB_DIR)/libavcodec.a
# LIBS += $(LIB_DIR)/libavdevice.a
# LIBS += $(LIB_DIR)/libavfilter.a
LIBS += $(LIB_DIR)/libavformat.a
LIBS += $(LIB_DIR)/libavutil.a
LIBS += $(LIB_DIR)/libswresample.a
LIBS += $(LIB_DIR)/libswscale.a

# ffmpeg 依赖的动态库
# libdl.so
# libz.so
# liblzma.so
# libbz2.so
# libdrm.so
# libX11.so
# libva-drm.so
# libva-x11.so
# LDFLAGS += -lva -lz -llzma -lbz2 -ldrm -lX11 -lva-drm -lva-x11
