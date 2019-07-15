AM_SRCS := native/trm.c \
           native/ioe.c \
           native/cte.c \
           native/trap.S \
           native/mpe.c \
           native/vme.c \
           native/platform.cpp \
           native/devices/input.c \
           native/devices/timer.c \
           native/devices/video.c \

image:
	@echo + LD "->" $(BINARY)
	@g++ -pie -o $(BINARY) -pthread -Wl,--start-group $(LINK_FILES) -Wl,--end-group -lSDL2 -lGL -lrt

run:
	$(BINARY)