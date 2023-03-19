CC=gcc
#CXX=$(CROSS_COMPILE)g++
CXX=g++
LD=ld

INC=-I./include \
	-I/usr/include \
	-I/usr/include/opencv2 \
	-I/usr/local/include/opencv2 \
	-I/usr/local/include/opencv4 \
#   -I$(PATH_SYSROOT)/usr/include \
#   -I$(PATH_SYSROOT)/usr/lib/cortexa9hf-vfp-neon-poky-linux-gnueabi/gcc/arm-poky-linux-gnueabi/4.8.2/include \
#   -I$(PATH_SYSROOT)/usr/include/glib-2.0 \
#   -I$(PATH_SYSROOT)/usr/include/glib-2.0/include \
#   -I$(PATH_SYSROOT)/usr/include/opencv \
#   -I$(PATH_SYSROOT)/usr/include/opencv2 \
#   -I$(PATH_KERNEL)/include/uapi \
#   -I$(PATH_KERNEL)/include

LIB_DIR=/usr/lib \
#		-L/home/success/caffe-jacinto/build/lib/libcaffe-nv.so \#
#		-L/home/success/caffe-jacinto/build/lib/libcaffe-nv.so.0.16 \
		-L/usr/local/cuda/lib64 \
#		-L/usr/lib/nvidia-384 \
		
	
DEFINES =-D__EXPORTED_HEADERS__ 

CFLAGS =$(DEFINES) $(INC) -O0 -g -Wall -pthread -lm -lpthread -lstdc++
#-DUSE_CUDNN -DUSE_NCCL -MMD -MP
CXXFLAGS =$(DEFINES) $(INC) -std=c++11 -O0 -g -Wall -pthread -ggdb -fopenmp -lm -lpthread 

#IPM DEBUG OPTION
CFLAGS += -DDAEGU
CXXFLAGS += -DDAEGU

LFLAGS =-Wl,-rpath-link=$(LIB_DIR) -L$(LIB_DIR) 
LFLAGS +=`pkg-config opencv --cflags --libs`
LFLAGS +=`pkg-config opencv4 --cflags --libs`


OBJDIR = obj
BINDIR = bin
OBJS = main.o

TARGET = ld

all : $(TARGET)
#	cp $(BINDIR)/$(TARGET) $(PATH_OUT)
#	scp $(BINDIR)/$(TARGET) root@192.168.100.2:/eKdac/bin/
	cd bin
#	sync	

install : $(TARGET)
#	cp $(BINDIR)/$(TARGET) $(PATH_OUT)
#	cp $(BINDIR)/$(TARGET) $(PATH_OUT)/../../rom/rfs/eKdac/bin
#	scp $(BINDIR)/$(TARGET) root@192.168.100.2:/eKdac/bin/

clean :
	rm -rf $(OBJDIR)/* $(BINDIR)/*
	rm -rf *.o
	
$(TARGET) : $(OBJS)
#	$(CC) -o $@ $(OBJS) $(LFLAGS)
	$(CXX) -o $@ $(OBJS) $(CXXFLAGS) $(LFLAGS)
#	$(CXX) -o $@ $(OBJS)  $(CXXFLAGS) $(LFLAGS)
	mv $(OBJS) $(OBJDIR)
	mv $(TARGET) $(BINDIR)
	


