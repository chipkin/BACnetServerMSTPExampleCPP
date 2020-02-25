# The Compiler: gcc for C, g++ for C++
CC := g++
NAME := BACnetServerMSTPExampleCPP_linux_x64_Release

# Compiler flags:
# -m32 for 32bit, -m64 for 64bit
# -Wall turns on most, but not all, compiler warnings
#
CFLAGS := -m64 -Wall

DEBUGFLAGS = -O0 -g3 -DCAS_BACNET_STACK_LIB_TYPE_LIB
RELEASEFLAGS = -O3 -DCAS_BACNET_STACK_LIB_TYPE_LIB
OBJECTFLAGS = -c -fmessage-length=0 -fPIC -MMD -MP
LDFLAGS = -static

SOURCES = $(wildcard BACnetServerMSTPExample/*.cpp) $(wildcard submodules/cas-bacnet-stack/adapters/cpp/*.cpp)  $(wildcard submodules/cas-bacnet-stack/submodules/cas-common/source/*.cpp) 
OBJECTS = $(addprefix obj/,$(notdir $(SOURCES:.cpp=.o))) obj/mstp.o obj/rs232.o
INCLUDES = -IBACnetServerMSTPExample -Isubmodules/cas-bacnet-stack/adapters/cpp -Isubmodules/cas-bacnet-stack/source -Isubmodules/cas-bacnet-stack/submodules/cas-common/source -Isubmodules/serial
LIBPATH = -Lbin 
LIB = -ldl -lCASBACnetStack_x64_Release 

# Build Target
TARGET = $(NAME)

all: $(NAME)

$(NAME): $(OBJECTS)
	@mkdir -p obj
	@echo 'Building target: $@'
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(NAME) $(OBJECTS) $(LIBPATH) $(LIB)
	@echo 'Finished building target: $@'
	@echo ' '

obj/%.o: BACnetServerMSTPExample/%.cpp
	$(CC) $(RELEASEFLAGS) $(CFLAGS) $(OBJECTFLAGS) $(INCLUDES) $(LIBPATH) -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o $@ $<

obj/%.o: submodules/cas-bacnet-stack/adapters/cpp/%.cpp
	$(CC) $(RELEASEFLAGS) $(CFLAGS) $(OBJECTFLAGS) $(INCLUDES) $(LIBPATH) -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o $@ $<

obj/%.o: submodules/cas-bacnet-stack/submodules/cas-common/source/%.cpp
	$(CC) $(RELEASEFLAGS) $(CFLAGS) $(OBJECTFLAGS) $(INCLUDES) $(LIBPATH) -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o $@ $<

obj/rs232.o : submodules/serial/rs232.h submodules/serial/rs232.c 
	$(CC) $(RELEASEFLAGS) $(CFLAGS) $(OBJECTFLAGS) $(INCLUDES) $(LIBPATH) -c submodules/serial/rs232.c -o obj/rs232.o

obj/mstp.o : submodules/cas-bacnet-stack/source/MSTP.h submodules/cas-bacnet-stack/source/MSTP.c 
	$(CC) $(RELEASEFLAGS) $(CFLAGS) $(OBJECTFLAGS) $(INCLUDES) $(LIBPATH) -c submodules/cas-bacnet-stack/source/MSTP.c -o obj/mstp.o

install:
	install -D $(NAME) bin/$(NAME)
	$(RM) $(NAME)

# make clean
# Removes target file and any .o object files, 
# .d dependency files, or ~ backup files
clean:
	$(RM) $(NAME) obj/* *~
