TARGET     = meshtastic-compression-test

SRCS       = main.c 
SRCS      += arithcode.c ac_stream.c

# protobuf auto-generated source
SRCS      += admin.pb.c clientonly.pb.c portnums.pb.c paxcount.pb.c mqtt.pb.c module_config.pb.c xmodem.pb.c
SRCS      += storeforward.pb.c telemetry.pb.c remote_hardware.pb.c device_ui.pb.c cannedmessages.pb.c config.pb.c
SRCS      += atak.pb.c powermon.pb.c connection_status.pb.c apponly.pb.c channel.pb.c deviceonly.pb.c
SRCS      += rtttl.pb.c localonly.pb.c mesh.pb.c

# include search paths (-I)
INCS       = -Iinc
INCS      += -Iarithcode
INCS      += -Igenerated
INCS      += -I/usr/local/include/nanopb

LIBS       = -lmosquitto -lprotobuf-nanopb 

# Compiler flags
CFLAGS     = -Wall -g -std=gnu11 -Og
CFLAGS    += -Wno-unused-variable -Wno-unused-function
CFLAGS    += $(INCS) $(DEFS)

# Linker flags
LDFLAGS    = $(LIBS)

# Source search paths
VPATH     = src
VPATH     += arithcode
VPATH     += generated/meshtastic

OBJS       = $(addprefix obj/,$(SRCS:.c=.o))
DEPS       = $(addprefix dep/,$(SRCS:.c=.d))

# Prettify output
V = 0
ifeq ($V, 0)
	Q = @
	P = > /dev/null
endif

###################################################

all: $(TARGET)

generated/meshtastic:
	$Qmkdir -p generated
	$Qsed -i .bak -f de-cpp.sed protobufs/meshtastic/deviceonly.proto
	$Qcd protobufs && nanopb_generator -C -D ../generated meshtastic/*.proto

dirs: dep obj

dep obj:
	@echo "[MKDIR]   $@"
	$Qmkdir -p $@

###################################################

# Custom flags for third-party files to silence their build warnings

###################################################

dep/%.d: %.c | dirs generated/meshtastic
	@echo "[DEP]     $(notdir $<)"
	$Q$(CC) $(CFLAGS) -MM -MF $@ $<

obj/%.o : %.c | dep/%.d 
	@echo "[CC]      $(notdir $<)"
	$Q$(CC) $(CFLAGS) -c -o $@ $<

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

$(TARGET): $(OBJS)
	@echo "[LD]      $(TARGET)"
	$Q$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	@echo "[RM]      $(TARGET)"; rm -f $(TARGET)
	@echo "[RM]      $(TARGET).map"; rm -f $(TARGET).map
	@echo "[RM]      $(TARGET).lst"; rm -f $(TARGET).lst
	@echo "[RMDIR]   dep"          ; rm -fr dep
	@echo "[RMDIR]   obj"          ; rm -fr obj
	@echo "[RMDIR]   generated"    ; rm -fr generated

.PHONY: all dirs clean


