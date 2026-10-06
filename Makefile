CC	=	gcc
CFLAGS	=	-Wall	-Wextra	-g	-pthread
LDFLAGS	=	-lrt	-lpthread	-lsqlite3	-lcjson

BUILD	=	build
TARGET	=	$(BUILD)/smart_home

SRCS = src/main.c src/sensor_simulator.c src/collector.c src/alert_engine.c src/http_server.c src/logger.c src/ipc.c

all:	$(TARGET)

$(TARGET):	$(SRCS)	|	$(BUILD)
	$(CC)	$(CFLAGS)	$(SRCS)	-o	$@	$(LDFLAGS)
	@echo	"=====pc版编译完成====="

$(BUILD):
	@mkdir	-p	$(BUILD)

clean:
	rm	-rf	$(BUILD)

.PHONY:	all	clean
