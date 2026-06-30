###########################################################################
## Makefile generated for component 'detect_aruco_in_camera_frame'. 
## 
## Makefile     : detect_aruco_in_camera_frame_rtw.mk
## Generated on : Fri Jun 12 09:38:52 2026
## Final product: ./detect_aruco_in_camera_frame.a
## Product type : static-library
## 
###########################################################################

###########################################################################
## MACROS
###########################################################################

# Macro Descriptions:
# PRODUCT_NAME            Name of the system to build
# MAKEFILE                Name of this makefile
# MODELLIB                Static library target

PRODUCT_NAME              = detect_aruco_in_camera_frame
MAKEFILE                  = detect_aruco_in_camera_frame_rtw.mk
MATLAB_ROOT               = /usr/local/MATLAB/R2026a
MATLAB_BIN                = /usr/local/MATLAB/R2026a/bin
MATLAB_ARCH_BIN           = $(MATLAB_BIN)/glnxa64
START_DIR                 = /home/daroe/asr_workspace/asr_flightstack/src/asr_perception/matlab/codegen
TGT_FCN_LIB               = ISO_C++11
SOLVER_OBJ                = 
CLASSIC_INTERFACE         = 0
MODEL_HAS_DYNAMICALLY_LOADED_SFCNS = 
RELATIVE_PATH_TO_ANCHOR   = .
C_STANDARD_OPTS           = -fwrapv
CPP_STANDARD_OPTS         = -fwrapv
MODELLIB                  = detect_aruco_in_camera_frame.a

###########################################################################
## TOOLCHAIN SPECIFICATIONS
###########################################################################

# Toolchain Name:          GNU gcc/g++ | gmake (64-bit Linux)
# Supported Version(s):    
# ToolchainInfo Version:   2026a
# Specification Revision:  1.0
# 
#-------------------------------------------
# Macros assumed to be defined elsewhere
#-------------------------------------------

# C_STANDARD_OPTS
# CPP_STANDARD_OPTS

#-----------
# MACROS
#-----------

WARN_FLAGS         = -Wall -W -Wwrite-strings -Winline -Wstrict-prototypes -Wnested-externs -Wpointer-arith -Wcast-align -Wno-stringop-overflow
WARN_FLAGS_MAX     = $(WARN_FLAGS) -Wcast-qual -Wshadow
CPP_WARN_FLAGS     = -Wall -W -Wwrite-strings -Winline -Wpointer-arith -Wcast-align -Wno-stringop-overflow
CPP_WARN_FLAGS_MAX = $(CPP_WARN_FLAGS) -Wcast-qual -Wshadow

TOOLCHAIN_SRCS = 
TOOLCHAIN_INCS = 
TOOLCHAIN_LIBS = 

FORMAT_FOR_ECHO_SH               = ""'$1'
FORMAT_FOR_ECHO                  = $(FORMAT_FOR_ECHO_SH)
HASH                             = \#
SEMICOLON                        = ;
OPEN_PAREN                       = (
CLOSE_PAREN                      = )
ESCAPE_SPECIAL_CHARS             = $(strip $(subst $(CLOSE_PAREN),\$(CLOSE_PAREN),\
	$(subst $(OPEN_PAREN),\$(OPEN_PAREN),\
	$(subst &,\&,\
	$(subst ~,\~,\
	$(subst ?,\?,\
	$(subst *,\*,\
	$(subst },\},\
	$(subst {,\{,\
	$(subst >,\>,\
	$(subst <,\<,\
	$(subst !,\!,\
	$(subst ],\],\
	$(subst [,\[,\
	$(subst $(HASH),\$(HASH),\
	$(subst \\,\\\,\
	$(subst ',\',\
	$(subst ",\",\
	$1))))))))))))))))))

#------------------------
# BUILD TOOL COMMANDS
#------------------------

# C Compiler: GNU C Compiler
CC = gcc

# Linker: GNU Linker
LD = g++

# C++ Compiler: GNU C++ Compiler
CPP = g++

# C++ Linker: GNU C++ Linker
CPP_LD = g++

# Archiver: GNU Archiver
AR = ar

# MEX Tool: MEX Tool
MEX_PATH = $(MATLAB_ARCH_BIN)
MEX = "$(MEX_PATH)/mex"

# Download: Download
DOWNLOAD =

# Execute: Execute
EXECUTE = $(PRODUCT)

# Builder: GMAKE Utility
MAKE_PATH = %MATLAB%/bin/glnxa64
MAKE = "$(MAKE_PATH)/gmake"


#-------------------------
# Directives/Utilities
#-------------------------

CDEBUG              = -g
C_OUTPUT_FLAG       = -o
LDDEBUG             = -g
OUTPUT_FLAG         = -o
CPPDEBUG            = -g
CPP_OUTPUT_FLAG     = -o
CPPLDDEBUG          = -g
OUTPUT_FLAG         = -o
ARDEBUG             =
STATICLIB_OUTPUT_FLAG =
MEX_DEBUG           = -g
RM                  = @rm -f
ECHO                = @echo
MV                  = @mv
RUN                 =

#--------------------------------------
# "Faster Runs" Build Configuration
#--------------------------------------

ARFLAGS              = ruvs
CFLAGS               = -c $(C_STANDARD_OPTS) -fPIC \
                       -O3
CPPFLAGS             = -c $(CPP_STANDARD_OPTS) -fPIC \
                       -O3
CPP_LDFLAGS          =
CPP_SHAREDLIB_LDFLAGS  = -shared -Wl,--no-undefined
DOWNLOAD_FLAGS       =
EXECUTE_FLAGS        =
LDFLAGS              =
MEX_CPPFLAGS         =
MEX_CPPLDFLAGS       =
MEX_CFLAGS           =
MEX_LDFLAGS          =
MAKE_FLAGS           = -j $(MAX_MAKE_JOBS) -l $(MAX_MAKE_LOAD_AVG) -f $(MAKEFILE)
SHAREDLIB_LDFLAGS    = -shared -Wl,--no-undefined



###########################################################################
## OUTPUT INFO
###########################################################################

PRODUCT = ./detect_aruco_in_camera_frame.a
PRODUCT_TYPE = "static-library"
BUILD_TYPE = "Static Library"

###########################################################################
## INCLUDE PATHS
###########################################################################

INCLUDES_BUILDINFO = -I$(START_DIR) -I/home/daroe/asr_workspace/asr_flightstack/src/asr_perception/matlab -I$(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/include -I$(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/include/aruco -I$(MATLAB_ROOT)/toolbox/images/opencv/opencvcg/include -I$(MATLAB_ROOT)/extern/include

INCLUDES = $(INCLUDES_BUILDINFO)

###########################################################################
## DEFINES
###########################################################################

DEFINES_CUSTOM = 
DEFINES_STANDARD = -DMODEL=detect_aruco_in_camera_frame

DEFINES = $(DEFINES_CUSTOM) $(DEFINES_STANDARD)

###########################################################################
## SOURCE FILES
###########################################################################

SRCS = $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/readArucoMarkerCore.cpp $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/cgCommon.cpp $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/aruco/readArucoImpl.cpp $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/aruco/arucoDictionary.cpp $(START_DIR)/detect_aruco_in_camera_frame_data.cpp $(START_DIR)/rt_nonfinite.cpp $(START_DIR)/rtGetNaN.cpp $(START_DIR)/rtGetInf.cpp $(START_DIR)/detect_aruco_in_camera_frame_initialize.cpp $(START_DIR)/detect_aruco_in_camera_frame_terminate.cpp $(START_DIR)/detect_aruco_in_camera_frame.cpp $(START_DIR)/readArucoMarker.cpp $(START_DIR)/estimateWorldCameraPose.cpp $(START_DIR)/rand.cpp $(START_DIR)/eml_rand_mt19937ar.cpp $(START_DIR)/solveP3P.cpp $(START_DIR)/roots.cpp $(START_DIR)/xnrm2.cpp $(START_DIR)/svd.cpp $(START_DIR)/xzlangeM.cpp $(START_DIR)/xdotc.cpp $(START_DIR)/rotationMatrixToVector.cpp $(START_DIR)/eml_rand_mt19937ar_stateful.cpp $(START_DIR)/xzgebal.cpp $(START_DIR)/xzgehrd.cpp $(START_DIR)/xdlahqr.cpp $(START_DIR)/xzlarfg.cpp $(START_DIR)/xdlanv2.cpp $(START_DIR)/xaxpy.cpp $(START_DIR)/xrotg.cpp $(START_DIR)/xrot.cpp $(START_DIR)/xswap.cpp $(START_DIR)/xzlascl.cpp $(START_DIR)/cameraIntrinsics.cpp $(START_DIR)/detect_aruco_in_camera_frame_rtwutil.cpp

ALL_SRCS = $(SRCS)

###########################################################################
## OBJECTS
###########################################################################

OBJS = readArucoMarkerCore.o cgCommon.o readArucoImpl.o arucoDictionary.o detect_aruco_in_camera_frame_data.o rt_nonfinite.o rtGetNaN.o rtGetInf.o detect_aruco_in_camera_frame_initialize.o detect_aruco_in_camera_frame_terminate.o detect_aruco_in_camera_frame.o readArucoMarker.o estimateWorldCameraPose.o rand.o eml_rand_mt19937ar.o solveP3P.o roots.o xnrm2.o svd.o xzlangeM.o xdotc.o rotationMatrixToVector.o eml_rand_mt19937ar_stateful.o xzgebal.o xzgehrd.o xdlahqr.o xzlarfg.o xdlanv2.o xaxpy.o xrotg.o xrot.o xswap.o xzlascl.o cameraIntrinsics.o detect_aruco_in_camera_frame_rtwutil.o

ALL_OBJS = $(OBJS)

###########################################################################
## PREBUILT OBJECT FILES
###########################################################################

PREBUILT_OBJS = 

###########################################################################
## LIBRARIES
###########################################################################

LIBS = $(MATLAB_ROOT)/bin/glnxa64/libopencv_calib3d.so.407 $(MATLAB_ROOT)/bin/glnxa64/libopencv_core.so.407 $(MATLAB_ROOT)/bin/glnxa64/libopencv_features2d.so.407 $(MATLAB_ROOT)/bin/glnxa64/libopencv_flann.so.407 $(MATLAB_ROOT)/bin/glnxa64/libopencv_imgproc.so.407 $(MATLAB_ROOT)/bin/glnxa64/libopencv_ml.so.407 $(MATLAB_ROOT)/bin/glnxa64/libopencv_objdetect.so.407 $(MATLAB_ROOT)/bin/glnxa64/libopencv_video.so.407

###########################################################################
## SYSTEM LIBRARIES
###########################################################################

SYSTEM_LIBS = -L$(MATLAB_ROOT)/sys/os/glnxa64 -lm -liomp5

###########################################################################
## ADDITIONAL TOOLCHAIN FLAGS
###########################################################################

#---------------
# C Compiler
#---------------

CFLAGS_OPTS = -fopenmp
CFLAGS_TFL = -msse2 -fno-predictive-commoning
CFLAGS_BASIC = $(DEFINES) $(INCLUDES)

CFLAGS += $(CFLAGS_OPTS) $(CFLAGS_TFL) $(CFLAGS_BASIC)

#-----------------
# C++ Compiler
#-----------------

CPPFLAGS_OPTS = -fopenmp
CPPFLAGS_TFL = -msse2 -fno-predictive-commoning
CPPFLAGS_BASIC = $(DEFINES) $(INCLUDES)

CPPFLAGS += $(CPPFLAGS_OPTS) $(CPPFLAGS_TFL) $(CPPFLAGS_BASIC)

###########################################################################
## INLINED COMMANDS
###########################################################################

###########################################################################
## PHONY TARGETS
###########################################################################

.PHONY : all build clean info prebuild download execute


all : build
	@echo $(call FORMAT_FOR_ECHO,### Successfully generated all binary outputs.)


build : prebuild $(PRODUCT)


prebuild : 


download : $(PRODUCT)


execute : download


###########################################################################
## FINAL TARGET
###########################################################################

#---------------------------------
# Create a static library         
#---------------------------------

$(PRODUCT) : $(OBJS) $(PREBUILT_OBJS)
	@echo $(call FORMAT_FOR_ECHO,### Creating static library "$(PRODUCT)" ...)
	$(AR) $(ARFLAGS)  $(PRODUCT) $(OBJS)
	@echo $(call FORMAT_FOR_ECHO,### Created: "$(PRODUCT)")


###########################################################################
## INTERMEDIATE TARGETS
###########################################################################

#---------------------
# SOURCE-TO-OBJECT
#---------------------

%.o : %.c
	$(CC) $(CFLAGS) -o "$@" "$<"


%.o : %.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : %.cc
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : %.cp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : %.cxx
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : %.CPP
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : %.c++
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : %.C
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(RELATIVE_PATH_TO_ANCHOR)/%.c
	$(CC) $(CFLAGS) -o "$@" "$<"


%.o : $(RELATIVE_PATH_TO_ANCHOR)/%.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(RELATIVE_PATH_TO_ANCHOR)/%.cc
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(RELATIVE_PATH_TO_ANCHOR)/%.cp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(RELATIVE_PATH_TO_ANCHOR)/%.cxx
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(RELATIVE_PATH_TO_ANCHOR)/%.CPP
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(RELATIVE_PATH_TO_ANCHOR)/%.c++
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(RELATIVE_PATH_TO_ANCHOR)/%.C
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(START_DIR)/%.c
	$(CC) $(CFLAGS) -o "$@" "$<"


%.o : $(START_DIR)/%.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(START_DIR)/%.cc
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(START_DIR)/%.cp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(START_DIR)/%.cxx
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(START_DIR)/%.CPP
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(START_DIR)/%.c++
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(START_DIR)/%.C
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : /home/daroe/asr_workspace/asr_flightstack/src/asr_perception/matlab/%.c
	$(CC) $(CFLAGS) -o "$@" "$<"


%.o : /home/daroe/asr_workspace/asr_flightstack/src/asr_perception/matlab/%.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : /home/daroe/asr_workspace/asr_flightstack/src/asr_perception/matlab/%.cc
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : /home/daroe/asr_workspace/asr_flightstack/src/asr_perception/matlab/%.cp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : /home/daroe/asr_workspace/asr_flightstack/src/asr_perception/matlab/%.cxx
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : /home/daroe/asr_workspace/asr_flightstack/src/asr_perception/matlab/%.CPP
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : /home/daroe/asr_workspace/asr_flightstack/src/asr_perception/matlab/%.c++
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : /home/daroe/asr_workspace/asr_flightstack/src/asr_perception/matlab/%.C
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/%.c
	$(CC) $(CFLAGS) -o "$@" "$<"


%.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/%.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/%.cc
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/%.cp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/%.cxx
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/%.CPP
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/%.c++
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/%.C
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/aruco/%.c
	$(CC) $(CFLAGS) -o "$@" "$<"


%.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/aruco/%.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/aruco/%.cc
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/aruco/%.cp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/aruco/%.cxx
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/aruco/%.CPP
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/aruco/%.c++
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


%.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/aruco/%.C
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


readArucoMarkerCore.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/readArucoMarkerCore.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


cgCommon.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/cgCommon.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


readArucoImpl.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/aruco/readArucoImpl.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


arucoDictionary.o : $(MATLAB_ROOT)/toolbox/vision/builtins/src/ocv/aruco/arucoDictionary.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


detect_aruco_in_camera_frame_data.o : $(START_DIR)/detect_aruco_in_camera_frame_data.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


rt_nonfinite.o : $(START_DIR)/rt_nonfinite.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


rtGetNaN.o : $(START_DIR)/rtGetNaN.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


rtGetInf.o : $(START_DIR)/rtGetInf.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


detect_aruco_in_camera_frame_initialize.o : $(START_DIR)/detect_aruco_in_camera_frame_initialize.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


detect_aruco_in_camera_frame_terminate.o : $(START_DIR)/detect_aruco_in_camera_frame_terminate.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


detect_aruco_in_camera_frame.o : $(START_DIR)/detect_aruco_in_camera_frame.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


readArucoMarker.o : $(START_DIR)/readArucoMarker.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


estimateWorldCameraPose.o : $(START_DIR)/estimateWorldCameraPose.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


rand.o : $(START_DIR)/rand.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


eml_rand_mt19937ar.o : $(START_DIR)/eml_rand_mt19937ar.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


solveP3P.o : $(START_DIR)/solveP3P.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


roots.o : $(START_DIR)/roots.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


xnrm2.o : $(START_DIR)/xnrm2.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


svd.o : $(START_DIR)/svd.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


xzlangeM.o : $(START_DIR)/xzlangeM.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


xdotc.o : $(START_DIR)/xdotc.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


rotationMatrixToVector.o : $(START_DIR)/rotationMatrixToVector.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


eml_rand_mt19937ar_stateful.o : $(START_DIR)/eml_rand_mt19937ar_stateful.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


xzgebal.o : $(START_DIR)/xzgebal.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


xzgehrd.o : $(START_DIR)/xzgehrd.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


xdlahqr.o : $(START_DIR)/xdlahqr.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


xzlarfg.o : $(START_DIR)/xzlarfg.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


xdlanv2.o : $(START_DIR)/xdlanv2.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


xaxpy.o : $(START_DIR)/xaxpy.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


xrotg.o : $(START_DIR)/xrotg.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


xrot.o : $(START_DIR)/xrot.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


xswap.o : $(START_DIR)/xswap.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


xzlascl.o : $(START_DIR)/xzlascl.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


cameraIntrinsics.o : $(START_DIR)/cameraIntrinsics.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


detect_aruco_in_camera_frame_rtwutil.o : $(START_DIR)/detect_aruco_in_camera_frame_rtwutil.cpp
	$(CPP) $(CPPFLAGS) -o "$@" "$<"


###########################################################################
## DEPENDENCIES
###########################################################################

$(ALL_OBJS) : rtw_proj.tmw $(MAKEFILE)


###########################################################################
## MISCELLANEOUS TARGETS
###########################################################################

info : 
	@echo $(call FORMAT_FOR_ECHO,### PRODUCT = $(PRODUCT))
	@echo $(call FORMAT_FOR_ECHO,### PRODUCT_TYPE = $(PRODUCT_TYPE))
	@echo $(call FORMAT_FOR_ECHO,### BUILD_TYPE = $(BUILD_TYPE))
	@echo $(call FORMAT_FOR_ECHO,### INCLUDES = $(INCLUDES))
	@echo $(call FORMAT_FOR_ECHO,### DEFINES = $(DEFINES))
	@echo $(call FORMAT_FOR_ECHO,### ALL_SRCS = $(ALL_SRCS))
	@echo $(call FORMAT_FOR_ECHO,### ALL_OBJS = $(ALL_OBJS))
	@echo $(call FORMAT_FOR_ECHO,### LIBS = $(LIBS))
	@echo $(call FORMAT_FOR_ECHO,### MODELREF_LIBS = $(MODELREF_LIBS))
	@echo $(call FORMAT_FOR_ECHO,### SYSTEM_LIBS = $(SYSTEM_LIBS))
	@echo $(call FORMAT_FOR_ECHO,### TOOLCHAIN_LIBS = $(TOOLCHAIN_LIBS))
	@echo $(call FORMAT_FOR_ECHO,### CFLAGS = $(CFLAGS))
	@echo $(call FORMAT_FOR_ECHO,### LDFLAGS = $(LDFLAGS))
	@echo $(call FORMAT_FOR_ECHO,### SHAREDLIB_LDFLAGS = $(SHAREDLIB_LDFLAGS))
	@echo $(call FORMAT_FOR_ECHO,### CPPFLAGS = $(CPPFLAGS))
	@echo $(call FORMAT_FOR_ECHO,### CPP_LDFLAGS = $(CPP_LDFLAGS))
	@echo $(call FORMAT_FOR_ECHO,### CPP_SHAREDLIB_LDFLAGS = $(CPP_SHAREDLIB_LDFLAGS))
	@echo $(call FORMAT_FOR_ECHO,### ARFLAGS = $(ARFLAGS))
	@echo $(call FORMAT_FOR_ECHO,### MEX_CFLAGS = $(MEX_CFLAGS))
	@echo $(call FORMAT_FOR_ECHO,### MEX_CPPFLAGS = $(MEX_CPPFLAGS))
	@echo $(call FORMAT_FOR_ECHO,### MEX_LDFLAGS = $(MEX_LDFLAGS))
	@echo $(call FORMAT_FOR_ECHO,### MEX_CPPLDFLAGS = $(MEX_CPPLDFLAGS))
	@echo $(call FORMAT_FOR_ECHO,### DOWNLOAD_FLAGS = $(DOWNLOAD_FLAGS))
	@echo $(call FORMAT_FOR_ECHO,### EXECUTE_FLAGS = $(EXECUTE_FLAGS))
	@echo $(call FORMAT_FOR_ECHO,### MAKE_FLAGS = $(MAKE_FLAGS))


clean : 
	$(ECHO) "### Deleting all derived files ..."
	$(RM) $(PRODUCT)
	$(RM) $(ALL_OBJS)
	$(ECHO) "### Deleted all derived files."


