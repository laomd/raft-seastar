SRC_BASE_PATH=/root/seastar/phxpaxos

all:phxelection_bin 

include $(SRC_BASE_PATH)/makefile.mk

include Makefile.define
PHXELECTION_SRC=$(PHXELECTION_OBJ)
PHXELECTION_INCS=$(sort $(SRC_BASE_PATH)/sample/phxelection $(PHXPAXOS_PLUGIN_PATH) $(PHXPAXOS_INCLUDE_PATH) $(LEVELDB_INCLUDE_PATH) $(PROTOBUF_INCLUDE_PATH))
PHXELECTION_FULL_LIB_PATH=$(SRC_BASE_PATH)//sample/phxelection
PHXELECTION_EXTRA_CPPFLAGS=-Wall -Werror

CPPFLAGS+=$(patsubst %,-I%, $(PHXELECTION_INCS))
CPPFLAGS+=$(PHXELECTION_EXTRA_CPPFLAGS)

PHXELECTION_LINK=
PHXELECTION_SYS_LIB=$(PHXPAXOS_LIB_PATH)/libphxpaxos_plugin.a $(PHXPAXOS_LIB_PATH)/libphxpaxos.a $(LEVELDB_LIB_PATH)/libleveldb.a $(PROTOBUF_LIB_PATH)/libprotobuf.a $(GLOG_LIB_PATH)/libglog.a $(GFLAGS_LIB_PATH)/libgflags.a -lpthread
PHXELECTION_FLAGS+=$(LDFLAGS)

PHXELECTION_FLAGS+=$(patsubst %,-l%, $(PHXELECTION_LINK))
PHXELECTION_FLAGS+=$(PHXELECTION_SYS_LIB)
phxelection_bin:phxelection_dir phxelection

phxelection_dir:$(PHXELECTION_FULL_LIB_PATH)
	@for dir in $^;\
	do \
	current_dir=`readlink $$dir -m`;\
	pwd_dir=`pwd`;\
	pwd_dir=`readlink $$pwd_dir -m`;\
	if ([ "$$current_dir" != "$$pwd_dir" ]); then \
	make -C $$dir;\
	fi;\
	done

phxelection:$(PHXELECTION_SRC)
	$(CXX) $^ -o $@ $(PHXELECTION_FLAGS)
	cp $@ $(SRC_BASE_PATH)/.sbin/

clean:
	rm -rf *.o *.pb.* phxelection $(SRC_BASE_PATH)/.sbin/phxelection phxelection 

